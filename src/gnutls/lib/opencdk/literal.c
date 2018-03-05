/* literal.c - Literal packet filters
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
#include <stdio.h>
#include <opencdk.h>
#include <main.h>
#include <filters.h>
#include "gnutls_int.h"
#include <str.h>


/* Duplicate the string @s but strip of possible
   relative folder names of it. */
static char *dup_trim_filename(const char *s)
{
	char *p = NULL;

	p = strrchr(s, '/');
	if (!p)
		p = strrchr(s, '\\');
	if (!p)
		return cdk_strdup(s);
	return cdk_strdup(p + 1);
}


static cdk_error_t literal_decode(void *data, FILE * in, FILE * out)
{
	literal_filter_t *pfx = data;
	cdk_stream_t si, so;
	cdk_packet_t pkt;
	cdk_pkt_literal_t pt;
	byte buf[BUFSIZE];
	ssize_t nread;
	int bufsize;
	cdk_error_t rc;

	_cdk_log_debug("literal filter: decode\n");

	if (!pfx || !in || !out)
		return CDK_Inv_Value;

	rc = _cdk_stream_fpopen(in, STREAMCTL_READ, &si);
	if (rc)
		return rc;

	cdk_pkt_new(&pkt);
	rc = cdk_pkt_read(si, pkt, 1);
	if (rc || pkt->pkttype != CDK_PKT_LITERAL) {
		cdk_pkt_release(pkt);
		cdk_stream_close(si);
		return !rc ? CDK_Inv_Packet : rc;
	}

	rc = _cdk_stream_fpopen(out, STREAMCTL_WRITE, &so);
	if (rc) {
		cdk_pkt_release(pkt);
		cdk_stream_close(si);
		return rc;
	}

	pt = pkt->pkt.literal;
	pfx->mode = pt->mode;

	if (pfx->filename && pt->namelen > 0) {
		/* The name in the literal packet is more authorative. */
		cdk_free(pfx->filename);
		pfx->filename = dup_trim_filename(pt->name);
	} else if (!pfx->filename && pt->namelen > 0)
		pfx->filename = dup_trim_filename(pt->name);
	else if (!pt->namelen && !pfx->filename && pfx->orig_filename) {
		/* In this case, we need to derrive the output file name
		   from the original name and cut off the OpenPGP extension.
		   If this is not possible, we return an error. */
		if (!stristr(pfx->orig_filename, ".gpg") &&
		    !stristr(pfx->orig_filename, ".pgp") &&
		    !stristr(pfx->orig_filename, ".asc")) {
			cdk_pkt_release(pkt);
			cdk_stream_close(si);
			cdk_stream_close(so);
			_cdk_log_debug
			    ("literal filter: no file name and no PGP extension\n");
			return CDK_Inv_Mode;
		}
		_cdk_log_debug
		    ("literal filter: derrive file name from original\n");
		pfx->filename = dup_trim_filename(pfx->orig_filename);
		pfx->filename[strlen(pfx->filename) - 4] = '\0';
	}

	while (!feof(in)) {
		_cdk_log_debug("literal_decode: part on %d size %lu\n",
			       (int) pfx->blkmode.on,
			       (unsigned long) pfx->blkmode.size);
		if (pfx->blkmode.on)
			bufsize = pfx->blkmode.size;
		else
			bufsize = pt->len < DIM(buf) ? pt->len : DIM(buf);
		nread = cdk_stream_read(pt->buf, buf, bufsize);
		if (nread == EOF) {
			rc = CDK_File_Error;
			break;
		}
		if (pfx->md_initialized)
			_gnutls_hash(&pfx->md, buf, nread);
		cdk_stream_write(so, buf, nread);
		pt->len -= nread;
		if (pfx->blkmode.on) {
			pfx->blkmode.size =
			    _cdk_pkt_read_len(in, &pfx->blkmode.on);
			if ((ssize_t) pfx->blkmode.size == EOF)
				return CDK_Inv_Packet;
		}
		if (pt->len <= 0 && !pfx->blkmode.on)
			break;
	}

	cdk_stream_close(si);
	cdk_stream_close(so);
	cdk_pkt_release(pkt);
	return rc;
}


static char intmode_to_char(int mode)
{
	switch (mode) {
	case CDK_LITFMT_BINARY:
		return 'b';
	case CDK_LITFMT_TEXT:
		return 't';
	case CDK_LITFMT_UNICODE:
		return 'u';
	default:
		return 'b';
	}

	return 'b';
}


static cdk_error_t literal_encode(void *data, FILE * in, FILE * out)
{
	literal_filter_t *pfx = data;
	cdk_pkt_literal_t pt;
	cdk_stream_t si;
	cdk_packet_t pkt;
	size_t filelen;
	cdk_error_t rc;

	_cdk_log_debug("literal filter: encode\n");

	if (!pfx || !in || !out)
		return CDK_Inv_Value;
	if (!pfx->filename) {
		pfx->filename = cdk_strdup("_CONSOLE");
		if (!pfx->filename)
			return CDK_Out_Of_Core;
	}

	rc = _cdk_stream_fpopen(in, STREAMCTL_READ, &si);
	if (rc)
		return rc;

	filelen = strlen(pfx->filename);
	cdk_pkt_new(&pkt);
	pt = pkt->pkt.literal = cdk_calloc(1, sizeof *pt + filelen);
	if (pt == NULL) {
		cdk_pkt_release(pkt);
		cdk_stream_close(si);
		return gnutls_assert_val(CDK_Out_Of_Core);
	}

	pt->name = (char *) pt + sizeof(*pt);

	memcpy(pt->name, pfx->filename, filelen);
	pt->namelen = filelen;
	pt->name[pt->namelen] = '\0';
	pt->timestamp = (u32) gnutls_time(NULL);
	pt->mode = intmode_to_char(pfx->mode);
	pt->len = cdk_stream_get_length(si);
	pt->buf = si;
	pkt->old_ctb = 1;
	pkt->pkttype = CDK_PKT_LITERAL;
	rc = _cdk_pkt_write_fp(out, pkt);

	cdk_pkt_release(pkt);
	cdk_stream_close(si);
	return rc;
}


int _cdk_filter_literal(void *data, int ctl, FILE * in, FILE * out)
{
	if (ctl == STREAMCTL_READ)
		return literal_decode(data, in, out);
	else if (ctl == STREAMCTL_WRITE)
		return literal_encode(data, in, out);
	else if (ctl == STREAMCTL_FREE) {
		literal_filter_t *pfx = data;
		if (pfx) {
			_cdk_log_debug("free literal filter\n");
			cdk_free(pfx->filename);
			pfx->filename = NULL;
			cdk_free(pfx->orig_filename);
			pfx->orig_filename = NULL;
			return 0;
		}
	}
	return CDK_Inv_Mode;
}

/* Remove all trailing white spaces from the string. */
static void _cdk_trim_string(char *s)
{
	int len = strlen(s);
	int i;

	for (i=len-1;i>=0;i--) {
		if ((s[i] == '\t' || s[i] == '\r' || s[i] == '\n' || s[i] == ' ')) {
			s[i] = 0;
		} else {
			break;
		}
	}
}

static int text_encode(void *data, FILE * in, FILE * out)
{
	const char *s;
	char buf[2048];

	if (!in || !out)
		return CDK_Inv_Value;

	/* FIXME: This code does not work for very long lines. */
	while (!feof(in)) {
		/* give space for trim_string \r\n */
		s = fgets(buf, DIM(buf) - 3, in);
		if (!s)
			break;
		_cdk_trim_string(buf);
		_gnutls_str_cat(buf, sizeof(buf), "\r\n");
		fwrite(buf, 1, strlen(buf), out);
	}

	return 0;
}


static int text_decode(void *data, FILE * in, FILE * out)
{
	text_filter_t *tfx = data;
	const char *s;
	char buf[2048];

	if (!tfx || !in || !out)
		return CDK_Inv_Value;

	while (!feof(in)) {
		s = fgets(buf, DIM(buf) - 1, in);
		if (!s)
			break;
		_cdk_trim_string(buf);
		fwrite(buf, 1, strlen(buf), out);
		fwrite(tfx->lf, 1, strlen(tfx->lf), out);
	}

	return 0;
}


int _cdk_filter_text(void *data, int ctl, FILE * in, FILE * out)
{
	if (ctl == STREAMCTL_READ)
		return text_encode(data, in, out);
	else if (ctl == STREAMCTL_WRITE)
		return text_decode(data, in, out);
	else if (ctl == STREAMCTL_FREE) {
		text_filter_t *tfx = data;
		if (tfx) {
			_cdk_log_debug("free text filter\n");
			tfx->lf = NULL;
		}
	}
	return CDK_Inv_Mode;
}
