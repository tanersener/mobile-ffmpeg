/* stream.c - The stream implementation
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz
 *
 * This file is part of OpenCDK.
 *
 * The OpenCDK library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "opencdk.h"
#include "main.h"
#include "filters.h"
#include "stream.h"
#include "types.h"

/* This is the maximal amount of bytes we map. */
#define MAX_MAP_SIZE 16777216

static cdk_error_t stream_flush(cdk_stream_t s);
static cdk_error_t stream_filter_write(cdk_stream_t s);
static int stream_cache_flush(cdk_stream_t s, FILE * fp);
struct stream_filter_s *filter_add(cdk_stream_t s, filter_fnct_t fnc,
				   int type);


/* FIXME: The read/write/putc/getc function cannot directly
	  return an error code. It is stored in an error variable
	  inside the string. Right now there is no code to
	  return the error code or to reset it. */

/**
 * cdk_stream_open:
 * @file: The file to open
 * @ret_s: The new STREAM object
 * 
 * Creates a new stream based on an existing file. The stream is
 * opened in read-only mode.
 **/
cdk_error_t cdk_stream_open(const char *file, cdk_stream_t * ret_s)
{
	return _cdk_stream_open_mode(file, "rb", ret_s);
}


/* Helper function to allow to open a stream in different modes. */
cdk_error_t
_cdk_stream_open_mode(const char *file, const char *mode,
		      cdk_stream_t * ret_s)
{
	cdk_stream_t s;

	if (!file || !ret_s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
#ifdef DEBUG_STREAM
	_gnutls_read_log("open stream `%s'\n", file);
#endif
	*ret_s = NULL;
	s = cdk_calloc(1, sizeof *s);
	if (!s) {
		gnutls_assert();
		return CDK_Out_Of_Core;
	}
	s->fname = cdk_strdup(file);
	if (!s->fname) {
		cdk_free(s);
		gnutls_assert();
		return CDK_Out_Of_Core;
	}
	s->fp = fopen(file, mode);
	if (!s->fp) {
		cdk_free(s->fname);
		cdk_free(s);
		gnutls_assert();
		return CDK_File_Error;
	}
#ifdef DEBUG_STREAM
	_gnutls_read_log("open stream fd=%d\n", fileno(s->fp));
#endif
	s->flags.write = 0;
	*ret_s = s;
	return 0;
}


/**
 * cdk_stream_new_from_cbs:
 * @cbs: the callback context with all user callback functions
 * @opa: uint8_t handle which is passed to all callbacks.
 * @ret_s: the allocated stream
 * 
 * This function creates a stream which uses user callback
 * for the core operations (open, close, read, write, seek).
 */
cdk_error_t
cdk_stream_new_from_cbs(cdk_stream_cbs_t cbs, void *opa,
			cdk_stream_t * ret_s)
{
	cdk_stream_t s;

	if (!cbs || !opa || !ret_s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	*ret_s = NULL;
	s = cdk_calloc(1, sizeof *s);
	if (!s) {
		gnutls_assert();
		return CDK_Out_Of_Core;
	}

	s->cbs.read = cbs->read;
	s->cbs.write = cbs->write;
	s->cbs.seek = cbs->seek;
	s->cbs.release = cbs->release;
	s->cbs.open = cbs->open;
	s->cbs_hd = opa;
	*ret_s = s;

	/* If there is a user callback for open, we need to call it
	   here because read/write expects an open stream. */
	if (s->cbs.open)
		return s->cbs.open(s->cbs_hd);
	return 0;
}


/**
 * cdk_stream_new: 
 * @file: The name of the new file
 * @ret_s: The new STREAM object
 * 
 * Create a new stream into the given file.
 **/
cdk_error_t cdk_stream_new(const char *file, cdk_stream_t * ret_s)
{
	cdk_stream_t s;

	if (!ret_s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
#ifdef DEBUG_STREAM
	_gnutls_read_log("new stream `%s'\n", file ? file : "[temp]");
#endif
	*ret_s = NULL;
	s = cdk_calloc(1, sizeof *s);
	if (!s) {
		gnutls_assert();
		return CDK_Out_Of_Core;
	}
	s->flags.write = 1;
	if (!file)
		s->flags.temp = 1;
	else {
		s->fname = cdk_strdup(file);
		if (!s->fname) {
			cdk_free(s);
			gnutls_assert();
			return CDK_Out_Of_Core;
		}
	}
	s->fp = _cdk_tmpfile();
	if (!s->fp) {
		cdk_free(s->fname);
		cdk_free(s);
		gnutls_assert();
		return CDK_File_Error;
	}
#ifdef DEBUG_STREAM
	_gnutls_read_log("new stream fd=%d\n", fileno(s->fp));
#endif
	*ret_s = s;
	return 0;
}

/**
 * cdk_stream_create: 
 * @file: the filename
 * @ret_s: the object
 *
 * Creates a new stream.
 * The difference to cdk_stream_new is, that no filtering can be used with
 * this kind of stream and everything is written directly to the stream.
 **/
cdk_error_t cdk_stream_create(const char *file, cdk_stream_t * ret_s)
{
	cdk_stream_t s;

	if (!file || !ret_s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
#ifdef DEBUG_STREAM
	_gnutls_read_log("create stream `%s'\n", file);
#endif
	*ret_s = NULL;
	s = cdk_calloc(1, sizeof *s);
	if (!s) {
		gnutls_assert();
		return CDK_Out_Of_Core;
	}
	s->flags.write = 1;
	s->flags.filtrated = 1;
	s->fname = cdk_strdup(file);
	if (!s->fname) {
		cdk_free(s);
		gnutls_assert();
		return CDK_Out_Of_Core;
	}
	s->fp = fopen(file, "w+b");
	if (!s->fp) {
		cdk_free(s->fname);
		cdk_free(s);
		gnutls_assert();
		return CDK_File_Error;
	}
#ifdef DEBUG_STREAM
	_gnutls_read_log("stream create fd=%d\n", fileno(s->fp));
#endif
	*ret_s = s;
	return 0;
}


/**
 * cdk_stream_tmp_new:
 * @r_out: the new temp stream.
 * 
 * Allocates a new tempory stream which is not associated with a file.
 */
cdk_error_t cdk_stream_tmp_new(cdk_stream_t * r_out)
{
	return cdk_stream_new(NULL, r_out);
}



/**
 * cdk_stream_tmp_from_mem:
 * @buf: the buffer which shall be written to the temp stream.
 * @buflen: how large the buffer is
 * @r_out: the new stream with the given contents.
 * 
 * Creates a new tempory stream with the given contests.
 */
cdk_error_t
cdk_stream_tmp_from_mem(const void *buf, size_t buflen,
			cdk_stream_t * r_out)
{
	cdk_stream_t s;
	cdk_error_t rc;
	int nwritten;

	*r_out = NULL;
	rc = cdk_stream_tmp_new(&s);
	if (rc) {
		gnutls_assert();
		return rc;
	}

	nwritten = cdk_stream_write(s, buf, buflen);
	if (nwritten == EOF) {
		cdk_stream_close(s);
		gnutls_assert();
		return s->error;
	}
	cdk_stream_seek(s, 0);
	*r_out = s;
	return 0;
}


cdk_error_t
_cdk_stream_fpopen(FILE * fp, unsigned write_mode, cdk_stream_t * ret_out)
{
	cdk_stream_t s;

	*ret_out = NULL;
	s = cdk_calloc(1, sizeof *s);
	if (!s) {
		gnutls_assert();
		return CDK_Out_Of_Core;
	}
#ifdef DEBUG_STREAM
	_gnutls_read_log("stream ref fd=%d\n", fileno(fp));
#endif
	s->fp = fp;
	s->fp_ref = 1;
	s->flags.filtrated = 1;
	s->flags.write = write_mode;

	*ret_out = s;
	return 0;
}


cdk_error_t _cdk_stream_append(const char *file, cdk_stream_t * ret_s)
{
	cdk_stream_t s;
	cdk_error_t rc;

	if (!ret_s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
	*ret_s = NULL;

	rc = _cdk_stream_open_mode(file, "a+b", &s);
	if (rc) {
		gnutls_assert();
		return rc;
	}

	/* In the append mode, we need to write to the flag. */
	s->flags.write = 1;
	*ret_s = s;
	return 0;
}

/**
 * cdk_stream_is_compressed:
 * @s: the stream
 *
 * Check whether stream is compressed.
 *
 * Returns: 0 if the stream is uncompressed, otherwise the compression
 *   algorithm.
 */
int cdk_stream_is_compressed(cdk_stream_t s)
{
	if (!s)
		return 0;
	return s->flags.compressed;
}

void _cdk_stream_set_compress_algo(cdk_stream_t s, int algo)
{
	if (!s)
		return;
	s->flags.compressed = algo;
}


cdk_error_t cdk_stream_flush(cdk_stream_t s)
{
	cdk_error_t rc;

	if (!s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	/* The user callback does not support flush */
	if (s->cbs_hd)
		return 0;

	/* For read-only streams, no flush is needed. */
	if (!s->flags.write)
		return 0;

	if (!s->flags.filtrated) {
		if (!cdk_stream_get_length(s))
			return 0;
		rc = cdk_stream_seek(s, 0);
		if (!rc)
			rc = stream_flush(s);
		if (!rc)
			rc = stream_filter_write(s);
		s->flags.filtrated = 1;
		if (rc) {
			s->error = rc;
			gnutls_assert();
			return rc;
		}
	}
	return 0;
}


void cdk_stream_tmp_set_mode(cdk_stream_t s, int val)
{
	if (s && s->flags.temp)
		s->fmode = val;
}


/**
 * cdk_stream_close:
 * @s: The STREAM object.
 *
 * Close a stream and flush all buffers.  This function work different
 * for read or write streams. When the stream is for reading, the
 * filtering is already done and we can simply close the file and all
 * buffers.  But for the case it's a write stream, we need to apply
 * all registered filters now. The file is closed in the filter
 * function and not here.
 **/
cdk_error_t cdk_stream_close(cdk_stream_t s)
{
	struct stream_filter_s *f, *f2;
	cdk_error_t rc;

	if (!s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
#ifdef DEBUG_STREAM
	_gnutls_read_log("close stream ref=%d `%s'\n",
			 s->fp_ref, s->fname ? s->fname : "[temp]");
#endif

	/* In the user callback mode, we call the release cb if possible
	   and just free the stream. */
	if (s->cbs_hd) {
		if (s->cbs.release)
			rc = s->cbs.release(s->cbs_hd);
		else
			rc = 0;
		cdk_free(s);
		gnutls_assert();
		return rc;
	}


	rc = 0;
	if (!s->flags.filtrated && !s->error)
		rc = cdk_stream_flush(s);
	if (!s->fp_ref && (s->fname || s->flags.temp)) {
		int err;

#ifdef DEBUG_STREAM
		_gnutls_read_log("close stream fd=%d\n", fileno(s->fp));
#endif
		err = fclose(s->fp);
		s->fp = NULL;
		if (err)
			rc = CDK_File_Error;
	}

	/* Iterate over the filter list and use the cleanup flag to
	   free the allocated internal structures. */
	f = s->filters;
	while (f) {
		f2 = f->next;
		if (f->fnct)
			f->fnct(f->uint8_t, STREAMCTL_FREE, NULL, NULL);
		cdk_free(f);
		f = f2;
	}

	if (s->fname) {
		cdk_free(s->fname);
		s->fname = NULL;
	}

	cdk_free(s->cache.buf);
	s->cache.alloced = 0;

	cdk_free(s);

	if (rc)
		gnutls_assert();

	return rc;
}


/**
 * cdk_stream_eof:
 * @s: The STREAM object.
 *
 *  Return if the associated file handle was set to EOF.  This
 *  function will only work with read streams.
 **/
int cdk_stream_eof(cdk_stream_t s)
{
	return s ? s->flags.eof : -1;
}


const char *_cdk_stream_get_fname(cdk_stream_t s)
{
	if (!s)
		return NULL;
	if (s->flags.temp)
		return NULL;
	return s->fname ? s->fname : NULL;
}


/* Return the underlying FP of the stream.
   WARNING: This handle should not be closed. */
FILE *_cdk_stream_get_fp(cdk_stream_t s)
{
	return s ? s->fp : NULL;
}


int _cdk_stream_get_errno(cdk_stream_t s)
{
	return s ? s->error : CDK_Inv_Value;
}


/**
 * cdk_stream_get_length:
 * @s: The STREAM object.
 *
 * Return the length of the associated file handle.  This function
 * should work for both read and write streams. For write streams an
 * additional flush is used to write possible pending data.
 **/
off_t cdk_stream_get_length(cdk_stream_t s)
{
	struct stat statbuf;
	cdk_error_t rc;

	if (!s) {
		gnutls_assert();
		return (off_t) 0;
	}

	/* The user callback does not support stat. */
	if (s->cbs_hd)
		return 0;

	rc = stream_flush(s);
	if (rc) {
		s->error = rc;
		gnutls_assert();
		return (off_t) 0;
	}

	if (fstat(fileno(s->fp), &statbuf)) {
		s->error = CDK_File_Error;
		gnutls_assert();
		return (off_t) 0;
	}

	return statbuf.st_size;
}


static struct stream_filter_s *filter_add2(cdk_stream_t s)
{
	struct stream_filter_s *f;

	assert(s);

	f = cdk_calloc(1, sizeof *f);
	if (!f)
		return NULL;
	f->next = s->filters;
	s->filters = f;
	return f;
}


static struct stream_filter_s *filter_search(cdk_stream_t s,
					     filter_fnct_t fnc)
{
	struct stream_filter_s *f;

	assert(s);

	for (f = s->filters; f; f = f->next) {
		if (f->fnct == fnc)
			return f;
	}

	return NULL;
}

static inline void set_uint8_t(struct stream_filter_s *f)
{
	switch (f->type) {
	case fARMOR:
		f->uint8_t = &f->u.afx;
		break;
	case fCIPHER:
		f->uint8_t = &f->u.cfx;
		break;
	case fLITERAL:
		f->uint8_t = &f->u.pfx;
		break;
	case fCOMPRESS:
		f->uint8_t = &f->u.zfx;
		break;
	case fHASH:
		f->uint8_t = &f->u.mfx;
		break;
	case fTEXT:
		f->uint8_t = &f->u.tfx;
		break;
	default:
		f->uint8_t = NULL;
	}

}

struct stream_filter_s *filter_add(cdk_stream_t s, filter_fnct_t fnc,
				   int type)
{
	struct stream_filter_s *f;

	assert(s);

	s->flags.filtrated = 0;
	f = filter_search(s, fnc);
	if (f)
		return f;
	f = filter_add2(s);
	if (!f)
		return NULL;
	f->fnct = fnc;
	f->flags.enabled = 1;
	f->tmp = NULL;
	f->type = type;

	set_uint8_t(f);

	return f;
}

static int stream_get_mode(cdk_stream_t s)
{
	assert(s);

	if (s->flags.temp)
		return s->fmode;
	return s->flags.write;
}


static filter_fnct_t stream_id_to_filter(int type)
{
	switch (type) {
	case fARMOR:
		return _cdk_filter_armor;
	case fLITERAL:
		return _cdk_filter_literal;
	case fTEXT:
		return _cdk_filter_text;
/*    case fCIPHER  : return _cdk_filter_cipher; */
/*    case fCOMPRESS: return _cdk_filter_compress; */
	default:
		return NULL;
	}
}


/**
 * cdk_stream_filter_disable: 
 * @s: The STREAM object
 * @type: The numberic filter ID.
 *
 * Disables the filter with the type 'type'.
 **/
cdk_error_t cdk_stream_filter_disable(cdk_stream_t s, int type)
{
	struct stream_filter_s *f;
	filter_fnct_t fnc;

	if (!s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	fnc = stream_id_to_filter(type);
	if (!fnc) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
	f = filter_search(s, fnc);
	if (f)
		f->flags.enabled = 0;
	return 0;
}


/* WARNING: tmp should not be closed by the caller. */
static cdk_error_t stream_fp_replace(cdk_stream_t s, FILE ** tmp)
{
	int rc;

	assert(s);

#ifdef DEBUG_STREAM
	_gnutls_read_log("replace stream fd=%d with fd=%d\n",
			 fileno(s->fp), fileno(*tmp));
#endif
	rc = fclose(s->fp);
	if (rc) {
		s->fp = NULL;
		gnutls_assert();
		return CDK_File_Error;
	}
	s->fp = *tmp;
	*tmp = NULL;
	return 0;
}


/* This function is exactly like filter_read, except the fact that we can't
   use tmpfile () all the time. That's why we open the real file when there
   is no last filter. */
static cdk_error_t stream_filter_write(cdk_stream_t s)
{
	struct stream_filter_s *f;
	cdk_error_t rc = 0;

	assert(s);

	if (s->flags.filtrated) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	for (f = s->filters; f; f = f->next) {
		if (!f->flags.enabled)
			continue;
		/* if there is no next filter, create the final output file */
#ifdef DEBUG_STREAM
		_gnutls_read_log
		    ("filter [write]: last filter=%d fname=%s\n",
		     f->next ? 1 : 0, s->fname);
#endif
		if (!f->next && s->fname)
			f->tmp = fopen(s->fname, "w+b");
		else
			f->tmp = _cdk_tmpfile();
		if (!f->tmp) {
			rc = CDK_File_Error;
			break;
		}
		/* If there is no next filter, flush the cache. We also do this
		   when the next filter is the armor filter because this filter
		   is special and before it starts, all data should be written. */
		if ((!f->next || f->next->type == fARMOR) && s->cache.size) {
			rc = stream_cache_flush(s, f->tmp);
			if (rc)
				break;
		}
		rc = f->fnct(f->uint8_t, f->ctl, s->fp, f->tmp);
#ifdef DEBUG_STREAM
		_gnutls_read_log("filter [write]: type=%d rc=%d\n",
				 f->type, rc);
#endif
		if (!rc)
			rc = stream_fp_replace(s, &f->tmp);
		if (!rc)
			rc = cdk_stream_seek(s, 0);
		if (rc) {
#ifdef DEBUG_STREAM
			_gnutls_read_log("filter [close]: fd=%d\n",
					 fileno(f->tmp));
#endif
			fclose(f->tmp);
			f->tmp = NULL;
			break;
		}
	}
	return rc;
}


/* Here all data from the file handle is passed through all filters.
   The scheme works like this:
   Create a tempfile and use it for the output of the filter. Then the
   original file handle will be closed and replace with the temp handle.
   The file pointer will be set to the begin and the game starts again. */
static cdk_error_t stream_filter_read(cdk_stream_t s)
{
	struct stream_filter_s *f;
	cdk_error_t rc = 0;

	assert(s);

	if (s->flags.filtrated)
		return 0;

	for (f = s->filters; f; f = f->next) {
		if (!f->flags.enabled)
			continue;
		if (f->flags.error) {
#ifdef DEBUG_STREAM
			_gnutls_read_log
			    ("filter %s [read]: has the error flag; skipped\n",
			     s->fname ? s->fname : "[temp]");
#endif
			continue;
		}

		f->tmp = _cdk_tmpfile();
		if (!f->tmp) {
			rc = CDK_File_Error;
			break;
		}
		rc = f->fnct(f->uint8_t, f->ctl, s->fp, f->tmp);
#ifdef DEBUG_STREAM
		_gnutls_read_log("filter %s [read]: type=%d rc=%d\n",
				 s->fname ? s->fname : "[temp]", f->type,
				 rc);
#endif
		if (rc) {
			f->flags.error = 1;
			break;
		}

		f->flags.error = 0;
		/* If the filter is read-only, do not replace the FP because
		   the contents were not altered in any way. */
		if (!f->flags.rdonly) {
			rc = stream_fp_replace(s, &f->tmp);
			if (rc)
				break;
		} else {
			fclose(f->tmp);
			f->tmp = NULL;
		}
		rc = cdk_stream_seek(s, 0);
		if (rc)
			break;
		/* Disable the filter after it was successfully used. The idea
		   is the following: let's say the armor filter was pushed and
		   later more filters were added. The second time the filter code
		   will be executed, only the new filter should be started but
		   not the old because we already used it. */
		f->flags.enabled = 0;
	}

	return rc;
}


void *_cdk_stream_get_uint8_t(cdk_stream_t s, int fid)
{
	struct stream_filter_s *f;

	if (!s)
		return NULL;

	for (f = s->filters; f; f = f->next) {
		if ((int) f->type == fid)
			return f->uint8_t;
	}
	return NULL;
}


/**
 * cdk_stream_read: 
 * @s: The STREAM object.
 * @buf: The buffer to insert the readed bytes.
 * @count: Request so much bytes.
 *
 * Tries to read count bytes from the STREAM object.
 * When this function is called the first time, it can take a while
 * because all filters need to be processed. Please remember that you
 * need to add the filters in reserved order.
 **/
int cdk_stream_read(cdk_stream_t s, void *buf, size_t buflen)
{
	int nread;
	int rc;

	if (!s) {
		gnutls_assert();
		return EOF;
	}

	if (s->cbs_hd) {
		if (s->cbs.read)
			return s->cbs.read(s->cbs_hd, buf, buflen);
		return 0;
	}

	if (s->flags.write && !s->flags.temp) {
		s->error = CDK_Inv_Mode;
		gnutls_assert();
		return EOF;	/* This is a write stream */
	}

	if (!s->flags.no_filter && !s->cache.on && !s->flags.filtrated) {
		rc = stream_filter_read(s);
		if (rc) {
			s->error = rc;
			if (s->fp && feof(s->fp))
				s->flags.eof = 1;
			gnutls_assert();
			return EOF;
		}
		s->flags.filtrated = 1;
	}

	if (!buf || !buflen)
		return 0;

	nread = fread(buf, 1, buflen, s->fp);
	if (!nread)
		nread = EOF;

	if (feof(s->fp)) {
		s->error = 0;
		s->flags.eof = 1;
	}
	return nread;
}


int cdk_stream_getc(cdk_stream_t s)
{
	unsigned char buf[2];
	int nread;

	if (!s) {
		gnutls_assert();
		return EOF;
	}
	nread = cdk_stream_read(s, buf, 1);
	if (nread == EOF) {
		s->error = CDK_File_Error;
		gnutls_assert();
		return EOF;
	}
	return buf[0];
}


/**
 * cdk_stream_write: 
 * @s: The STREAM object
 * @buf: The buffer with the values to write.
 * @count: The size of the buffer.
 *
 * Tries to write count bytes into the stream.
 * In this function we simply write the bytes to the stream. We can't
 * use the filters here because it would mean they have to support
 * partial flushing.
 **/
int cdk_stream_write(cdk_stream_t s, const void *buf, size_t count)
{
	int nwritten;

	if (!s) {
		gnutls_assert();
		return EOF;
	}

	if (s->cbs_hd) {
		if (s->cbs.write)
			return s->cbs.write(s->cbs_hd, buf, count);
		return 0;
	}

	if (!s->flags.write) {
		s->error = CDK_Inv_Mode;	/* this is a read stream */
		gnutls_assert();
		return EOF;
	}

	if (!buf || !count)
		return stream_flush(s);

	if (s->cache.on) {
#ifdef DEBUG_STREAM
		_gnutls_read_log("stream[ref=%u]: written %d bytes\n",
				 s->fp_ref, (int) count);
#endif

		/* We need to resize the buffer if the additional data wouldn't
		   fit into it. We allocate more memory to avoid to resize it the
		   next time the function is used. */
		if (s->cache.size + count > s->cache.alloced) {
			byte *old = s->cache.buf;

			s->cache.buf =
			    cdk_calloc(1,
				       s->cache.alloced + count +
				       STREAM_BUFSIZE);
			s->cache.alloced += (count + STREAM_BUFSIZE);
			memcpy(s->cache.buf, old, s->cache.size);
			cdk_free(old);
#ifdef DEBUG_STREAM
			_gnutls_read_log
			    ("stream: enlarge cache to %d octets\n",
			     (int) s->cache.alloced);
#endif
		}

		memcpy(s->cache.buf + s->cache.size, buf, count);
		s->cache.size += count;
		return count;
	}
#ifdef DEBUG_STREAM
	_gnutls_read_log("stream[fd=%u]: written %d bytes\n",
			 fileno(s->fp), (int) count);
#endif

	nwritten = fwrite(buf, 1, count, s->fp);
	if (!nwritten)
		nwritten = EOF;
	return nwritten;
}


int cdk_stream_putc(cdk_stream_t s, int c)
{
	byte buf[2];
	int nwritten;

	if (!s) {
		gnutls_assert();
		return EOF;
	}
	buf[0] = c;
	nwritten = cdk_stream_write(s, buf, 1);
	if (nwritten == EOF)
		return EOF;
	return 0;
}


off_t cdk_stream_tell(cdk_stream_t s)
{
	return s ? ftell(s->fp) : (off_t) 0;
}


cdk_error_t cdk_stream_seek(cdk_stream_t s, off_t offset)
{
	off_t len;

	if (!s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	if (s->cbs_hd) {
		if (s->cbs.seek)
			return s->cbs.seek(s->cbs_hd, offset);
		return 0;
	}

	/* Set or reset the EOF flag. */
	len = cdk_stream_get_length(s);
	if (len == offset)
		s->flags.eof = 1;
	else
		s->flags.eof = 0;

	if (fseek(s->fp, offset, SEEK_SET)) {
		gnutls_assert();
		return CDK_File_Error;
	}
	return 0;
}


static cdk_error_t stream_flush(cdk_stream_t s)
{
	assert(s);

	/* For some constellations it cannot be assured that the
	   return value is defined, thus we ignore it for now. */
	(void) fflush(s->fp);
	return 0;
}


/**
 * cdk_stream_set_armor_flag:
 * @s: the stream object
 * @type: the type of armor to use
 * 
 * If the file is in read-mode, no armor type needs to be
 * defined (armor_type=0) because the armor filter will be
 * used for decoding existing armor data.
 * For the write mode, @armor_type can be set to any valid
 * armor type (message, key, sig).
 **/
cdk_error_t cdk_stream_set_armor_flag(cdk_stream_t s, int armor_type)
{
	struct stream_filter_s *f;

	if (!s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
	f = filter_add(s, _cdk_filter_armor, fARMOR);
	if (!f) {
		gnutls_assert();
		return CDK_Out_Of_Core;
	}
	f->u.afx.idx = f->u.afx.idx2 = armor_type;
	f->ctl = stream_get_mode(s);
	return 0;
}


/**
 * cdk_stream_set_literal_flag:
 * @s: the stream object
 * @mode: the mode to use (binary, text, unicode)
 * @fname: the file name to store in the packet.
 *
 * In read mode it kicks off the literal decoding routine to
 * unwrap the data from the packet. The @mode parameter is ignored.
 * In write mode the function can be used to wrap the stream data
 * into a literal packet with the given mode and file name.
 **/
cdk_error_t
cdk_stream_set_literal_flag(cdk_stream_t s, cdk_lit_format_t mode,
			    const char *fname)
{
	struct stream_filter_s *f;
	const char *orig_fname;

#ifdef DEBUG_STREAM
	_gnutls_read_log("stream: enable literal mode.\n");
#endif

	if (!s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	orig_fname = _cdk_stream_get_fname(s);
	f = filter_add(s, _cdk_filter_literal, fLITERAL);
	if (!f) {
		gnutls_assert();
		return CDK_Out_Of_Core;
	}
	f->u.pfx.mode = mode;
	f->u.pfx.filename = fname ? cdk_strdup(fname) : NULL;
	f->u.pfx.orig_filename =
	    orig_fname ? cdk_strdup(orig_fname) : NULL;
	f->ctl = stream_get_mode(s);
	if (s->blkmode > 0) {
		f->u.pfx.blkmode.on = 1;
		f->u.pfx.blkmode.size = s->blkmode;
	}
	return 0;
}


/**
 * cdk_stream_set_compress_flag:
 * @s: the stream object
 * @algo: the compression algo
 * @level: level of compression (0..9)
 * 
 * In read mode it kicks off the decompression filter to retrieve
 * the uncompressed data.
 * In write mode the stream data will be compressed with the
 * given algorithm at the given level.
 **/
cdk_error_t
cdk_stream_set_compress_flag(cdk_stream_t s, int algo, int level)
{

	gnutls_assert();
	return CDK_Not_Implemented;
}


/**
 * cdk_stream_set_text_flag:
 * @s: the stream object
 * @lf: line ending
 * 
 * Pushes the text filter to store the stream data in cannoncial format.
 **/
cdk_error_t cdk_stream_set_text_flag(cdk_stream_t s, const char *lf)
{
	struct stream_filter_s *f;

	if (!s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
	f = filter_add(s, _cdk_filter_text, fTEXT);
	if (!f) {
		gnutls_assert();
		return CDK_Out_Of_Core;
	}
	f->ctl = stream_get_mode(s);
	f->u.tfx.lf = lf;
	return 0;
}

/**
 * cdk_stream_enable_cache:
 * @s: the stream object
 * @val: 1=on, 0=off
 *
 * Enables or disable the cache section of a stream object.
 **/
cdk_error_t cdk_stream_enable_cache(cdk_stream_t s, int val)
{
	if (!s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
	if (!s->flags.write) {
		gnutls_assert();
		return CDK_Inv_Mode;
	}
	s->cache.on = val;
	if (!s->cache.buf) {
		s->cache.buf = cdk_calloc(1, STREAM_BUFSIZE);
		s->cache.alloced = STREAM_BUFSIZE;
#ifdef DEBUG_STREAM
		_gnutls_read_log("stream: allocate cache of %d octets\n",
				 STREAM_BUFSIZE);
#endif
	}
	return 0;
}


static int stream_cache_flush(cdk_stream_t s, FILE * fp)
{
	int nwritten;

	assert(s);

	/* FIXME: We should find a way to use cdk_stream_write here. */
	if (s->cache.size > 0) {
		nwritten = fwrite(s->cache.buf, 1, s->cache.size, fp);
		if (!nwritten) {
			gnutls_assert();
			return CDK_File_Error;
		}
		s->cache.size = 0;
		s->cache.on = 0;
		memset(s->cache.buf, 0, s->cache.alloced);
	}
	return 0;
}


/**
 * cdk_stream_kick_off:
 * @inp: the input stream
 * @out: the output stream.
 * 
 * Passes the entire data from @inp into the output stream @out
 * with all the activated filters.
 */
cdk_error_t cdk_stream_kick_off(cdk_stream_t inp, cdk_stream_t out)
{
	byte buf[BUFSIZE];
	int nread, nwritten;
	cdk_error_t rc;

	if (!inp || !out) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
	rc = CDK_Success;
	while (!cdk_stream_eof(inp)) {
		nread = cdk_stream_read(inp, buf, DIM(buf));
		if (!nread || nread == EOF)
			break;
		nwritten = cdk_stream_write(out, buf, nread);
		if (!nwritten || nwritten == EOF) {	/* In case of errors, we leave the loop. */
			rc = inp->error;
			break;
		}
	}

	memset(buf, 0, sizeof(buf));
	return rc;
}


/**
 * cdk_stream_mmap_part:
 * @s: the stream
 * @off: the offset where to start
 * @len: how much bytes shall be mapped
 * @ret_buf: the buffer to store the content
 * @ret_buflen: length of the buffer
 *
 * Maps the data of the given stream into a memory section. @ret_count
 * contains the length of the buffer.
 **/
cdk_error_t
cdk_stream_mmap_part(cdk_stream_t s, off_t off, size_t len,
		     byte ** ret_buf, size_t * ret_buflen)
{
	cdk_error_t rc;
	off_t oldpos;
	unsigned int n;

	if (!ret_buf || !ret_buflen) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
	*ret_buf = NULL;
	*ret_buflen = 0;

	if (!s) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	/* Memory mapping is not supported on custom I/O objects. */
	if (s->cbs_hd) {
#ifdef DEBUG_STREAM
		_gnutls_read_log
		    ("cdk_stream_mmap_part: not supported on callbacks\n");
#endif
		gnutls_assert();
		return CDK_Inv_Mode;
	}

	oldpos = cdk_stream_tell(s);
	rc = cdk_stream_flush(s);
	if (rc) {
		gnutls_assert();
		return rc;
	}
	rc = cdk_stream_seek(s, off);
	if (rc) {
		gnutls_assert();
		return rc;
	}
	if (!len)
		len = cdk_stream_get_length(s);
	if (!len) {
		_gnutls_read_log
		    ("cdk_stream_mmap_part: invalid file size %lu\n",
		     (unsigned long) len);
		gnutls_assert();
		return s->error;
	}
	if (len > MAX_MAP_SIZE) {
		gnutls_assert();
		return CDK_Too_Short;
	}

	*ret_buf = cdk_calloc(1, len + 1);
	*ret_buflen = len;
	n = cdk_stream_read(s, *ret_buf, len);
	if (n != len)
		*ret_buflen = n;
	rc = cdk_stream_seek(s, oldpos);
	if (rc)
		gnutls_assert();
	return rc;
}


cdk_error_t cdk_stream_mmap(cdk_stream_t inp, byte ** buf, size_t * buflen)
{
	off_t len;

	/* We need to make sure all data is flushed before we retrieve the size. */
	cdk_stream_flush(inp);
	len = cdk_stream_get_length(inp);
	return cdk_stream_mmap_part(inp, 0, len, buf, buflen);
}


/**
 * cdk_stream_peek:
 * @inp: the input stream handle
 * @s: buffer
 * @count: number of bytes to peek
 *
 * The function acts like cdk_stream_read with the difference that
 * the file pointer is moved to the old position after the bytes were read.
 **/
int cdk_stream_peek(cdk_stream_t inp, byte * buf, size_t buflen)
{
	off_t off;
	int nbytes;

	if (!inp || !buf)
		return 0;
	if (inp->cbs_hd)
		return 0;

	off = cdk_stream_tell(inp);
	nbytes = cdk_stream_read(inp, buf, buflen);
	if (nbytes == -1)
		return 0;
	if (cdk_stream_seek(inp, off))
		return 0;
	return nbytes;
}


/* Try to read a line from the given stream. */
int _cdk_stream_gets(cdk_stream_t s, char *buf, size_t count)
{
	int c, i;

	assert(s);

	i = 0;
	while (!cdk_stream_eof(s) && count > 0) {
		c = cdk_stream_getc(s);
		if (c == EOF || c == '\r' || c == '\n') {
			buf[i++] = '\0';
			break;
		}
		buf[i++] = c;
		count--;
	}
	return i;
}


/* Try to write string into the stream @s. */
int _cdk_stream_puts(cdk_stream_t s, const char *buf)
{
	return cdk_stream_write(s, buf, strlen(buf));
}


/* Activate the block mode for the given stream. */
cdk_error_t _cdk_stream_set_blockmode(cdk_stream_t s, size_t nbytes)
{
	assert(s);

#ifdef DEBUG_STREAM
	_gnutls_read_log("stream: activate block mode with blocksize %d\n",
			 (int) nbytes);
#endif
	s->blkmode = nbytes;
	return 0;
}


/* Return the block mode state of the given stream. */
int _cdk_stream_get_blockmode(cdk_stream_t s)
{
	return s ? s->blkmode : 0;
}
