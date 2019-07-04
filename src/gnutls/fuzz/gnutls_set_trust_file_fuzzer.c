/*
# Copyright 2017 Red Hat, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
################################################################################
*/

#include <config.h>

#include <stdint.h>

#include <gnutls/gnutls.h>
#include "fuzzer.h"

static const uint8_t *g_data;
static size_t g_size;

#if ! defined _WIN32 && defined HAVE_FMEMOPEN
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
FILE *fopen(const char *pathname, const char *mode)
{
	FILE *(*libc_fopen)(const char *, const char *) =
		(FILE *(*)(const char *, const char *)) dlsym (RTLD_NEXT, "fopen");

	if (!strcmp(pathname, "ca_or_crl"))
		return fmemopen((void *) g_data, g_size, mode);

	return libc_fopen(pathname, mode);
}
#endif

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	g_data = data;
	g_size = size;

	gnutls_certificate_credentials_t creds;
	gnutls_certificate_allocate_credentials(&creds);
	gnutls_certificate_set_x509_trust_file(creds, "ca_or_crl", GNUTLS_X509_FMT_PEM);
	gnutls_certificate_set_x509_crl_file(creds, "ca_or_crl", GNUTLS_X509_FMT_PEM);
	gnutls_certificate_free_credentials(creds);

	return 0;
}
