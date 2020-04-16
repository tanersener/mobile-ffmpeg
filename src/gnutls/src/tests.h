/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef GNUTLS_SRC_TESTS_H
#define GNUTLS_SRC_TESTS_H

typedef enum {
	TEST_SUCCEED, TEST_FAILED, TEST_UNSURE, TEST_IGNORE/*keep socket*/, TEST_IGNORE2/*destroy socket*/
} test_code_t;

test_code_t test_chain_order(gnutls_session_t session);
test_code_t test_server(gnutls_session_t state);
test_code_t test_record_padding(gnutls_session_t state);
test_code_t test_no_extensions(gnutls_session_t state);
test_code_t test_heartbeat_extension(gnutls_session_t state);
test_code_t test_small_records(gnutls_session_t state);
test_code_t test_rfc7507(gnutls_session_t state);
test_code_t test_dhe(gnutls_session_t state);
test_code_t test_rfc7919(gnutls_session_t state);
test_code_t test_dhe_group(gnutls_session_t state);
test_code_t test_ssl3(gnutls_session_t state);
test_code_t test_ssl3_with_extensions(gnutls_session_t state);
test_code_t test_ssl3_unknown_ciphersuites(gnutls_session_t state);
test_code_t test_aes(gnutls_session_t state);
test_code_t test_camellia_cbc(gnutls_session_t state);
test_code_t test_camellia_gcm(gnutls_session_t state);
test_code_t test_md5(gnutls_session_t state);
test_code_t test_sha(gnutls_session_t state);
test_code_t test_3des(gnutls_session_t state);
test_code_t test_arcfour(gnutls_session_t state);
test_code_t test_chacha20(gnutls_session_t state);
test_code_t test_tls1(gnutls_session_t state);
test_code_t test_tls1_nossl3(gnutls_session_t session);
test_code_t test_safe_renegotiation(gnutls_session_t state);
test_code_t test_ext_master_secret(gnutls_session_t state);
test_code_t test_etm(gnutls_session_t state);
test_code_t test_safe_renegotiation_scsv(gnutls_session_t state);
test_code_t test_tls1_1(gnutls_session_t state);
test_code_t test_tls1_2(gnutls_session_t state);
test_code_t test_tls1_3(gnutls_session_t state);
test_code_t test_known_protocols(gnutls_session_t state);
test_code_t test_tls1_1_fallback(gnutls_session_t state);
test_code_t test_tls1_6_fallback(gnutls_session_t state);
test_code_t test_tls_disable0(gnutls_session_t state);
test_code_t test_tls_disable1(gnutls_session_t state);
test_code_t test_tls_disable2(gnutls_session_t state);
test_code_t test_ocsp_status(gnutls_session_t state);
test_code_t test_rsa_pms(gnutls_session_t state);
test_code_t test_max_record_size(gnutls_session_t state);
test_code_t test_version_rollback(gnutls_session_t state);
test_code_t test_anonymous(gnutls_session_t state);
test_code_t test_unknown_ciphersuites(gnutls_session_t state);
test_code_t test_bye(gnutls_session_t state);
test_code_t test_certificate(gnutls_session_t state);
test_code_t test_server_cas(gnutls_session_t state);
test_code_t test_session_resume2(gnutls_session_t state);
test_code_t test_rsa_pms_version_check(gnutls_session_t session);
test_code_t test_version_oob(gnutls_session_t session);
test_code_t test_send_record(gnutls_session_t session);
test_code_t test_send_record_with_allow_small_records(gnutls_session_t session);
int _test_srp_username_callback(gnutls_session_t session,
				char **username, char **password);

test_code_t test_rsa(gnutls_session_t session);
test_code_t test_ecdhe_x25519(gnutls_session_t session);
test_code_t test_ecdhe_secp521r1(gnutls_session_t session);
test_code_t test_ecdhe_secp384r1(gnutls_session_t session);
test_code_t test_ecdhe_secp256r1(gnutls_session_t session);
test_code_t test_ecdhe(gnutls_session_t session);
test_code_t test_aes_gcm(gnutls_session_t session);
test_code_t test_aes_ccm(gnutls_session_t session);
test_code_t test_aes_ccm_8(gnutls_session_t session);
test_code_t test_sha256(gnutls_session_t session);

#ifdef ENABLE_GOST
test_code_t test_vko_gost_12(gnutls_session_t session);
test_code_t test_gost_cnt(gnutls_session_t session);
test_code_t test_gost_imit(gnutls_session_t session);
#endif

#endif /* GNUTLS_SRC_TESTS_H */
