/* filters.h - Filter structs
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

#ifndef CDK_FILTERS_H
#define CDK_FILTERS_H

enum {
	STREAMCTL_READ = 0,
	STREAMCTL_WRITE = 1,
	STREAMCTL_FREE = 2
};

typedef struct {
	cipher_hd_st hd;
	digest_hd_st mdc;
	int mdc_method;
	u32 datalen;
	struct {
		size_t on;
		off_t size;
		off_t nleft;
	} blkmode;
	cdk_stream_t s;
} cipher_filter_t;

typedef struct {
	int digest_algo;
	digest_hd_st md;
	int md_initialized;
} md_filter_t;

typedef struct {
	const char *le;		/* line endings */
	const char *hdrlines;
	u32 crc;
	int crc_okay;
	int idx, idx2;
} armor_filter_t;

typedef struct {
	cdk_lit_format_t mode;
	char *orig_filename;	/* This original name of the input file. */
	char *filename;
	digest_hd_st md;
	int md_initialized;
	struct {
		size_t on;
		off_t size;
	} blkmode;
} literal_filter_t;

typedef struct {
	size_t inbufsize;
	byte inbuf[8192];
	size_t outbufsize;
	byte outbuf[8192];
	int algo;		/* compress algo */
	int level;
} compress_filter_t;

typedef struct {
	const char *lf;
} text_filter_t;


/*-- armor.c -*/
int _cdk_filter_armor(void *uint8_t, int ctl, FILE * in, FILE * out);

/*-- cipher.c --*/
cdk_error_t _cdk_filter_hash(void *uint8_t, int ctl, FILE * in,
			     FILE * out);
cdk_error_t _cdk_filter_cipher(void *uint8_t, int ctl, FILE * in,
			       FILE * out);

/*-- literal.c --*/
int _cdk_filter_literal(void *uint8_t, int ctl, FILE * in, FILE * out);
int _cdk_filter_text(void *uint8_t, int ctl, FILE * in, FILE * out);

/*-- compress.c --*/
cdk_error_t _cdk_filter_compress(void *uint8_t, int ctl,
				 FILE * in, FILE * out);

#endif				/* CDK_FILTERS_H */
