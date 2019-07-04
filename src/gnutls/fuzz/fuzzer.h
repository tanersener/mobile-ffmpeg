/*
 * Copyright(c) 2017 Free Software Foundation, Inc.
 *
 * GnuTLS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with gnutls.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stddef.h> // size_t
#include <stdint.h> // uint8_t
#include <gnutls/gnutls.h>

#if defined __clang__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
  #pragma GCC diagnostic ignored "-Wvla-larger-than="
#endif

#ifdef __cplusplus
extern "C"
#endif

#define VERS_STR "-VERS-ALL:+VERS-TLS1.3:+VERS-TLS1.2:+VERS-TLS1.1:+VERS-TLS1.0:+VERS-SSL3.0"

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);

static void __attribute__((constructor)) init(void)
{
	gnutls_global_init();
}

static void __attribute__((destructor)) deinit(void)
{
	gnutls_global_deinit();
}
