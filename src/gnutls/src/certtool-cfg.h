/*
 * Copyright (C) 2004-2012 Free Software Foundation, Inc.
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
 *
 * Written by Nikos Mavrogiannopoulos <nmav@gnutls.org>.
 */

#ifndef GNUTLS_SRC_CERTTOOL_CFG_H
#define GNUTLS_SRC_CERTTOOL_CFG_H

#include <stdbool.h>
#include <stdint.h>
#include <gnutls/x509.h>

void cfg_init(void);
int template_parse(const char *template);

void read_crt_set(gnutls_x509_crt_t crt, const char *input_str,
		  const char *oid);
void read_crq_set(gnutls_x509_crq_t crq, const char *input_str,
		  const char *oid);
int64_t read_int(const char *input_str);
int serial_decode(const char *input, gnutls_datum_t *output);
const char *read_str(const char *input_str);
int read_yesno(const char *input_str, int def);

const char *get_pass(void);
const char *get_confirmed_pass(bool empty_ok);
const char *get_challenge_pass(void);
void get_crl_dist_point_set(gnutls_x509_crt_t crt);
void crt_constraints_set(gnutls_x509_crt_t crt);
void get_country_crt_set(gnutls_x509_crt_t crt);
void get_organization_crt_set(gnutls_x509_crt_t crt);
void get_unit_crt_set(gnutls_x509_crt_t crt);
void get_state_crt_set(gnutls_x509_crt_t crt);
void get_locality_crt_set(gnutls_x509_crt_t crt);
void get_cn_crt_set(gnutls_x509_crt_t crt);
void get_dn_crt_set(gnutls_x509_crt_t crt);
void get_dn_crq_set(gnutls_x509_crq_t crt);
void get_uid_crt_set(gnutls_x509_crt_t crt);
void get_pkcs9_email_crt_set(gnutls_x509_crt_t crt);
void get_oid_crt_set(gnutls_x509_crt_t crt);
void get_key_purpose_set(int type, void *crt);
void get_serial(unsigned char* serial, size_t* serial_size);
time_t get_expiration_date(void);
time_t get_activation_date(void);
int get_ca_status(void);
void get_crl_number(unsigned char* serial, size_t* serial_size);
int get_path_len(void);
int get_crq_extensions_status(void);
const char *get_pkcs12_key_name(void);
int get_tls_client_status(void);
int get_tls_server_status(void);
time_t get_crl_next_update(void);
time_t get_crl_revocation_date(void);
time_t get_crl_this_update_date(void);
int get_time_stamp_status(void);
int get_email_protection_status(void);
int get_ocsp_sign_status(void);
int get_code_sign_status(void);
int get_crl_sign_status(void);
int get_cert_sign_status(void);
int get_encrypt_status(int server);
int get_sign_status(int server);
void get_ip_addr_set(int type, void *crt);
void get_dns_name_set(int type, void *crt);
void get_other_name_set(int type, void *crt);
void get_extensions_crt_set(int type, void *crt);
void get_policy_set(gnutls_x509_crt_t);
void get_uri_set(int type, void *crt);
void get_email_set(int type, void *crt);
int get_ipsec_ike_status(void);
void get_dc_set(int type, void *crt);
void get_ca_issuers_set(gnutls_x509_crt_t crt);
void get_ocsp_issuer_set(gnutls_x509_crt_t crt);
void crt_unique_ids_set(gnutls_x509_crt_t crt);
void get_tlsfeatures_set(int type, void *crt);

int get_key_agreement_status(void);
int get_non_repudiation_status(void);
int get_data_encipherment_status(void);

void get_cn_crq_set(gnutls_x509_crq_t crq);
void get_uid_crq_set(gnutls_x509_crq_t crq);
void get_locality_crq_set(gnutls_x509_crq_t crq);
void get_state_crq_set(gnutls_x509_crq_t crq);
void get_unit_crq_set(gnutls_x509_crq_t crq);
void get_organization_crq_set(gnutls_x509_crq_t crq);
void get_country_crq_set(gnutls_x509_crq_t crq);
void get_oid_crq_set(gnutls_x509_crq_t crq);
const char *get_proxy_policy(char **policy, size_t * policylen);

void crq_extensions_set(gnutls_x509_crt_t crt, gnutls_x509_crq_t crq);

#endif /* GNUTLS_SRC_CERTTOOL_CFG_H */
