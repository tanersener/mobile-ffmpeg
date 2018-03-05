/*
 * Copyright (C) 2014 Red Hat
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
 * You should have received a copy of the GNU General Public License
 * along with GnuTLS; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/dane.h>

#include "utils.h"

#define MAX_QUERIES 8

struct data_entry_st {
	const char *name;
	char *const queries[MAX_QUERIES];
	int q_size[MAX_QUERIES];
	int expected_ret;
	unsigned no_queries;
	int secure;
	int bogus;
	const char *cert;
	const char *ca;
	unsigned expected_status;	/* if cert is non-null */
	int expected_verify_ret;	/* if cert is non-null */
};

const struct data_entry_st data_entries[] = {
	{
	 .name = "Entry parsing",
	 .queries = {
		     (char *)
		     "\x00\x00\x01\x19\x40\x0b\xe5\xb7\xa3\x1f\xb7\x33\x91\x77\x00\x78\x9d\x2f\x0a\x24\x71\xc0\xc9\xd5\x06\xc0\xe5\x04\xc0\x6c\x16\xd7\xcb\x17\xc0",
		     (char *)
		     "\x03\x00\x01\x03\x32\xaa\x2d\x58\xb3\xe0\x54\x4b\x65\x65\x64\x38\x93\x70\x68\xba\x44\xce\x2f\x14\x46\x9c\x4f\x50\xc9\xcc\x69\x33\xc8\x08\xd3",
		     (char *)
		     "\x03\x01\x01\x46\x25\x73\x19\x5c\x86\xe8\x61\xab\xab\x8e\xcc\xfb\xc7\xf0\x48\x69\x58\xef\xdf\xf9\x44\x9a\xc1\x07\x29\xb3\xa0\xf9\x06\xf3\x88",
		     NULL},
	 .q_size = {35, 35, 35, 0},
	 .expected_ret = 0,
	 .no_queries = 3,
	 .secure = 1,
	 .bogus = 0},
	{			/* as the previous but with first byte invalid */
	 .name = "Cert verification (single entry)",
	 .queries = {
		     (char *)
		     "\x03\x01\x01\x54\x4f\x28\x4d\x66\xaf\x2d\xe0\x8c\x17\xe7\x48\x6a\xed\xfa\x2e\x00\xaa\x1a\xc6\xbb\xf3\xaf\x5c\xa6\x2b\x55\xab\x7a\xc2\x69\xbe",
		     NULL},
	 .q_size = {35, 35, 35, 0},
	 .expected_ret = 0,
	 .no_queries = 1,
	 .secure = 1,
	 .bogus = 0,
	 .expected_verify_ret = 0,
	 .expected_status = 0,
	 .cert = "-----BEGIN CERTIFICATE-----\n"
	 "MIIE+DCCA+CgAwIBAgISESHVV5p9ybDcuT+A7ITU5IQYMA0GCSqGSIb3DQEBCwUA\n"
	 "MGAxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9iYWxTaWduIG52LXNhMTYwNAYD\n"
	 "VQQDEy1HbG9iYWxTaWduIERvbWFpbiBWYWxpZGF0aW9uIENBIC0gU0hBMjU2IC0g\n"
	 "RzIwHhcNMTUxMDIxMDkxOTAwWhcNMTYxMjE4MTY1NDU2WjA8MSEwHwYDVQQLExhE\n"
	 "b21haW4gQ29udHJvbCBWYWxpZGF0ZWQxFzAVBgNVBAMMDioubmxuZXRsYWJzLm5s\n"
	 "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzHpmwd7SC6vLKde0IcYD\n"
	 "rrVcSSZFNrmP6Wtw5rR7KTBGfj70lCzo1Tu4KzOeFL23cB/Y8kWPojw73eYM+lnr\n"
	 "woZmdG28q+nYeZYRNjFpeLmwK87bpWxw760FrdQSdPrgM9uZS02AWD8PWIWZQ+0X\n"
	 "5XbkgSSjgSRAeT6Ki+8r9TcA+rgUv208kHVgFrBqeNQ//oRojN/7tBbbXrVTy37W\n"
	 "yWLCijExfBzQSsamZqskwhmzYyCJOXCqHUGh/Nyt9WvcX4YE7ogba33M7EQX2C37\n"
	 "ZH+XcmHGdhhLahuMoAm39mchN8TwY7R6DtmvM/WhDdc4dkEWjvrUnGYQhajsKVIZ\n"
	 "oQIDAQABo4IBzjCCAcowDgYDVR0PAQH/BAQDAgWgMEkGA1UdIARCMEAwPgYGZ4EM\n"
	 "AQIBMDQwMgYIKwYBBQUHAgEWJmh0dHBzOi8vd3d3Lmdsb2JhbHNpZ24uY29tL3Jl\n"
	 "cG9zaXRvcnkvMCcGA1UdEQQgMB6CDioubmxuZXRsYWJzLm5sggxubG5ldGxhYnMu\n"
	 "bmwwCQYDVR0TBAIwADAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwQwYD\n"
	 "VR0fBDwwOjA4oDagNIYyaHR0cDovL2NybC5nbG9iYWxzaWduLmNvbS9ncy9nc2Rv\n"
	 "bWFpbnZhbHNoYTJnMi5jcmwwgZQGCCsGAQUFBwEBBIGHMIGEMEcGCCsGAQUFBzAC\n"
	 "hjtodHRwOi8vc2VjdXJlLmdsb2JhbHNpZ24uY29tL2NhY2VydC9nc2RvbWFpbnZh\n"
	 "bHNoYTJnMnIxLmNydDA5BggrBgEFBQcwAYYtaHR0cDovL29jc3AyLmdsb2JhbHNp\n"
	 "Z24uY29tL2dzZG9tYWludmFsc2hhMmcyMB0GA1UdDgQWBBR8k4wtqr2L7in153sI\n"
	 "aE9Eo+ZB5zAfBgNVHSMEGDAWgBTqTnzUgC3lFYGGJoyCbcCYpM+XDzANBgkqhkiG\n"
	 "9w0BAQsFAAOCAQEAHgjG+iHJ8INGp/J0VskjmMItSdcTJhsQbAf1Pz1eu87cXhFa\n"
	 "Vro1xRN9KcsKhnd6TbflDpZkM0g9kX1nGZUWLxMmDbx6N/Y+0X9XHBkgTcVgo1gn\n"
	 "DkzBfMq/Qmy6Szl+RqNinvM2VjkjreWP2AFmIvbZxjMQDAtSs+5l1Qd+xR3Qxrim\n"
	 "5XFIaS7lR8ediLKO0trf7TcbXYZ72u3pxVxm7y2Vzi4mC+lcEcc6409b1yeSRbx/\n"
	 "9N6pYa8Uk3ZaeR6hZHx/g448vVwAqmKrsyJZOayDwHxrFeFWPfJSrFlT8kLmkr5A\n"
	 "VKOWjR5fslCGWqONiFHhyujZocIw03v5+kD9lw==\n"
	 "-----END CERTIFICATE-----\n"},
	{
	 .name = "Cert verification (multi entries)",
	 .queries = {
		     (char *)
		     "\x00\x00\x01\x19\x40\x0b\xe5\xb7\xa3\x1f\xb7\x33\x91\x77\x00\x78\x9d\x2f\x0a\x24\x71\xc0\xc9\xd5\x06\xc0\xe5\x04\xc0\x6c\x16\xd7\xcb\x17\xc0",
		     (char *)
		     "\x03\x01\x01\x54\x4f\x28\x4d\x66\xaf\x2d\xe0\x8c\x17\xe7\x48\x6a\xed\xfa\x2e\x00\xaa\x1a\xc6\xbb\xf3\xaf\x5c\xa6\x2b\x55\xab\x7a\xc2\x69\xbe",
		     (char *)
		     "\x03\x00\x01\x03\x32\xaa\x2d\x58\xb3\xe0\x54\x4b\x65\x65\x64\x38\x93\x70\x68\xba\x44\xce\x2f\x14\x46\x9c\x4f\x50\xc9\xcc\x69\x33\xc8\x08\xd3",
		     NULL},
	 .q_size = { 35, 35, 35, 0},
	 .expected_ret = 0,
	 .no_queries = 3,
	 .secure = 1,
	 .bogus = 0,
	 .expected_verify_ret = 0,
	 .expected_status = 0,
	 .cert = "-----BEGIN CERTIFICATE-----\n"
	 "MIIE+DCCA+CgAwIBAgISESHVV5p9ybDcuT+A7ITU5IQYMA0GCSqGSIb3DQEBCwUA\n"
	 "MGAxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9iYWxTaWduIG52LXNhMTYwNAYD\n"
	 "VQQDEy1HbG9iYWxTaWduIERvbWFpbiBWYWxpZGF0aW9uIENBIC0gU0hBMjU2IC0g\n"
	 "RzIwHhcNMTUxMDIxMDkxOTAwWhcNMTYxMjE4MTY1NDU2WjA8MSEwHwYDVQQLExhE\n"
	 "b21haW4gQ29udHJvbCBWYWxpZGF0ZWQxFzAVBgNVBAMMDioubmxuZXRsYWJzLm5s\n"
	 "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzHpmwd7SC6vLKde0IcYD\n"
	 "rrVcSSZFNrmP6Wtw5rR7KTBGfj70lCzo1Tu4KzOeFL23cB/Y8kWPojw73eYM+lnr\n"
	 "woZmdG28q+nYeZYRNjFpeLmwK87bpWxw760FrdQSdPrgM9uZS02AWD8PWIWZQ+0X\n"
	 "5XbkgSSjgSRAeT6Ki+8r9TcA+rgUv208kHVgFrBqeNQ//oRojN/7tBbbXrVTy37W\n"
	 "yWLCijExfBzQSsamZqskwhmzYyCJOXCqHUGh/Nyt9WvcX4YE7ogba33M7EQX2C37\n"
	 "ZH+XcmHGdhhLahuMoAm39mchN8TwY7R6DtmvM/WhDdc4dkEWjvrUnGYQhajsKVIZ\n"
	 "oQIDAQABo4IBzjCCAcowDgYDVR0PAQH/BAQDAgWgMEkGA1UdIARCMEAwPgYGZ4EM\n"
	 "AQIBMDQwMgYIKwYBBQUHAgEWJmh0dHBzOi8vd3d3Lmdsb2JhbHNpZ24uY29tL3Jl\n"
	 "cG9zaXRvcnkvMCcGA1UdEQQgMB6CDioubmxuZXRsYWJzLm5sggxubG5ldGxhYnMu\n"
	 "bmwwCQYDVR0TBAIwADAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwQwYD\n"
	 "VR0fBDwwOjA4oDagNIYyaHR0cDovL2NybC5nbG9iYWxzaWduLmNvbS9ncy9nc2Rv\n"
	 "bWFpbnZhbHNoYTJnMi5jcmwwgZQGCCsGAQUFBwEBBIGHMIGEMEcGCCsGAQUFBzAC\n"
	 "hjtodHRwOi8vc2VjdXJlLmdsb2JhbHNpZ24uY29tL2NhY2VydC9nc2RvbWFpbnZh\n"
	 "bHNoYTJnMnIxLmNydDA5BggrBgEFBQcwAYYtaHR0cDovL29jc3AyLmdsb2JhbHNp\n"
	 "Z24uY29tL2dzZG9tYWludmFsc2hhMmcyMB0GA1UdDgQWBBR8k4wtqr2L7in153sI\n"
	 "aE9Eo+ZB5zAfBgNVHSMEGDAWgBTqTnzUgC3lFYGGJoyCbcCYpM+XDzANBgkqhkiG\n"
	 "9w0BAQsFAAOCAQEAHgjG+iHJ8INGp/J0VskjmMItSdcTJhsQbAf1Pz1eu87cXhFa\n"
	 "Vro1xRN9KcsKhnd6TbflDpZkM0g9kX1nGZUWLxMmDbx6N/Y+0X9XHBkgTcVgo1gn\n"
	 "DkzBfMq/Qmy6Szl+RqNinvM2VjkjreWP2AFmIvbZxjMQDAtSs+5l1Qd+xR3Qxrim\n"
	 "5XFIaS7lR8ediLKO0trf7TcbXYZ72u3pxVxm7y2Vzi4mC+lcEcc6409b1yeSRbx/\n"
	 "9N6pYa8Uk3ZaeR6hZHx/g448vVwAqmKrsyJZOayDwHxrFeFWPfJSrFlT8kLmkr5A\n"
	 "VKOWjR5fslCGWqONiFHhyujZocIw03v5+kD9lw==\n"
	 "-----END CERTIFICATE-----\n"},
	{
	 .name = "Cert verification (invalid hash)",
	 .queries = {
		     (char *)
		     "\x03\x01\x01\x54\x4f\x28\x4d\x66\xaf\x2d\xe0\x8c\x17\xe7\x49\x6a\xed\xfa\x2e\x00\xaa\x1a\xc6\xbb\xf3\xaf\x5c\xa6\x2b\x55\xab\x7a\xc2\x69\xbe",
		     NULL},
	 .q_size = { 35, 0},
	 .expected_ret = 0,
	 .no_queries = 1,
	 .secure = 1,
	 .bogus = 0,
	 .expected_verify_ret = 0,
	 .expected_status = DANE_VERIFY_CERT_DIFFERS,
	 .cert = "-----BEGIN CERTIFICATE-----\n"
	 "MIIE+DCCA+CgAwIBAgISESHVV5p9ybDcuT+A7ITU5IQYMA0GCSqGSIb3DQEBCwUA\n"
	 "MGAxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9iYWxTaWduIG52LXNhMTYwNAYD\n"
	 "VQQDEy1HbG9iYWxTaWduIERvbWFpbiBWYWxpZGF0aW9uIENBIC0gU0hBMjU2IC0g\n"
	 "RzIwHhcNMTUxMDIxMDkxOTAwWhcNMTYxMjE4MTY1NDU2WjA8MSEwHwYDVQQLExhE\n"
	 "b21haW4gQ29udHJvbCBWYWxpZGF0ZWQxFzAVBgNVBAMMDioubmxuZXRsYWJzLm5s\n"
	 "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzHpmwd7SC6vLKde0IcYD\n"
	 "rrVcSSZFNrmP6Wtw5rR7KTBGfj70lCzo1Tu4KzOeFL23cB/Y8kWPojw73eYM+lnr\n"
	 "woZmdG28q+nYeZYRNjFpeLmwK87bpWxw760FrdQSdPrgM9uZS02AWD8PWIWZQ+0X\n"
	 "5XbkgSSjgSRAeT6Ki+8r9TcA+rgUv208kHVgFrBqeNQ//oRojN/7tBbbXrVTy37W\n"
	 "yWLCijExfBzQSsamZqskwhmzYyCJOXCqHUGh/Nyt9WvcX4YE7ogba33M7EQX2C37\n"
	 "ZH+XcmHGdhhLahuMoAm39mchN8TwY7R6DtmvM/WhDdc4dkEWjvrUnGYQhajsKVIZ\n"
	 "oQIDAQABo4IBzjCCAcowDgYDVR0PAQH/BAQDAgWgMEkGA1UdIARCMEAwPgYGZ4EM\n"
	 "AQIBMDQwMgYIKwYBBQUHAgEWJmh0dHBzOi8vd3d3Lmdsb2JhbHNpZ24uY29tL3Jl\n"
	 "cG9zaXRvcnkvMCcGA1UdEQQgMB6CDioubmxuZXRsYWJzLm5sggxubG5ldGxhYnMu\n"
	 "bmwwCQYDVR0TBAIwADAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwQwYD\n"
	 "VR0fBDwwOjA4oDagNIYyaHR0cDovL2NybC5nbG9iYWxzaWduLmNvbS9ncy9nc2Rv\n"
	 "bWFpbnZhbHNoYTJnMi5jcmwwgZQGCCsGAQUFBwEBBIGHMIGEMEcGCCsGAQUFBzAC\n"
	 "hjtodHRwOi8vc2VjdXJlLmdsb2JhbHNpZ24uY29tL2NhY2VydC9nc2RvbWFpbnZh\n"
	 "bHNoYTJnMnIxLmNydDA5BggrBgEFBQcwAYYtaHR0cDovL29jc3AyLmdsb2JhbHNp\n"
	 "Z24uY29tL2dzZG9tYWludmFsc2hhMmcyMB0GA1UdDgQWBBR8k4wtqr2L7in153sI\n"
	 "aE9Eo+ZB5zAfBgNVHSMEGDAWgBTqTnzUgC3lFYGGJoyCbcCYpM+XDzANBgkqhkiG\n"
	 "9w0BAQsFAAOCAQEAHgjG+iHJ8INGp/J0VskjmMItSdcTJhsQbAf1Pz1eu87cXhFa\n"
	 "Vro1xRN9KcsKhnd6TbflDpZkM0g9kX1nGZUWLxMmDbx6N/Y+0X9XHBkgTcVgo1gn\n"
	 "DkzBfMq/Qmy6Szl+RqNinvM2VjkjreWP2AFmIvbZxjMQDAtSs+5l1Qd+xR3Qxrim\n"
	 "5XFIaS7lR8ediLKO0trf7TcbXYZ72u3pxVxm7y2Vzi4mC+lcEcc6409b1yeSRbx/\n"
	 "9N6pYa8Uk3ZaeR6hZHx/g448vVwAqmKrsyJZOayDwHxrFeFWPfJSrFlT8kLmkr5A\n"
	 "VKOWjR5fslCGWqONiFHhyujZocIw03v5+kD9lw==\n"
	 "-----END CERTIFICATE-----\n"},
	{
	 .name = "Cert verification (bogus data)",
	 .queries = {
		     (char *)
		     "\x00\x00\x01\x19\x40\x0b\xe5\xb7\xa3\x1f\xb7\x33\x91\x77\x00\x78\x9d\x2f\x0a\x24\x71\xc0\xc9\xd5\x06\xc0\xe5\x04\xc0\x6c\x16\xd7\xcb\x17\xc0",
		     NULL},
	 .q_size = { 35, 0},
	 .expected_ret = 0,
	 .no_queries = 1,
	 .secure = 1,
	 .bogus = 0,
	 .expected_verify_ret = DANE_E_REQUESTED_DATA_NOT_AVAILABLE,
	 .expected_status = -1,
	 .cert = "-----BEGIN CERTIFICATE-----\n"
	 "MIIE+DCCA+CgAwIBAgISESHVV5p9ybDcuT+A7ITU5IQYMA0GCSqGSIb3DQEBCwUA\n"
	 "MGAxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9iYWxTaWduIG52LXNhMTYwNAYD\n"
	 "VQQDEy1HbG9iYWxTaWduIERvbWFpbiBWYWxpZGF0aW9uIENBIC0gU0hBMjU2IC0g\n"
	 "RzIwHhcNMTUxMDIxMDkxOTAwWhcNMTYxMjE4MTY1NDU2WjA8MSEwHwYDVQQLExhE\n"
	 "b21haW4gQ29udHJvbCBWYWxpZGF0ZWQxFzAVBgNVBAMMDioubmxuZXRsYWJzLm5s\n"
	 "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAzHpmwd7SC6vLKde0IcYD\n"
	 "rrVcSSZFNrmP6Wtw5rR7KTBGfj70lCzo1Tu4KzOeFL23cB/Y8kWPojw73eYM+lnr\n"
	 "woZmdG28q+nYeZYRNjFpeLmwK87bpWxw760FrdQSdPrgM9uZS02AWD8PWIWZQ+0X\n"
	 "5XbkgSSjgSRAeT6Ki+8r9TcA+rgUv208kHVgFrBqeNQ//oRojN/7tBbbXrVTy37W\n"
	 "yWLCijExfBzQSsamZqskwhmzYyCJOXCqHUGh/Nyt9WvcX4YE7ogba33M7EQX2C37\n"
	 "ZH+XcmHGdhhLahuMoAm39mchN8TwY7R6DtmvM/WhDdc4dkEWjvrUnGYQhajsKVIZ\n"
	 "oQIDAQABo4IBzjCCAcowDgYDVR0PAQH/BAQDAgWgMEkGA1UdIARCMEAwPgYGZ4EM\n"
	 "AQIBMDQwMgYIKwYBBQUHAgEWJmh0dHBzOi8vd3d3Lmdsb2JhbHNpZ24uY29tL3Jl\n"
	 "cG9zaXRvcnkvMCcGA1UdEQQgMB6CDioubmxuZXRsYWJzLm5sggxubG5ldGxhYnMu\n"
	 "bmwwCQYDVR0TBAIwADAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwQwYD\n"
	 "VR0fBDwwOjA4oDagNIYyaHR0cDovL2NybC5nbG9iYWxzaWduLmNvbS9ncy9nc2Rv\n"
	 "bWFpbnZhbHNoYTJnMi5jcmwwgZQGCCsGAQUFBwEBBIGHMIGEMEcGCCsGAQUFBzAC\n"
	 "hjtodHRwOi8vc2VjdXJlLmdsb2JhbHNpZ24uY29tL2NhY2VydC9nc2RvbWFpbnZh\n"
	 "bHNoYTJnMnIxLmNydDA5BggrBgEFBQcwAYYtaHR0cDovL29jc3AyLmdsb2JhbHNp\n"
	 "Z24uY29tL2dzZG9tYWludmFsc2hhMmcyMB0GA1UdDgQWBBR8k4wtqr2L7in153sI\n"
	 "aE9Eo+ZB5zAfBgNVHSMEGDAWgBTqTnzUgC3lFYGGJoyCbcCYpM+XDzANBgkqhkiG\n"
	 "9w0BAQsFAAOCAQEAHgjG+iHJ8INGp/J0VskjmMItSdcTJhsQbAf1Pz1eu87cXhFa\n"
	 "Vro1xRN9KcsKhnd6TbflDpZkM0g9kX1nGZUWLxMmDbx6N/Y+0X9XHBkgTcVgo1gn\n"
	 "DkzBfMq/Qmy6Szl+RqNinvM2VjkjreWP2AFmIvbZxjMQDAtSs+5l1Qd+xR3Qxrim\n"
	 "5XFIaS7lR8ediLKO0trf7TcbXYZ72u3pxVxm7y2Vzi4mC+lcEcc6409b1yeSRbx/\n"
	 "9N6pYa8Uk3ZaeR6hZHx/g448vVwAqmKrsyJZOayDwHxrFeFWPfJSrFlT8kLmkr5A\n"
	 "VKOWjR5fslCGWqONiFHhyujZocIw03v5+kD9lw==\n"
	 "-----END CERTIFICATE-----\n"},
	{
	 .name = "CA verification (valid)",
	 .queries = {
		     (char *)
		     "\x00\x00\x01\x19\x40\x0b\xe5\xb7\xa3\x1f\xb7\x33\x91\x77\x00\x78\x9d\x2f\x0a\x24\x71\xc0\xc9\xd5\x06\xc0\xe5\x04\xc0\x6c\x16\xd7\xcb\x17\xc0",
		     NULL},
	 .q_size = { 35, 0},
	 .expected_ret = 0,
	 .no_queries = 1,
	 .secure = 1,
	 .bogus = 0,
	 .expected_verify_ret = 0,
	 .expected_status = 0,
	 .cert = "-----BEGIN CERTIFICATE-----\n"
	 "MIIGXjCCBUagAwIBAgIQBNO3A71kyzonos0JsLRHrjANBgkqhkiG9w0BAQsFADBw\n"
	 "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
	 "d3cuZGlnaWNlcnQuY29tMS8wLQYDVQQDEyZEaWdpQ2VydCBTSEEyIEhpZ2ggQXNz\n"
	 "dXJhbmNlIFNlcnZlciBDQTAeFw0xNDA0MjIwMDAwMDBaFw0xNzA0MjYxMjAwMDBa\n"
	 "MG0xCzAJBgNVBAYTAlVTMRcwFQYDVQQIEw5Ob3J0aCBDYXJvbGluYTEQMA4GA1UE\n"
	 "BxMHUmFsZWlnaDEVMBMGA1UEChMMUmVkIEhhdCBJbmMuMRwwGgYDVQQDDBMqLmZl\n"
	 "ZG9yYXByb2plY3Qub3JnMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA\n"
	 "vpd22JvToGSgnx2CBtfdoqvraQWNpQ1aXd/PSM0PVIqEjutrKJF7Xhr7DgHRLOhy\n"
	 "ko1CzfSp5n6nB7raqDq2kddWgqL3tuyb+lSwdQRGuJZsWW3CCwzR3VNRQUnPldpd\n"
	 "vqHVSkjHkIZYVcZ2FnMYWEa43ESnmgiQGBg4G+T7/9Pv+10SQ+fOE175GWZKHkJm\n"
	 "vJZAjIO2uxvJ/rCq3YQI6hdAsclIiSZ4X8UXWt0IMjp/RdCCnv+SS4XCirZ/IDqM\n"
	 "H+WdMllD0/cbgIOr4SXEuUPEJcI5NziuILe05RefFeZXoC6dxNWr8BvAjxxrZtpS\n"
	 "/7OMwE+WYkVIH8fkgCTVfsa2ZOvMM5CWzxqWKhbFsbw6EGSVIIUtI3C28i3rjLjr\n"
	 "XZ/94k3pf3i/u6DzUmlWm8psn6XZXru0+FKPTrmeDluyuxJsgzudk8mF8Cjw/Oc0\n"
	 "IHVg6Qw/Dm/OM9cAVqmb6ld3GF+QlkzTwurEGKeGj8s8Td0WoPOf6apB/PIaDIu1\n"
	 "rJphTVyGNqfKqMFFOwqH/M9CVtaEfwYqT9aB8OSE8MtFe3L1WypEq4tK8VUtoi98\n"
	 "0S9mz4fxathakM+js1eyup/uz0W4cKIFbONLgod0g1arMmSB1Ox7GD6qaUC6zKr8\n"
	 "hWcKMROSg8VFYMhqwGR2k64knXDsVH1mAOgRbJabr3ECAwEAAaOCAfUwggHxMB8G\n"
	 "A1UdIwQYMBaAFFFo/5CvAgd1PMzZZWRiohK4WXI7MB0GA1UdDgQWBBRaTFeTslW8\n"
	 "sjOiEWQkQoHtHefJIjAxBgNVHREEKjAoghMqLmZlZG9yYXByb2plY3Qub3JnghFm\n"
	 "ZWRvcmFwcm9qZWN0Lm9yZzAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYB\n"
	 "BQUHAwEGCCsGAQUFBwMCMHUGA1UdHwRuMGwwNKAyoDCGLmh0dHA6Ly9jcmwzLmRp\n"
	 "Z2ljZXJ0LmNvbS9zaGEyLWhhLXNlcnZlci1nMi5jcmwwNKAyoDCGLmh0dHA6Ly9j\n"
	 "cmw0LmRpZ2ljZXJ0LmNvbS9zaGEyLWhhLXNlcnZlci1nMi5jcmwwQgYDVR0gBDsw\n"
	 "OTA3BglghkgBhv1sAQEwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNl\n"
	 "cnQuY29tL0NQUzCBgwYIKwYBBQUHAQEEdzB1MCQGCCsGAQUFBzABhhhodHRwOi8v\n"
	 "b2NzcC5kaWdpY2VydC5jb20wTQYIKwYBBQUHMAKGQWh0dHA6Ly9jYWNlcnRzLmRp\n"
	 "Z2ljZXJ0LmNvbS9EaWdpQ2VydFNIQTJIaWdoQXNzdXJhbmNlU2VydmVyQ0EuY3J0\n"
	 "MAwGA1UdEwEB/wQCMAAwDQYJKoZIhvcNAQELBQADggEBADSBIYR5GwUfYTHlXeej\n"
	 "tgOMbGIiBD1YPBNlP7vLiGc9+Z4rUxWy/TkL7WUFJf1L88ph1CUQ8TbRjLz2RqL8\n"
	 "snkFWjMsH9ddnwTO4zkCtTjC9fu+broPkmvzmHq2hlXuiDz9G7XvjtbtPujrrKOz\n"
	 "o1pPAEl5c4B0ANaYL0OMUDhvskJguVMC5S/ZNuvNg6k3jkKZWGZPfcxgcZoPvBM8\n"
	 "oIjImGyUMpy7bqRPp4K2xoN530GjoXg8OWIvyAwA06ENLZrU1fcSJsvH2gZVzk8s\n"
	 "EvqFNFnOJN3aQ21imUjAesJ9dXSeCpscDDHqwzmRPuj2/QgtpMCmSZf34mdEzDIJ\n"
	 "hrA=\n" "-----END CERTIFICATE-----\n",
	 .ca = "-----BEGIN CERTIFICATE-----\n"
	 "MIIEsTCCA5mgAwIBAgIQBOHnpNxc8vNtwCtCuF0VnzANBgkqhkiG9w0BAQsFADBs\n"
	 "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
	 "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
	 "ZSBFViBSb290IENBMB4XDTEzMTAyMjEyMDAwMFoXDTI4MTAyMjEyMDAwMFowcDEL\n"
	 "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
	 "LmRpZ2ljZXJ0LmNvbTEvMC0GA1UEAxMmRGlnaUNlcnQgU0hBMiBIaWdoIEFzc3Vy\n"
	 "YW5jZSBTZXJ2ZXIgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC2\n"
	 "4C/CJAbIbQRf1+8KZAayfSImZRauQkCbztyfn3YHPsMwVYcZuU+UDlqUH1VWtMIC\n"
	 "Kq/QmO4LQNfE0DtyyBSe75CxEamu0si4QzrZCwvV1ZX1QK/IHe1NnF9Xt4ZQaJn1\n"
	 "itrSxwUfqJfJ3KSxgoQtxq2lnMcZgqaFD15EWCo3j/018QsIJzJa9buLnqS9UdAn\n"
	 "4t07QjOjBSjEuyjMmqwrIw14xnvmXnG3Sj4I+4G3FhahnSMSTeXXkgisdaScus0X\n"
	 "sh5ENWV/UyU50RwKmmMbGZJ0aAo3wsJSSMs5WqK24V3B3aAguCGikyZvFEohQcft\n"
	 "bZvySC/zA/WiaJJTL17jAgMBAAGjggFJMIIBRTASBgNVHRMBAf8ECDAGAQH/AgEA\n"
	 "MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw\n"
	 "NAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2Vy\n"
	 "dC5jb20wSwYDVR0fBEQwQjBAoD6gPIY6aHR0cDovL2NybDQuZGlnaWNlcnQuY29t\n"
	 "L0RpZ2lDZXJ0SGlnaEFzc3VyYW5jZUVWUm9vdENBLmNybDA9BgNVHSAENjA0MDIG\n"
	 "BFUdIAAwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQuY29tL0NQ\n"
	 "UzAdBgNVHQ4EFgQUUWj/kK8CB3U8zNllZGKiErhZcjswHwYDVR0jBBgwFoAUsT7D\n"
	 "aQP4v0cB1JgmGggC72NkK8MwDQYJKoZIhvcNAQELBQADggEBABiKlYkD5m3fXPwd\n"
	 "aOpKj4PWUS+Na0QWnqxj9dJubISZi6qBcYRb7TROsLd5kinMLYBq8I4g4Xmk/gNH\n"
	 "E+r1hspZcX30BJZr01lYPf7TMSVcGDiEo+afgv2MW5gxTs14nhr9hctJqvIni5ly\n"
	 "/D6q1UEL2tU2ob8cbkdJf17ZSHwD2f2LSaCYJkJA69aSEaRkCldUxPUd1gJea6zu\n"
	 "xICaEnL6VpPX/78whQYwvwt/Tv9XBZ0k7YXDK/umdaisLRbvfXknsuvCnQsH6qqF\n"
	 "0wGjIChBWUMo0oHjqvbsezt3tkBigAVBRQHvFwY+3sAzm2fTYS5yh+Rp/BIAV0Ae\n"
	 "cPUeybQ=\n" "-----END CERTIFICATE-----\n"},
	{
	 .name = "CA verification (invalid)",
	 .queries = {
		     (char *)
		     "\x00\x00\x01\x19\x40\x0b\xe5\xb7\xa3\x1f\xb7\x33\x92\x77\x00\x78\x9d\x2f\x0a\x24\x71\xc0\xc9\xd5\x06\xc0\xe5\x04\xc0\x6c\x16\xd7\xcb\x17\xc0",
		     NULL},
	 .q_size = { 35, 0},
	 .expected_ret = 0,
	 .no_queries = 1,
	 .secure = 1,
	 .bogus = 0,
	 .expected_verify_ret = 0,
	 .expected_status = DANE_VERIFY_CA_CONSTRAINTS_VIOLATED,
	 .cert = "-----BEGIN CERTIFICATE-----\n"
	 "MIIGXjCCBUagAwIBAgIQBNO3A71kyzonos0JsLRHrjANBgkqhkiG9w0BAQsFADBw\n"
	 "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
	 "d3cuZGlnaWNlcnQuY29tMS8wLQYDVQQDEyZEaWdpQ2VydCBTSEEyIEhpZ2ggQXNz\n"
	 "dXJhbmNlIFNlcnZlciBDQTAeFw0xNDA0MjIwMDAwMDBaFw0xNzA0MjYxMjAwMDBa\n"
	 "MG0xCzAJBgNVBAYTAlVTMRcwFQYDVQQIEw5Ob3J0aCBDYXJvbGluYTEQMA4GA1UE\n"
	 "BxMHUmFsZWlnaDEVMBMGA1UEChMMUmVkIEhhdCBJbmMuMRwwGgYDVQQDDBMqLmZl\n"
	 "ZG9yYXByb2plY3Qub3JnMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA\n"
	 "vpd22JvToGSgnx2CBtfdoqvraQWNpQ1aXd/PSM0PVIqEjutrKJF7Xhr7DgHRLOhy\n"
	 "ko1CzfSp5n6nB7raqDq2kddWgqL3tuyb+lSwdQRGuJZsWW3CCwzR3VNRQUnPldpd\n"
	 "vqHVSkjHkIZYVcZ2FnMYWEa43ESnmgiQGBg4G+T7/9Pv+10SQ+fOE175GWZKHkJm\n"
	 "vJZAjIO2uxvJ/rCq3YQI6hdAsclIiSZ4X8UXWt0IMjp/RdCCnv+SS4XCirZ/IDqM\n"
	 "H+WdMllD0/cbgIOr4SXEuUPEJcI5NziuILe05RefFeZXoC6dxNWr8BvAjxxrZtpS\n"
	 "/7OMwE+WYkVIH8fkgCTVfsa2ZOvMM5CWzxqWKhbFsbw6EGSVIIUtI3C28i3rjLjr\n"
	 "XZ/94k3pf3i/u6DzUmlWm8psn6XZXru0+FKPTrmeDluyuxJsgzudk8mF8Cjw/Oc0\n"
	 "IHVg6Qw/Dm/OM9cAVqmb6ld3GF+QlkzTwurEGKeGj8s8Td0WoPOf6apB/PIaDIu1\n"
	 "rJphTVyGNqfKqMFFOwqH/M9CVtaEfwYqT9aB8OSE8MtFe3L1WypEq4tK8VUtoi98\n"
	 "0S9mz4fxathakM+js1eyup/uz0W4cKIFbONLgod0g1arMmSB1Ox7GD6qaUC6zKr8\n"
	 "hWcKMROSg8VFYMhqwGR2k64knXDsVH1mAOgRbJabr3ECAwEAAaOCAfUwggHxMB8G\n"
	 "A1UdIwQYMBaAFFFo/5CvAgd1PMzZZWRiohK4WXI7MB0GA1UdDgQWBBRaTFeTslW8\n"
	 "sjOiEWQkQoHtHefJIjAxBgNVHREEKjAoghMqLmZlZG9yYXByb2plY3Qub3JnghFm\n"
	 "ZWRvcmFwcm9qZWN0Lm9yZzAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYB\n"
	 "BQUHAwEGCCsGAQUFBwMCMHUGA1UdHwRuMGwwNKAyoDCGLmh0dHA6Ly9jcmwzLmRp\n"
	 "Z2ljZXJ0LmNvbS9zaGEyLWhhLXNlcnZlci1nMi5jcmwwNKAyoDCGLmh0dHA6Ly9j\n"
	 "cmw0LmRpZ2ljZXJ0LmNvbS9zaGEyLWhhLXNlcnZlci1nMi5jcmwwQgYDVR0gBDsw\n"
	 "OTA3BglghkgBhv1sAQEwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNl\n"
	 "cnQuY29tL0NQUzCBgwYIKwYBBQUHAQEEdzB1MCQGCCsGAQUFBzABhhhodHRwOi8v\n"
	 "b2NzcC5kaWdpY2VydC5jb20wTQYIKwYBBQUHMAKGQWh0dHA6Ly9jYWNlcnRzLmRp\n"
	 "Z2ljZXJ0LmNvbS9EaWdpQ2VydFNIQTJIaWdoQXNzdXJhbmNlU2VydmVyQ0EuY3J0\n"
	 "MAwGA1UdEwEB/wQCMAAwDQYJKoZIhvcNAQELBQADggEBADSBIYR5GwUfYTHlXeej\n"
	 "tgOMbGIiBD1YPBNlP7vLiGc9+Z4rUxWy/TkL7WUFJf1L88ph1CUQ8TbRjLz2RqL8\n"
	 "snkFWjMsH9ddnwTO4zkCtTjC9fu+broPkmvzmHq2hlXuiDz9G7XvjtbtPujrrKOz\n"
	 "o1pPAEl5c4B0ANaYL0OMUDhvskJguVMC5S/ZNuvNg6k3jkKZWGZPfcxgcZoPvBM8\n"
	 "oIjImGyUMpy7bqRPp4K2xoN530GjoXg8OWIvyAwA06ENLZrU1fcSJsvH2gZVzk8s\n"
	 "EvqFNFnOJN3aQ21imUjAesJ9dXSeCpscDDHqwzmRPuj2/QgtpMCmSZf34mdEzDIJ\n"
	 "hrA=\n" "-----END CERTIFICATE-----\n",
	 .ca = "-----BEGIN CERTIFICATE-----\n"
	 "MIIEsTCCA5mgAwIBAgIQBOHnpNxc8vNtwCtCuF0VnzANBgkqhkiG9w0BAQsFADBs\n"
	 "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
	 "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
	 "ZSBFViBSb290IENBMB4XDTEzMTAyMjEyMDAwMFoXDTI4MTAyMjEyMDAwMFowcDEL\n"
	 "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
	 "LmRpZ2ljZXJ0LmNvbTEvMC0GA1UEAxMmRGlnaUNlcnQgU0hBMiBIaWdoIEFzc3Vy\n"
	 "YW5jZSBTZXJ2ZXIgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC2\n"
	 "4C/CJAbIbQRf1+8KZAayfSImZRauQkCbztyfn3YHPsMwVYcZuU+UDlqUH1VWtMIC\n"
	 "Kq/QmO4LQNfE0DtyyBSe75CxEamu0si4QzrZCwvV1ZX1QK/IHe1NnF9Xt4ZQaJn1\n"
	 "itrSxwUfqJfJ3KSxgoQtxq2lnMcZgqaFD15EWCo3j/018QsIJzJa9buLnqS9UdAn\n"
	 "4t07QjOjBSjEuyjMmqwrIw14xnvmXnG3Sj4I+4G3FhahnSMSTeXXkgisdaScus0X\n"
	 "sh5ENWV/UyU50RwKmmMbGZJ0aAo3wsJSSMs5WqK24V3B3aAguCGikyZvFEohQcft\n"
	 "bZvySC/zA/WiaJJTL17jAgMBAAGjggFJMIIBRTASBgNVHRMBAf8ECDAGAQH/AgEA\n"
	 "MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw\n"
	 "NAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2Vy\n"
	 "dC5jb20wSwYDVR0fBEQwQjBAoD6gPIY6aHR0cDovL2NybDQuZGlnaWNlcnQuY29t\n"
	 "L0RpZ2lDZXJ0SGlnaEFzc3VyYW5jZUVWUm9vdENBLmNybDA9BgNVHSAENjA0MDIG\n"
	 "BFUdIAAwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQuY29tL0NQ\n"
	 "UzAdBgNVHQ4EFgQUUWj/kK8CB3U8zNllZGKiErhZcjswHwYDVR0jBBgwFoAUsT7D\n"
	 "aQP4v0cB1JgmGggC72NkK8MwDQYJKoZIhvcNAQELBQADggEBABiKlYkD5m3fXPwd\n"
	 "aOpKj4PWUS+Na0QWnqxj9dJubISZi6qBcYRb7TROsLd5kinMLYBq8I4g4Xmk/gNH\n"
	 "E+r1hspZcX30BJZr01lYPf7TMSVcGDiEo+afgv2MW5gxTs14nhr9hctJqvIni5ly\n"
	 "/D6q1UEL2tU2ob8cbkdJf17ZSHwD2f2LSaCYJkJA69aSEaRkCldUxPUd1gJea6zu\n"
	 "xICaEnL6VpPX/78whQYwvwt/Tv9XBZ0k7YXDK/umdaisLRbvfXknsuvCnQsH6qqF\n"
	 "0wGjIChBWUMo0oHjqvbsezt3tkBigAVBRQHvFwY+3sAzm2fTYS5yh+Rp/BIAV0Ae\n"
	 "cPUeybQ=\n" "-----END CERTIFICATE-----\n"},
	{			/* as the previous but with first byte invalid */
	 .name = "CA verification (multiple entries)",
	 .queries = {
		     (char *)
		     "\x00\x00\x01\x19\x40\x0b\xe5\xb7\xa3\x1f\xb7\x33\x91\x77\x00\x78\x9d\x2f\x0a\x24\x71\xc0\xc9\xd5\x06\xc0\xe5\x04\xc0\x6c\x16\xd7\xcb\x17\xc0",
		     (char *)
		     "\x03\x01\x01\x54\x4f\x28\x4d\x66\xaf\x2d\xe0\x8c\x17\xe7\x48\x6a\xed\xfa\x2e\x00\xaa\x1a\xc6\xbb\xf3\xaf\x5c\xa6\x2b\x55\xab\x7a\xc2\x69\xbe",
		     (char *)
		     "\x00\x00\x01\x19\x40\x0b\xe5\xb7\xa3\x1f\xb7\x33\x91\x77\x00\x78\x9d\x2f\x0a\x24\x71\xc0\xc9\xd5\x06\xc0\xe5\x04\xc0\x6c\x16\xd7\xcb\x17\xc0",
		     (char *)
		     "\x03\x00\x01\x03\x32\xaa\x2d\x58\xb3\xe0\x54\x4b\x65\x65\x64\x38\x93\x70\x68\xba\x44\xce\x2f\x14\x46\x9c\x4f\x50\xc9\xcc\x69\x33\xc8\x08\xd3",
		     NULL},
	 .q_size = { 35, 35, 35, 35, 0},
	 .expected_ret = 0,
	 .no_queries = 4,
	 .secure = 1,
	 .bogus = 0,
	 .expected_verify_ret = 0,
	 .expected_status = 0,
	 .cert = "-----BEGIN CERTIFICATE-----\n"
	 "MIIGXjCCBUagAwIBAgIQBNO3A71kyzonos0JsLRHrjANBgkqhkiG9w0BAQsFADBw\n"
	 "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
	 "d3cuZGlnaWNlcnQuY29tMS8wLQYDVQQDEyZEaWdpQ2VydCBTSEEyIEhpZ2ggQXNz\n"
	 "dXJhbmNlIFNlcnZlciBDQTAeFw0xNDA0MjIwMDAwMDBaFw0xNzA0MjYxMjAwMDBa\n"
	 "MG0xCzAJBgNVBAYTAlVTMRcwFQYDVQQIEw5Ob3J0aCBDYXJvbGluYTEQMA4GA1UE\n"
	 "BxMHUmFsZWlnaDEVMBMGA1UEChMMUmVkIEhhdCBJbmMuMRwwGgYDVQQDDBMqLmZl\n"
	 "ZG9yYXByb2plY3Qub3JnMIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEA\n"
	 "vpd22JvToGSgnx2CBtfdoqvraQWNpQ1aXd/PSM0PVIqEjutrKJF7Xhr7DgHRLOhy\n"
	 "ko1CzfSp5n6nB7raqDq2kddWgqL3tuyb+lSwdQRGuJZsWW3CCwzR3VNRQUnPldpd\n"
	 "vqHVSkjHkIZYVcZ2FnMYWEa43ESnmgiQGBg4G+T7/9Pv+10SQ+fOE175GWZKHkJm\n"
	 "vJZAjIO2uxvJ/rCq3YQI6hdAsclIiSZ4X8UXWt0IMjp/RdCCnv+SS4XCirZ/IDqM\n"
	 "H+WdMllD0/cbgIOr4SXEuUPEJcI5NziuILe05RefFeZXoC6dxNWr8BvAjxxrZtpS\n"
	 "/7OMwE+WYkVIH8fkgCTVfsa2ZOvMM5CWzxqWKhbFsbw6EGSVIIUtI3C28i3rjLjr\n"
	 "XZ/94k3pf3i/u6DzUmlWm8psn6XZXru0+FKPTrmeDluyuxJsgzudk8mF8Cjw/Oc0\n"
	 "IHVg6Qw/Dm/OM9cAVqmb6ld3GF+QlkzTwurEGKeGj8s8Td0WoPOf6apB/PIaDIu1\n"
	 "rJphTVyGNqfKqMFFOwqH/M9CVtaEfwYqT9aB8OSE8MtFe3L1WypEq4tK8VUtoi98\n"
	 "0S9mz4fxathakM+js1eyup/uz0W4cKIFbONLgod0g1arMmSB1Ox7GD6qaUC6zKr8\n"
	 "hWcKMROSg8VFYMhqwGR2k64knXDsVH1mAOgRbJabr3ECAwEAAaOCAfUwggHxMB8G\n"
	 "A1UdIwQYMBaAFFFo/5CvAgd1PMzZZWRiohK4WXI7MB0GA1UdDgQWBBRaTFeTslW8\n"
	 "sjOiEWQkQoHtHefJIjAxBgNVHREEKjAoghMqLmZlZG9yYXByb2plY3Qub3JnghFm\n"
	 "ZWRvcmFwcm9qZWN0Lm9yZzAOBgNVHQ8BAf8EBAMCBaAwHQYDVR0lBBYwFAYIKwYB\n"
	 "BQUHAwEGCCsGAQUFBwMCMHUGA1UdHwRuMGwwNKAyoDCGLmh0dHA6Ly9jcmwzLmRp\n"
	 "Z2ljZXJ0LmNvbS9zaGEyLWhhLXNlcnZlci1nMi5jcmwwNKAyoDCGLmh0dHA6Ly9j\n"
	 "cmw0LmRpZ2ljZXJ0LmNvbS9zaGEyLWhhLXNlcnZlci1nMi5jcmwwQgYDVR0gBDsw\n"
	 "OTA3BglghkgBhv1sAQEwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNl\n"
	 "cnQuY29tL0NQUzCBgwYIKwYBBQUHAQEEdzB1MCQGCCsGAQUFBzABhhhodHRwOi8v\n"
	 "b2NzcC5kaWdpY2VydC5jb20wTQYIKwYBBQUHMAKGQWh0dHA6Ly9jYWNlcnRzLmRp\n"
	 "Z2ljZXJ0LmNvbS9EaWdpQ2VydFNIQTJIaWdoQXNzdXJhbmNlU2VydmVyQ0EuY3J0\n"
	 "MAwGA1UdEwEB/wQCMAAwDQYJKoZIhvcNAQELBQADggEBADSBIYR5GwUfYTHlXeej\n"
	 "tgOMbGIiBD1YPBNlP7vLiGc9+Z4rUxWy/TkL7WUFJf1L88ph1CUQ8TbRjLz2RqL8\n"
	 "snkFWjMsH9ddnwTO4zkCtTjC9fu+broPkmvzmHq2hlXuiDz9G7XvjtbtPujrrKOz\n"
	 "o1pPAEl5c4B0ANaYL0OMUDhvskJguVMC5S/ZNuvNg6k3jkKZWGZPfcxgcZoPvBM8\n"
	 "oIjImGyUMpy7bqRPp4K2xoN530GjoXg8OWIvyAwA06ENLZrU1fcSJsvH2gZVzk8s\n"
	 "EvqFNFnOJN3aQ21imUjAesJ9dXSeCpscDDHqwzmRPuj2/QgtpMCmSZf34mdEzDIJ\n"
	 "hrA=\n" "-----END CERTIFICATE-----\n",
	 .ca = "-----BEGIN CERTIFICATE-----\n"
	 "MIIEsTCCA5mgAwIBAgIQBOHnpNxc8vNtwCtCuF0VnzANBgkqhkiG9w0BAQsFADBs\n"
	 "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
	 "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
	 "ZSBFViBSb290IENBMB4XDTEzMTAyMjEyMDAwMFoXDTI4MTAyMjEyMDAwMFowcDEL\n"
	 "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
	 "LmRpZ2ljZXJ0LmNvbTEvMC0GA1UEAxMmRGlnaUNlcnQgU0hBMiBIaWdoIEFzc3Vy\n"
	 "YW5jZSBTZXJ2ZXIgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQC2\n"
	 "4C/CJAbIbQRf1+8KZAayfSImZRauQkCbztyfn3YHPsMwVYcZuU+UDlqUH1VWtMIC\n"
	 "Kq/QmO4LQNfE0DtyyBSe75CxEamu0si4QzrZCwvV1ZX1QK/IHe1NnF9Xt4ZQaJn1\n"
	 "itrSxwUfqJfJ3KSxgoQtxq2lnMcZgqaFD15EWCo3j/018QsIJzJa9buLnqS9UdAn\n"
	 "4t07QjOjBSjEuyjMmqwrIw14xnvmXnG3Sj4I+4G3FhahnSMSTeXXkgisdaScus0X\n"
	 "sh5ENWV/UyU50RwKmmMbGZJ0aAo3wsJSSMs5WqK24V3B3aAguCGikyZvFEohQcft\n"
	 "bZvySC/zA/WiaJJTL17jAgMBAAGjggFJMIIBRTASBgNVHRMBAf8ECDAGAQH/AgEA\n"
	 "MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIw\n"
	 "NAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzABhhhodHRwOi8vb2NzcC5kaWdpY2Vy\n"
	 "dC5jb20wSwYDVR0fBEQwQjBAoD6gPIY6aHR0cDovL2NybDQuZGlnaWNlcnQuY29t\n"
	 "L0RpZ2lDZXJ0SGlnaEFzc3VyYW5jZUVWUm9vdENBLmNybDA9BgNVHSAENjA0MDIG\n"
	 "BFUdIAAwKjAoBggrBgEFBQcCARYcaHR0cHM6Ly93d3cuZGlnaWNlcnQuY29tL0NQ\n"
	 "UzAdBgNVHQ4EFgQUUWj/kK8CB3U8zNllZGKiErhZcjswHwYDVR0jBBgwFoAUsT7D\n"
	 "aQP4v0cB1JgmGggC72NkK8MwDQYJKoZIhvcNAQELBQADggEBABiKlYkD5m3fXPwd\n"
	 "aOpKj4PWUS+Na0QWnqxj9dJubISZi6qBcYRb7TROsLd5kinMLYBq8I4g4Xmk/gNH\n"
	 "E+r1hspZcX30BJZr01lYPf7TMSVcGDiEo+afgv2MW5gxTs14nhr9hctJqvIni5ly\n"
	 "/D6q1UEL2tU2ob8cbkdJf17ZSHwD2f2LSaCYJkJA69aSEaRkCldUxPUd1gJea6zu\n"
	 "xICaEnL6VpPX/78whQYwvwt/Tv9XBZ0k7YXDK/umdaisLRbvfXknsuvCnQsH6qqF\n"
	 "0wGjIChBWUMo0oHjqvbsezt3tkBigAVBRQHvFwY+3sAzm2fTYS5yh+Rp/BIAV0Ae\n"
	 "cPUeybQ=\n" "-----END CERTIFICATE-----\n"}
};

static time_t mytime(time_t * t)
{
	time_t then = 1461671166;

	if (t)
		*t = then;

	return then;
}

static void crt_to_der(gnutls_datum_t * chain, const char *pem, unsigned size)
{
	int ret;
	gnutls_x509_crt_t crt;
	gnutls_datum_t input = { (void *)pem, size };

	gnutls_x509_crt_init(&crt);

	ret = gnutls_x509_crt_import(crt, &input, GNUTLS_X509_FMT_PEM);
	if (ret < 0) {
		fail("%d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	ret = gnutls_x509_crt_export2(crt, GNUTLS_X509_FMT_DER, chain);
	if (ret < 0) {
		fail("%d: %s\n", __LINE__, gnutls_strerror(ret));
		exit(1);
	}

	gnutls_x509_crt_deinit(crt);
}

static void dane_raw_check(void)
{
	dane_state_t s;
	dane_query_t r;
	int ret;
	unsigned int entries, i, j;
	char **r_data;
	int *r_data_len;
	int secure;
	int bogus;

	gnutls_global_set_time_function(mytime);

	ret = dane_state_init(&s, DANE_F_IGNORE_LOCAL_RESOLVER);
	if (ret < 0) {
		fail("dane_state_init: %s\n", dane_strerror(ret));
	}

	for (j = 0; j < sizeof(data_entries) / sizeof(data_entries[0]); j++) {
		if (debug)
			success("running test[%d]: %s\n", j,
				data_entries[j].name);

		ret =
		    dane_raw_tlsa(s, &r, data_entries[j].queries,
				  data_entries[j].q_size,
				  data_entries[j].secure,
				  data_entries[j].bogus);
		if (ret != data_entries[j].expected_ret) {
			fail("test[%d]: %d: %s\n", j, __LINE__,
			     dane_strerror(ret));
		}

		ret =
		    dane_query_to_raw_tlsa(r, &entries, &r_data, &r_data_len,
					   &secure, &bogus);
		if (ret < 0) {
			fail("test[%d]: %d: %s\n", j, __LINE__,
			     dane_strerror(ret));
		}

		if (entries != data_entries[j].no_queries)
			fail("test[%d]: %d\n", j, __LINE__);

		if (secure != data_entries[j].secure)
			fail("test[%d]: %d\n", j, __LINE__);

		if (bogus != data_entries[j].bogus)
			fail("test[%d]: %d\n", j, __LINE__);

		for (i = 0; i < entries; i++) {
			if (r_data_len[i] != data_entries[j].q_size[i])
				fail("test[%d]: %d: %s\n", j, __LINE__,
				     dane_strerror(ret));

			if (memcmp
			    (r_data[i], data_entries[j].queries[i],
			     r_data_len[i]) != 0)
				fail("test[%d]: %d: %s\n", j, __LINE__,
				     dane_strerror(ret));
		}

		if (data_entries[j].cert) {	/* verify cert */
			gnutls_datum_t chain[2];
			unsigned status = 0;
			unsigned chain_size = 1;

			crt_to_der(&chain[0], data_entries[j].cert,
				   strlen(data_entries[j].cert));

			if (data_entries[j].ca) {
				crt_to_der(&chain[1], data_entries[j].ca,
					   strlen(data_entries[j].ca));
				chain_size++;
			}

			ret =
			    dane_verify_crt_raw(NULL, chain, chain_size,
						GNUTLS_CRT_X509, r, 0, 0,
						&status);

			if (ret != data_entries[j].expected_verify_ret)
				fail("test[%d]: %d: %s\n", j, __LINE__,
				     dane_strerror(ret));

			if (ret >= 0
			    && status != data_entries[j].expected_status) {
				fail("tests[%d]: expected verif. status %x, got %x\n", j, data_entries[j].expected_status, status);
			}
			free(chain[0].data);
			if (chain_size == 2)
				free(chain[1].data);
		}

		if (debug)
			success("completed test[%d]: %s\n", j,
				data_entries[j].name);

		gnutls_free(r_data);
		gnutls_free(r_data_len);

		dane_query_deinit(r);
	}
	dane_state_deinit(s);
}

void doit(void)
{
	int ret;

	ret = global_init();
	if (ret < 0) {
		fail("global_init\n");
		exit(1);
	}

	dane_raw_check();

	/* we're done */

	gnutls_global_deinit();
}
