/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDE_COMMON_H
# define INCLUDE_COMMON_H

#define SERVER "127.0.0.1"

#include <config.h>
#include <gnutls/gnutls.h>
#include <certtool-common.h>
#include <c-ctype.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif

#include <signal.h>
#ifdef _WIN32
#include <io.h>
#include <winbase.h>
#include <sys/select.h>
#undef OCSP_RESPONSE
#endif

#ifndef __attribute__
#if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5)
#define __attribute__(Spec)	/* empty */
#endif
#endif

/* the number of elements in the priority structures.
 */
#define PRI_MAX 16

extern const char str_unknown[];

#define P_PRINT_CERT 1
#define P_WAIT_FOR_CERT (1<<1)
int print_info(gnutls_session_t state, int verbose, int flags);
void print_cert_info(gnutls_session_t, int flag, int print_cert);
void print_cert_info_compact(gnutls_session_t session);

void print_cert_info2(gnutls_session_t, int flag, FILE *fp, int print_cert);

void print_list(const char *priorities, int verbose);
int cert_verify(gnutls_session_t session, const char *hostname, const char *purpose);

const char *raw_to_string(const unsigned char *raw, size_t raw_size);
const char *raw_to_hex(const unsigned char *raw, size_t raw_size);
const char *raw_to_base64(const unsigned char *raw, size_t raw_size);
int check_command(gnutls_session_t session, const char *str);

int
pin_callback(void *user, int attempt, const char *token_url,
	     const char *token_label, unsigned int flags, char *pin,
	     size_t pin_max);

void pkcs11_common(common_info_st *c);

inline static int is_ip(const char *hostname)
{
int len = strlen(hostname);

	if (strchr(hostname, ':') != 0)
		return 1;
	else if (len > 2 && c_isdigit(hostname[0]) && c_isdigit(hostname[len-1]))
		return 1;
	return 0;
}

void sockets_init(void);

#ifdef _WIN32
static int system_recv_timeout(gnutls_transport_ptr_t ptr, unsigned int ms)
{
	fd_set rfds;
	struct timeval tv;
	int ret, fd = (long)ptr;

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	tv.tv_sec = 0;
	tv.tv_usec = ms * 1000;
	while (tv.tv_usec >= 1000000) {
		tv.tv_usec -= 1000000;
		tv.tv_sec++;
	}

	return select(fd + 1, &rfds, NULL, NULL, &tv);
}

static ssize_t
system_write(gnutls_transport_ptr ptr, const void *data, size_t data_size)
{
	return send((long)ptr, data, data_size, 0);
}

static ssize_t
system_read(gnutls_transport_ptr_t ptr, void *data, size_t data_size)
{
	return recv((long)ptr, data, data_size, 0);
}

static
void set_read_funcs(gnutls_session_t session)
{
	gnutls_transport_set_push_function(session, system_write);
	gnutls_transport_set_pull_function(session, system_read);
	gnutls_transport_set_pull_timeout_function(session, system_recv_timeout);
}
#else
# define set_read_funcs(x)
#endif

#endif
