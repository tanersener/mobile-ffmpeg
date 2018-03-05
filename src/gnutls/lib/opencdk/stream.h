/* stream.h - internal definiton for the STREAM object
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

#ifndef CDK_STREAM_H
#define CDK_STREAM_H

/* The default buffer size for the stream. */
#define STREAM_BUFSIZE 8192

enum {
	fDUMMY = 0,
	fARMOR = 1,
	fCIPHER = 2,
	fLITERAL = 3,
	fCOMPRESS = 4,
	fHASH = 5,
	fTEXT = 6
};

/* Type definition for the filter function. */
typedef int(*filter_fnct_t) (void *uint8_t, int ctl, FILE * in,
				     FILE * out);

/* The stream filter context structure. */
struct stream_filter_s {
	struct stream_filter_s *next;
	filter_fnct_t fnct;
	void *uint8_t;
	FILE *tmp;
	union {
		armor_filter_t afx;
		cipher_filter_t cfx;
		literal_filter_t pfx;
		compress_filter_t zfx;
		text_filter_t tfx;
		md_filter_t mfx;
	} u;
	struct {
		unsigned enabled:1;
		unsigned rdonly:1;
		unsigned error:1;
	} flags;
	unsigned type;
	unsigned ctl;
};


/* The stream context structure. */
struct cdk_stream_s {
	struct stream_filter_s *filters;
	int fmode;
	int error;
	size_t blkmode;
	struct {
		unsigned filtrated:1;
		unsigned eof:1;
		unsigned write:1;
		unsigned temp:1;
		unsigned reset:1;
		unsigned no_filter:1;
		unsigned compressed:3;
	} flags;
	struct {
		unsigned char *buf;
		unsigned on:1;
		size_t size;
		size_t alloced;
	} cache;
	char *fname;
	FILE *fp;
	unsigned int fp_ref:1;
	struct cdk_stream_cbs_s cbs;
	void *cbs_hd;
};

#endif				/* CDK_STREAM_H */
