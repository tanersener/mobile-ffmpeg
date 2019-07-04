/*
 * Copyright (C) 2017 Red Hat, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 */

#include "utils.h"

#define TLS_EXT_SUPPORTED_VERSIONS 43
#define TLS_EXT_POST_HANDSHAKE 49

#define SKIP16(pos, _total) { \
	uint16_t _s; \
	if ((size_t)pos+2 > (size_t)_total) fail("error0: at %d total: %d\n", pos+2, _total); \
	_s = (msg->data[pos] << 8) | msg->data[pos+1]; \
	if ((size_t)(pos+2+_s) > (size_t)_total) fail("error1: at %d field: %d, total: %d\n", pos+2, (int)_s, _total); \
	pos += 2+_s; \
	}

#define SKIP8(pos, _total) { \
	uint8_t _s; \
	if ((size_t)pos+1 > (size_t)_total) fail("error\n"); \
	_s = msg->data[pos]; \
	if ((size_t)(pos+1+_s) > (size_t)_total) fail("error\n"); \
	pos += 1+_s; \
	}

typedef void (*ext_parse_func)(void *priv, gnutls_datum_t *extdata);

#define HANDSHAKE_SESSION_ID_POS 34

#if defined __clang__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-function"
#endif

/* Returns 0 if the extension was not found, 1 otherwise.
 */
static unsigned find_client_extension(const gnutls_datum_t *msg, unsigned extnr, void *priv, ext_parse_func cb)
{
	unsigned pos;

	if (msg->size < HANDSHAKE_SESSION_ID_POS)
		fail("invalid client hello\n");

	/* we expect the legacy version to be present */
	/* ProtocolVersion legacy_version = 0x0303 */
	if (msg->data[0] != 0x03) {
		fail("ProtocolVersion contains %d.%d\n", (int)msg->data[0], (int)msg->data[1]);
	}

	pos = HANDSHAKE_SESSION_ID_POS;
	/* legacy_session_id */
	SKIP8(pos, msg->size);

	/* CipherSuites */
	SKIP16(pos, msg->size);

	/* legacy_compression_methods */
	SKIP8(pos, msg->size);

	pos += 2;

	while (pos < msg->size) {
		uint16_t type;

		if (pos+4 > msg->size)
			fail("invalid client hello\n");

		type = (msg->data[pos] << 8) | msg->data[pos+1];
		pos+=2;

		if (debug)
			success("Found client extension %d\n", (int)type);

		if (type != extnr) {
			SKIP16(pos, msg->size);
		} else { /* found */
			ssize_t size = (msg->data[pos] << 8) | msg->data[pos+1];
			gnutls_datum_t data;

			pos+=2;
			if (pos + size > msg->size) {
				fail("error in extension length (pos: %d, ext: %d, total: %d)\n", pos, (int)size, msg->size);
			}
			data.data = &msg->data[pos];
			data.size = size;
			if (cb)
				cb(priv, &data);
			return 1;
		}
	}
	return 0;
}

static unsigned is_client_extension_last(const gnutls_datum_t *msg, unsigned extnr)
{
	unsigned pos, found = 0;

	if (msg->size < HANDSHAKE_SESSION_ID_POS)
		fail("invalid client hello\n");

	/* we expect the legacy version to be present */
	/* ProtocolVersion legacy_version = 0x0303 */
	if (msg->data[0] != 0x03) {
		fail("ProtocolVersion contains %d.%d\n", (int)msg->data[0], (int)msg->data[1]);
	}

	pos = HANDSHAKE_SESSION_ID_POS;
	/* legacy_session_id */
	SKIP8(pos, msg->size);

	/* CipherSuites */
	SKIP16(pos, msg->size);

	/* legacy_compression_methods */
	SKIP8(pos, msg->size);

	pos += 2;

	while (pos < msg->size) {
		uint16_t type;

		if (pos+4 > msg->size)
			fail("invalid client hello\n");

		type = (msg->data[pos] << 8) | msg->data[pos+1];
		pos+=2;

		if (debug)
			success("Found client extension %d\n", (int)type);

		if (type != extnr) {
			if (found) {
				success("found extension %d after %d\n", type, extnr);
				return 0;
			}
			SKIP16(pos, msg->size);
		} else { /* found */
			found = 1;
			SKIP16(pos, msg->size);
		}
	}

	if (found)
		return 1;
	return 0;
}

#define TLS_RANDOM_SIZE 32

static unsigned find_server_extension(const gnutls_datum_t *msg, unsigned extnr, void *priv, ext_parse_func cb)
{
	unsigned pos = 0;

	success("server hello of %d bytes\n", msg->size);
	/* we expect the legacy version to be present */
	/* ProtocolVersion legacy_version = 0x0303 */
	if (msg->data[0] != 0x03 || msg->data[1] != 0x03) {
		fail("ProtocolVersion contains %d.%d\n", (int)msg->data[0], (int)msg->data[1]);
	}

	if (msg->data[1] >= 0x04) {
		success("assuming TLS 1.3 or better hello format (seen %d.%d)\n", (int)msg->data[0], (int)msg->data[1]);
	}

	pos += 2+TLS_RANDOM_SIZE;

	/* legacy_session_id */
	SKIP8(pos, msg->size);

	/* CipherSuite */
	pos += 2;

	/* legacy_compression_methods */
	SKIP8(pos, msg->size);

	pos += 2;

	while (pos < msg->size) {
		uint16_t type;

		if (pos+4 > msg->size)
			fail("invalid server hello\n");

		type = (msg->data[pos] << 8) | msg->data[pos+1];
		pos+=2;

		success("Found server extension %d\n", (int)type);

		if (type != extnr) {
			SKIP16(pos, msg->size);
		} else { /* found */
			ssize_t size = (msg->data[pos] << 8) | msg->data[pos+1];
			gnutls_datum_t data;

			pos+=2;
			if (pos + size < msg->size) {
				fail("error in server extension length (pos: %d, total: %d)\n", pos, msg->size);
			}
			data.data = &msg->data[pos];
			data.size = size;
			if (cb)
				cb(priv, &data);
			return 1;
		}
	}

	return 0;
}

#if defined __clang__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#  pragma GCC diagnostic pop
#endif
