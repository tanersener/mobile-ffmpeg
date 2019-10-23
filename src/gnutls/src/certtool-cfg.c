/*
 * Copyright (C) 2004-2014 Free Software Foundation, Inc.
 * Copyright (C) 2013,2014 Nikos Mavrogiannopoulos
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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <certtool-cfg.h>
#include <gnutls/x509.h>
#include <gnutls/x509-ext.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <timespec.h>
#include <parse-datetime.h>
#include <autoopts/options.h>
#include <intprops.h>
#include <gnutls/crypto.h>
#include <libtasn1.h>

/* for inet_pton */
#include <sys/types.h>

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#elif HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif

/* From gnulib for inet_pton() */
#include <arpa/inet.h>

/* Gnulib portability files. */
#include <getpass.h>
#include "certtool-common.h"

/* to print uint64_t */
# define __STDC_FORMAT_MACROS
# include <inttypes.h>

extern int batch;
extern int ask_pass;

#define MAX_ENTRIES 128
#define MAX_POLICIES 8

#define CHECK_MALLOC(x) \
	if (x == NULL) { \
		fprintf(stderr, "memory error\n"); \
		exit(1); \
	}

#define PRINT_TIME_T_ERROR \
	if (sizeof(time_t) < 8) \
		fprintf(stderr, "This system expresses time with a 32-bit time_t; that prevents dates after 2038 to be expressed by GnuTLS.\n")

enum option_types { OPTION_NUMERIC, OPTION_STRING, OPTION_BOOLEAN, OPTION_MULTI_LINE };

struct cfg_options {
	const char *name;
	unsigned type;

	/* used when parsing */
	unsigned found;
};

static struct cfg_options available_options[] = {
	{ .name = "unit", .type = OPTION_MULTI_LINE },
	{ .name = "ou", .type = OPTION_MULTI_LINE },
	{ .name = "organization", .type = OPTION_MULTI_LINE },
	{ .name = "o", .type = OPTION_MULTI_LINE },
	{ .name = "dc", .type = OPTION_MULTI_LINE },
	{ .name = "dns_name", .type = OPTION_MULTI_LINE },
	{ .name = "ip_address", .type = OPTION_MULTI_LINE },
	{ .name = "email", .type = OPTION_MULTI_LINE },
	{ .name = "krb5_principal", .type = OPTION_MULTI_LINE },
	{ .name = "other_name", .type = OPTION_MULTI_LINE },
	{ .name = "other_name_utf8", .type = OPTION_MULTI_LINE },
	{ .name = "other_name_octet", .type = OPTION_MULTI_LINE },
	{ .name = "xmpp_name", .type = OPTION_MULTI_LINE },
	{ .name = "key_purpose_oid", .type = OPTION_MULTI_LINE },
	{ .name = "nc_exclude_dns", .type = OPTION_MULTI_LINE },
	{ .name = "nc_exclude_ip", .type = OPTION_MULTI_LINE },
	{ .name = "nc_exclude_email", .type = OPTION_MULTI_LINE },
	{ .name = "nc_permit_dns", .type = OPTION_MULTI_LINE },
	{ .name = "nc_permit_ip", .type = OPTION_MULTI_LINE },
	{ .name = "nc_permit_email", .type = OPTION_MULTI_LINE },
	{ .name = "dn_oid", .type = OPTION_MULTI_LINE },
	{ .name = "add_extension", .type = OPTION_MULTI_LINE },
	{ .name = "add_critical_extension", .type = OPTION_MULTI_LINE },
	{ .name = "crl_dist_points", .type = OPTION_MULTI_LINE },
	{ .name = "uri", .type = OPTION_MULTI_LINE },
	{ .name = "ocsp_uri", .type = OPTION_MULTI_LINE },
	{ .name = "ca_issuers_uri", .type = OPTION_MULTI_LINE },
	{ .name = "locality", .type = OPTION_STRING },
	{ .name = "state", .type = OPTION_STRING },
	{ .name = "dn", .type = OPTION_STRING },
	{ .name = "cn", .type = OPTION_STRING },
	{ .name = "uid", .type = OPTION_STRING },
	{ .name = "subject_unique_id", .type = OPTION_STRING },
	{ .name = "issuer_unique_id", .type = OPTION_STRING },
	{ .name = "challenge_password", .type = OPTION_STRING },
	{ .name = "password", .type = OPTION_STRING },
	{ .name = "pkcs9_email", .type = OPTION_STRING },
	{ .name = "country", .type = OPTION_STRING },
	{ .name = "expiration_date", .type = OPTION_STRING },
	{ .name = "activation_date", .type = OPTION_STRING },
	{ .name = "crl_revocation_date", .type = OPTION_STRING },
	{ .name = "crl_this_update_date", .type = OPTION_STRING },
	{ .name = "crl_next_update_date", .type = OPTION_STRING },
	{ .name = "policy*", .type = OPTION_MULTI_LINE }, /* not a multi-line but there are multi as it is a wildcard */
	{ .name = "inhibit_anypolicy_skip_certs", .type = OPTION_NUMERIC },
	{ .name = "pkcs12_key_name", .type = OPTION_STRING },
	{ .name = "proxy_policy_language", .type = OPTION_STRING },
	{ .name = "serial", .type = OPTION_STRING },
	{ .name = "expiration_days", .type = OPTION_NUMERIC },
	{ .name = "crl_next_update", .type = OPTION_NUMERIC },
	{ .name = "crl_number", .type = OPTION_STRING },
	{ .name = "path_len", .type = OPTION_NUMERIC },
	{ .name = "ca", .type = OPTION_BOOLEAN },
	{ .name = "honor_crq_extensions", .type = OPTION_BOOLEAN },
	{ .name = "honor_crq_ext", .type = OPTION_MULTI_LINE },
	{ .name = "tls_www_client", .type = OPTION_BOOLEAN },
	{ .name = "tls_www_server", .type = OPTION_BOOLEAN },
	{ .name = "signing_key", .type = OPTION_BOOLEAN },
	{ .name = "encryption_key", .type = OPTION_BOOLEAN },
	{ .name = "cert_signing_key", .type = OPTION_BOOLEAN },
	{ .name = "crl_signing_key", .type = OPTION_BOOLEAN },
	{ .name = "code_signing_key", .type = OPTION_BOOLEAN },
	{ .name = "ocsp_signing_key", .type = OPTION_BOOLEAN },
	{ .name = "time_stamping_key", .type = OPTION_BOOLEAN },
	{ .name = "email_protection_key", .type = OPTION_BOOLEAN },
	{ .name = "ipsec_ike_key", .type = OPTION_BOOLEAN },
	{ .name = "key_agreement", .type = OPTION_BOOLEAN },
	{ .name = "data_encipherment", .type = OPTION_BOOLEAN },
	{ .name = "non_repudiation", .type = OPTION_BOOLEAN },
	{ .name = "tls_feature", .type = OPTION_MULTI_LINE },
};

typedef struct _cfg_ctx {
	char **organization;
	char **unit;
	char *locality;
	char *state;
	char *dn;
	char *cn;
	char *uid;
	uint8_t *subject_unique_id;
	unsigned subject_unique_id_size;
	uint8_t *issuer_unique_id;
	unsigned issuer_unique_id_size;
	char *challenge_password;
	char *pkcs9_email;
	char *country;
	char *policy_oid[MAX_POLICIES];
	char *policy_txt[MAX_POLICIES];
	char *policy_url[MAX_POLICIES];
	char **dc;
	char **dns_name;
	char **uri;
	char **ip_addr;
	char **email;
	char **krb5_principal;
	char **other_name;
	char **other_name_utf8;
	char **other_name_octet;
	char **xmpp_name;
	char **dn_oid;
	char **extensions;
	char **crit_extensions;
	char **permitted_nc_ip;
	char **excluded_nc_ip;
	char **permitted_nc_dns;
	char **excluded_nc_dns;
	char **permitted_nc_email;
	char **excluded_nc_email;
	char **crl_dist_points;
	char *password;
	char *pkcs12_key_name;
	char *expiration_date;
	char *activation_date;
	char *revocation_date;
	char *this_update_date;
	char *next_update_date;
	uint8_t *serial;
	unsigned serial_size;
	int expiration_days;
	int skip_certs; /* from inhibit anypolicy */
	int ca;
	int path_len;
	int tls_www_client;
	int tls_www_server;
	int signing_key;
	int encryption_key;
	int cert_sign_key;
	int crl_sign_key;
	int non_repudiation;
	int data_encipherment;
	int key_agreement;
	int code_sign_key;
	int ocsp_sign_key;
	int time_stamping_key;
	int email_protection_key;
	int ipsec_ike_key;
	char **key_purpose_oids;
	int crl_next_update;
	uint8_t *crl_number;
	unsigned crl_number_size;
	int honor_crq_extensions;
	char *proxy_policy_language;
	char **exts_to_honor;
	char **ocsp_uris;
	char **ca_issuers_uris;
	char **tls_features;
} cfg_ctx;

cfg_ctx cfg;

void cfg_init(void)
{
	memset(&cfg, 0, sizeof(cfg));
	cfg.path_len = -1;
	cfg.skip_certs = -1;
}

#define READ_MULTI_LINE(name, s_name) \
  val = optionGetValue(pov, name); \
  if (val != NULL && val->valType == OPARG_TYPE_STRING) \
  { \
    if (s_name == NULL) { \
      i = 0; \
      s_name = malloc(sizeof(char*)*MAX_ENTRIES); \
      CHECK_MALLOC(s_name); \
      do { \
	if (val && strcmp(val->pzName, name)!=0) \
	  continue; \
	s_name[i] = strdup(val->v.strVal); \
	i++; \
	  if (i>=MAX_ENTRIES) \
	    break; \
      } while((val = optionNextValue(pov, val)) != NULL); \
      s_name[i] = NULL; \
    } \
  }

#define READ_MULTI_LINE_TOKENIZED(name, s_name) \
  val = optionGetValue(pov, name); \
  if (val != NULL && val->valType == OPARG_TYPE_STRING) \
  { \
    char *str; \
    char *p; \
    if (s_name == NULL) { \
      i = 0; \
      s_name = malloc(sizeof(char*)*MAX_ENTRIES); \
      CHECK_MALLOC(s_name); \
      do { \
	if (val && strcmp(val->pzName, name)!=0) \
	  continue; \
	str = strdup(val->v.strVal); \
	CHECK_MALLOC(str); \
	if ((p=strchr(str, ' ')) == NULL && (p=strchr(str, '\t')) == NULL) { \
	  fprintf(stderr, "Error parsing %s\n", name); \
	  exit(1); \
	} \
	p[0] = 0; \
	p++; \
	s_name[i] = strdup(str); \
	while(*p==' ' || *p == '\t') p++; \
	if (p[0] == 0) { \
	  fprintf(stderr, "Error (2) parsing %s\n", name); \
	  exit(1); \
	} \
	s_name[i+1] = strdup(p); \
	i+=2; \
	free(str); \
	if (i>=MAX_ENTRIES) \
	  break; \
      } while((val = optionNextValue(pov, val)) != NULL); \
      s_name[i] = NULL; \
    } \
  }

#define READ_BOOLEAN(name, s_name) \
  val = optionGetValue(pov, name); \
  if (val != NULL) \
    { \
      s_name = 1; \
    }

/* READ_NUMERIC only returns a long */
#define READ_NUMERIC(name, s_name) \
  val = optionGetValue(pov, name); \
  if (val != NULL) \
    { \
      if (val->valType == OPARG_TYPE_NUMERIC) \
	s_name = val->v.longVal; \
      else if (val->valType == OPARG_TYPE_STRING) \
	s_name = strtol(val->v.strVal, NULL, 10); \
    }

#define HEX_DECODE(hex, output, output_size) \
	{ \
		gnutls_datum_t _input = {(void*)hex, strlen(hex)}; \
		gnutls_datum_t _output; \
		ret = gnutls_hex_decode2(&_input, &_output); \
		if (ret < 0) { \
			fprintf(stderr, "error in hex ID: %s\n", hex); \
			exit(1); \
		} \
		output = _output.data; \
		output_size = _output.size; \
	}

#define SERIAL_DECODE(input, output, output_size) \
	{ \
		gnutls_datum_t _output; \
		ret = serial_decode(input, &_output); \
		if (ret < 0) { \
			fprintf(stderr, "error parsing number: %s\n", input); \
			exit(1); \
		} \
		output = _output.data; \
		output_size = _output.size; \
	}


static int handle_option(const tOptionValue* val)
{
unsigned j;
unsigned len, cmp;

	for (j=0;j<sizeof(available_options)/sizeof(available_options[0]);j++) {
		len = strlen(available_options[j].name);
		if (len > 2 && available_options[j].name[len-1] == '*')
			cmp = strncasecmp(val->pzName, available_options[j].name, len-1);
		else
			cmp = strcasecmp(val->pzName, available_options[j].name);

		if (cmp == 0) {
			if (available_options[j].type != OPTION_MULTI_LINE &&
			    available_options[j].found != 0) {
			    fprintf(stderr, "Warning: multiple options found for '%s'; only the first will be taken into account.\n", available_options[j].name);
			}
			available_options[j].found = 1;
			return 1;
		}
	}

	return 0;
}

int template_parse(const char *template)
{
	/* Parsing return code */
	unsigned int i;
	int ret;
	tOptionValue const *pov;
	const tOptionValue *val, *prev;
	char tmpstr[256];

	pov = configFileLoad(template);
	if (pov == NULL) {
		perror("configFileLoad");
		fprintf(stderr, "Error loading template: %s\n", template);
		exit(1);
	}

	val = optionGetValue(pov, NULL);
	while (val != NULL) {
		if (handle_option(val) == 0) {
			fprintf(stderr, "Warning: skipping unknown option '%s'\n", val->pzName);
		}
		prev = val;
		val = optionNextValue(pov, prev);
	}

	/* Option variables */
	READ_MULTI_LINE("unit", cfg.unit);
	if (cfg.unit == NULL) {
		READ_MULTI_LINE("ou", cfg.unit);
	}

	READ_MULTI_LINE("organization", cfg.organization);
	if (cfg.organization == NULL) {
		READ_MULTI_LINE("o", cfg.organization);
	}

	val = optionGetValue(pov, "locality");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.locality = strdup(val->v.strVal);

	val = optionGetValue(pov, "state");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.state = strdup(val->v.strVal);

	val = optionGetValue(pov, "dn");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.dn = strdup(val->v.strVal);

	val = optionGetValue(pov, "cn");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.cn = strdup(val->v.strVal);

	val = optionGetValue(pov, "uid");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.uid = strdup(val->v.strVal);

	val = optionGetValue(pov, "issuer_unique_id");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		HEX_DECODE(val->v.strVal, cfg.issuer_unique_id, cfg.issuer_unique_id_size);

	val = optionGetValue(pov, "subject_unique_id");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		HEX_DECODE(val->v.strVal, cfg.subject_unique_id, cfg.subject_unique_id_size);

	val = optionGetValue(pov, "challenge_password");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.challenge_password = strdup(val->v.strVal);

	val = optionGetValue(pov, "password");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.password = strdup(val->v.strVal);

	val = optionGetValue(pov, "pkcs9_email");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.pkcs9_email = strdup(val->v.strVal);

	val = optionGetValue(pov, "country");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.country = strdup(val->v.strVal);

	val = optionGetValue(pov, "expiration_date");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.expiration_date = strdup(val->v.strVal);

	val = optionGetValue(pov, "activation_date");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.activation_date = strdup(val->v.strVal);

	val = optionGetValue(pov, "crl_revocation_date");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.revocation_date = strdup(val->v.strVal);

	val = optionGetValue(pov, "crl_this_update_date");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.this_update_date = strdup(val->v.strVal);

	val = optionGetValue(pov, "crl_next_update_date");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.next_update_date = strdup(val->v.strVal);

	READ_NUMERIC("inhibit_anypolicy_skip_certs", cfg.skip_certs);

	for (i = 0; i < MAX_POLICIES; i++) {
		snprintf(tmpstr, sizeof(tmpstr), "policy%d", i + 1);
		val = optionGetValue(pov, tmpstr);
		if (val != NULL && val->valType == OPARG_TYPE_STRING)
			cfg.policy_oid[i] = strdup(val->v.strVal);

		if (cfg.policy_oid[i] != NULL) {
			snprintf(tmpstr, sizeof(tmpstr), "policy%d_url",
				 i + 1);
			val = optionGetValue(pov, tmpstr);
			if (val != NULL
			    && val->valType == OPARG_TYPE_STRING)
				cfg.policy_url[i] = strdup(val->v.strVal);

			snprintf(tmpstr, sizeof(tmpstr), "policy%d_txt",
				 i + 1);
			val = optionGetValue(pov, tmpstr);
			if (val != NULL
			    && val->valType == OPARG_TYPE_STRING) {
				cfg.policy_txt[i] = strdup(val->v.strVal);
			}
		}
	}

	READ_MULTI_LINE("dc", cfg.dc);
	READ_MULTI_LINE("dns_name", cfg.dns_name);
	READ_MULTI_LINE("uri", cfg.uri);
	READ_MULTI_LINE("krb5_principal", cfg.krb5_principal);
	READ_MULTI_LINE_TOKENIZED("other_name", cfg.other_name);
	READ_MULTI_LINE_TOKENIZED("other_name_octet", cfg.other_name_octet);
	READ_MULTI_LINE_TOKENIZED("other_name_utf8", cfg.other_name_utf8);

	READ_MULTI_LINE("xmpp_name", cfg.xmpp_name);
	READ_MULTI_LINE("ip_address", cfg.ip_addr);
	READ_MULTI_LINE("email", cfg.email);
	READ_MULTI_LINE("key_purpose_oid", cfg.key_purpose_oids);

	READ_MULTI_LINE("nc_exclude_ip", cfg.excluded_nc_ip);
	READ_MULTI_LINE("nc_exclude_dns", cfg.excluded_nc_dns);
	READ_MULTI_LINE("nc_exclude_email", cfg.excluded_nc_email);
	READ_MULTI_LINE("nc_permit_ip", cfg.permitted_nc_ip);
	READ_MULTI_LINE("nc_permit_dns", cfg.permitted_nc_dns);
	READ_MULTI_LINE("nc_permit_email", cfg.permitted_nc_email);

	READ_MULTI_LINE_TOKENIZED("dn_oid", cfg.dn_oid);

	READ_MULTI_LINE_TOKENIZED("add_extension", cfg.extensions);
	READ_MULTI_LINE_TOKENIZED("add_critical_extension", cfg.crit_extensions);

	READ_MULTI_LINE("crl_dist_points", cfg.crl_dist_points);

	val = optionGetValue(pov, "pkcs12_key_name");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.pkcs12_key_name = strdup(val->v.strVal);


	val = optionGetValue(pov, "serial");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		SERIAL_DECODE(val->v.strVal, cfg.serial, cfg.serial_size);

	READ_NUMERIC("expiration_days", cfg.expiration_days);
	READ_NUMERIC("crl_next_update", cfg.crl_next_update);

	val = optionGetValue(pov, "crl_number");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		SERIAL_DECODE(val->v.strVal, cfg.crl_number, cfg.crl_number_size);

	READ_NUMERIC("path_len", cfg.path_len);

	val = optionGetValue(pov, "proxy_policy_language");
	if (val != NULL && val->valType == OPARG_TYPE_STRING)
		cfg.proxy_policy_language = strdup(val->v.strVal);

	READ_MULTI_LINE("ocsp_uri", cfg.ocsp_uris);
	READ_MULTI_LINE("ca_issuers_uri", cfg.ca_issuers_uris);

	READ_BOOLEAN("ca", cfg.ca);
	READ_BOOLEAN("honor_crq_extensions", cfg.honor_crq_extensions);
	READ_MULTI_LINE("honor_crq_ext", cfg.exts_to_honor);

	READ_BOOLEAN("tls_www_client", cfg.tls_www_client);
	READ_BOOLEAN("tls_www_server", cfg.tls_www_server);
	READ_BOOLEAN("signing_key", cfg.signing_key);
	READ_BOOLEAN("encryption_key", cfg.encryption_key);
	READ_BOOLEAN("cert_signing_key", cfg.cert_sign_key);
	READ_BOOLEAN("crl_signing_key", cfg.crl_sign_key);
	READ_BOOLEAN("code_signing_key", cfg.code_sign_key);
	READ_BOOLEAN("ocsp_signing_key", cfg.ocsp_sign_key);
	READ_BOOLEAN("time_stamping_key", cfg.time_stamping_key);
	READ_BOOLEAN("email_protection_key", cfg.email_protection_key);
	READ_BOOLEAN("ipsec_ike_key", cfg.ipsec_ike_key);

	READ_BOOLEAN("data_encipherment", cfg.data_encipherment);
	READ_BOOLEAN("key_agreement", cfg.key_agreement);
	READ_BOOLEAN("non_repudiation", cfg.non_repudiation);

	READ_MULTI_LINE("tls_feature", cfg.tls_features);

	optionUnloadNested(pov);

	return 0;
}

#define IS_NEWLINE(x) ((x[0] == '\n') || (x[0] == '\r'))
#define MAX_INPUT_SIZE 512

static size_t strip_nl(char *str, size_t str_size)
{
	if ((str_size > 0) && (str[str_size - 1] == '\n')) {
		str_size--;
		str[str_size] = 0;
	}
	if ((str_size > 0) && (str[str_size - 1] == '\r')) {
		str_size--;
		str[str_size] = 0;
	}
	return str_size;
}

static int copystr_without_nl(char *out, size_t out_size, const char *in, size_t in_size)
{
	if (in_size+1 >= out_size) {
		fprintf(stderr, "Too long line to parse in interactive mode; please use templates.\n");
		exit(1);
	}
	memcpy(out, in, in_size+1); /* copy terminating null */
	strip_nl(out, in_size);
	return 0;
}

void
read_crt_set(gnutls_x509_crt_t crt, const char *input_str, const char *oid)
{
	ssize_t ret;
	char *lineptr = NULL;
	size_t linesize = 0;

	fputs(input_str, stderr);
	ret = getline(&lineptr, &linesize, stdin);
	if (ret == -1)
		return;

	if (IS_NEWLINE(lineptr)) {
		free(lineptr);
		return;
	}

	linesize = strip_nl(lineptr, ret);

	ret =
	    gnutls_x509_crt_set_dn_by_oid(crt, oid, 0, lineptr,
					  linesize);
	if (ret < 0) {
		fprintf(stderr, "set_dn: %s\n", gnutls_strerror(ret));
		exit(1);
	}
	free(lineptr);
}

void
read_crq_set(gnutls_x509_crq_t crq, const char *input_str, const char *oid)
{
	ssize_t ret;
	char *lineptr = NULL;
	size_t linesize = 0;

	fputs(input_str, stderr);
	ret = getline(&lineptr, &linesize, stdin);
	if (ret == -1)
		return;

	if (IS_NEWLINE(lineptr)) {
		free(lineptr);
		return;
	}

	linesize = strip_nl(lineptr, ret);

	ret =
	    gnutls_x509_crq_set_dn_by_oid(crq, oid, 0, lineptr,
					  linesize);
	if (ret < 0) {
		fprintf(stderr, "set_dn: %s\n", gnutls_strerror(ret));
		exit(1);
	}
	free(lineptr);
}

/* The input_str should contain %d or %u to print the default.
 */
static int64_t read_int_with_default(const char *input_str, long def)
{
	char *endptr;
	int64_t l;
	static char input[MAX_INPUT_SIZE];

	fprintf(stderr, input_str, def);
	if (fgets(input, sizeof(input), stdin) == NULL)
		return def;

	if (IS_NEWLINE(input))
		return def;

#if SIZEOF_LONG < 8
	l = strtoll(input, &endptr, 0);

	if (*endptr != '\0' && *endptr != '\r' && *endptr != '\n') {
		fprintf(stderr, "Trailing garbage ignored: `%s'\n",
			endptr);
		return 0;
	} else {
		*endptr = 0;
	}

	if (l <= LLONG_MIN || l >= LLONG_MAX) {
		fprintf(stderr, "Integer out of range: `%s' (max: %llu)\n", input, LLONG_MAX-1);
		return 0;
	}
#else
	l = strtol(input, &endptr, 0);

	if (*endptr != '\0' && *endptr != '\r' && *endptr != '\n') {
		fprintf(stderr, "Trailing garbage ignored: `%s'\n",
			endptr);
		return 0;
	} else {
		*endptr = 0;
	}

	if (l <= LONG_MIN || l >= LONG_MAX) {
		fprintf(stderr, "Integer out of range: `%s' (max: %lu)\n", input, LONG_MAX-1);
		return 0;
	}
#endif



	if (input == endptr)
		l = def;

	return l;
}

int64_t read_int(const char *input_str)
{
	return read_int_with_default(input_str, 0);
}

int serial_decode(const char *input, gnutls_datum_t *output)
{
	int i;
	int64_t value;
	char *endptr;
	int64_t value_limit;
	gnutls_datum_t input_datum;

	if (input[0] == '0' && input[1] == 'x') {
		input_datum.data = (void *) (input + 2);
		input_datum.size = strlen(input + 2);
		if (input_datum.size == 0) {
			return GNUTLS_E_PARSING_ERROR;
		}
		return gnutls_hex_decode2(&input_datum, output);
	}

#if SIZEOF_LONG < 8
	value = strtol(input, &endptr, 10);
	value_limit = LONG_MAX;
#else
	value = strtoll(input, &endptr, 10);
	value_limit = LLONG_MAX;
#endif

	if (*endptr != '\0') {
		fprintf(stderr, "Trailing garbage: `%s'\n", endptr);
		return GNUTLS_E_PARSING_ERROR;
	}

	if (value <= 0 || value >= value_limit) {
		fprintf(stderr, "Integer out of range: `%s' (min: 1, max: %"PRId64")\n", input, value_limit-1);
		return GNUTLS_E_PARSING_ERROR;
	}

	output->size = sizeof(int64_t);
	output->data = gnutls_malloc(output->size);
	if (output->data == NULL) {
		output->size = 0;
		return GNUTLS_E_MEMORY_ERROR;
	}

	for (i = output->size - 1; i >= 0; i--) {
		output->data[i] = value & 0xff;
		value = value >> 8;
	}

	return 0;
}

const char *read_str(const char *input_str)
{
	static char input[MAX_INPUT_SIZE];
	ssize_t ret;
	char *lineptr = NULL;
	size_t linesize = 0;

	fputs(input_str, stderr);
	ret = getline(&lineptr, &linesize, stdin);
	if (ret == -1)
		return NULL;

	ret = copystr_without_nl(input, sizeof(input), lineptr, ret);
	free(lineptr);
	if (ret < 0)
		return NULL;

	if (IS_NEWLINE(input))
		return NULL;

	if (input[0] == 0)
		return NULL;

	return input;
}

/* Default is:
 * def: 0 -> no
 * def: 1 -> yes
 */
int read_yesno(const char *input_str, int def)
{
	char input[MAX_INPUT_SIZE];

      restart:
	fputs(input_str, stderr);
	if (fgets(input, sizeof(input), stdin) == NULL)
		return def;

	if (IS_NEWLINE(input))
		return def;

	if (input[0] == 'y' || input[0] == 'Y')
		return 1;
	else if (input[0] == 'n' || input[0] == 'N')
		return 0;
	else
		goto restart;
}


/* Wrapper functions for non-interactive mode.
 */
const char *get_pass(void)
{
	if (batch && !ask_pass)
		return cfg.password;
	else
		return getpass("Enter password: ");
}

const char *get_confirmed_pass(bool empty_ok)
{
	if (batch && !ask_pass)
		return cfg.password;
	else {
		const char *pass = NULL;
		char *copy = NULL;

		do {
			if (pass)
				fprintf(stderr,
					"Password mismatch, try again.\n");

			free(copy);

			pass = getpass("Enter password: ");
			copy = strdup(pass);
			pass = getpass("Confirm password: ");
		}
		while (strcmp(pass, copy) != 0
		       && !(empty_ok && *pass == '\0'));

		free(copy);

		return pass;
	}
}

const char *get_challenge_pass(void)
{
	if (batch && !ask_pass)
		return cfg.challenge_password;
	else
		return getpass("Enter a challenge password: ");
}

void get_crl_dist_point_set(gnutls_x509_crt_t crt)
{
	int ret = 0, i;

	if (batch) {
		if (!cfg.crl_dist_points)
			return;

		for (i = 0; cfg.crl_dist_points[i] != NULL; i++) {
			ret =
			    gnutls_x509_crt_set_crl_dist_points
			    (crt, GNUTLS_SAN_URI, cfg.crl_dist_points[i],
			     0);
			if (ret < 0)
				break;
		}
	} else {
		const char *p;
		unsigned int counter = 0;

		do {
			if (counter == 0) {
				p = read_str
				    ("Enter the URI of the CRL distribution point: ");
			} else {
				p = read_str
				    ("Enter an additional URI of the CRL distribution point: ");
			}
			if (!p)
				return;

			ret = gnutls_x509_crt_set_crl_dist_points
			    (crt, GNUTLS_SAN_URI, p, 0);
			if (ret < 0)
				break;

			counter++;
		}
		while (p);
	}

	if (ret < 0) {
		fprintf(stderr, "gnutls_x509_crt_set_crl_dist_points: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}
}

void get_country_crt_set(gnutls_x509_crt_t crt)
{
	int ret;

	if (batch) {
		if (!cfg.country)
			return;
		ret =
		    gnutls_x509_crt_set_dn_by_oid(crt,
						  GNUTLS_OID_X520_COUNTRY_NAME,
						  0, cfg.country,
						  strlen(cfg.country));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crt_set(crt, "Country name (2 chars): ",
			     GNUTLS_OID_X520_COUNTRY_NAME);
	}

}

void get_organization_crt_set(gnutls_x509_crt_t crt)
{
	int ret;
	unsigned i;

	if (batch) {
		if (!cfg.organization)
			return;

		for (i = 0; cfg.organization[i] != NULL; i++) {
			ret =
			    gnutls_x509_crt_set_dn_by_oid(crt,
						  GNUTLS_OID_X520_ORGANIZATION_NAME,
						  0, cfg.organization[i],
						  strlen(cfg.organization[i]));
			if (ret < 0) {
				fprintf(stderr, "set_dn: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}
	} else {
		read_crt_set(crt, "Organization name: ",
			     GNUTLS_OID_X520_ORGANIZATION_NAME);
	}

}

void get_unit_crt_set(gnutls_x509_crt_t crt)
{
	int ret;
	unsigned i;

	if (batch) {
		if (!cfg.unit)
			return;

		for (i = 0; cfg.unit[i] != NULL; i++) {
			ret =
			    gnutls_x509_crt_set_dn_by_oid(crt,
						  GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME,
						  0, cfg.unit[i],
						  strlen(cfg.unit[i]));
			if (ret < 0) {
				fprintf(stderr, "set_dn: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}
	} else {
		read_crt_set(crt, "Organizational unit name: ",
			     GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME);
	}

}

void get_state_crt_set(gnutls_x509_crt_t crt)
{
	int ret;

	if (batch) {
		if (!cfg.state)
			return;
		ret =
		    gnutls_x509_crt_set_dn_by_oid(crt,
						  GNUTLS_OID_X520_STATE_OR_PROVINCE_NAME,
						  0, cfg.state,
						  strlen(cfg.state));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crt_set(crt, "State or province name: ",
			     GNUTLS_OID_X520_STATE_OR_PROVINCE_NAME);
	}

}

void get_locality_crt_set(gnutls_x509_crt_t crt)
{
	int ret;

	if (batch) {
		if (!cfg.locality)
			return;
		ret =
		    gnutls_x509_crt_set_dn_by_oid(crt,
						  GNUTLS_OID_X520_LOCALITY_NAME,
						  0, cfg.locality,
						  strlen(cfg.locality));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crt_set(crt, "Locality name: ",
			     GNUTLS_OID_X520_LOCALITY_NAME);
	}

}

void get_cn_crt_set(gnutls_x509_crt_t crt)
{
	int ret;

	if (batch) {
		if (!cfg.cn)
			return;
		ret =
		    gnutls_x509_crt_set_dn_by_oid(crt,
						  GNUTLS_OID_X520_COMMON_NAME,
						  0, cfg.cn,
						  strlen(cfg.cn));
		if (ret < 0) {
			fprintf(stderr, "set_dn_by_oid: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crt_set(crt, "Common name: ",
			     GNUTLS_OID_X520_COMMON_NAME);
	}

}

void get_dn_crt_set(gnutls_x509_crt_t crt)
{
	int ret;
	const char *err;

	if (batch) {
		if (!cfg.dn)
			return;
		ret = gnutls_x509_crt_set_dn(crt, cfg.dn, &err);
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s at: %s\n",
				gnutls_strerror(ret), err);
			exit(1);
		}
	}
}

void crt_constraints_set(gnutls_x509_crt_t crt)
{
	int ret;
	unsigned i;
	gnutls_x509_name_constraints_t nc;
	gnutls_datum_t name;

	if (batch) {
		if (cfg.permitted_nc_dns == NULL && cfg.permitted_nc_email == NULL &&
			cfg.excluded_nc_dns == NULL && cfg.excluded_nc_email == NULL &&
			cfg.permitted_nc_ip == NULL && cfg.excluded_nc_ip == NULL)
			return; /* nothing to do */

		ret = gnutls_x509_name_constraints_init(&nc);
		if (ret < 0) {
			fprintf(stderr, "nc_init: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		if (cfg.permitted_nc_ip) {
			for (i = 0; cfg.permitted_nc_ip[i] != NULL; i++) {
				ret = gnutls_x509_cidr_to_rfc5280(cfg.permitted_nc_ip[i], &name);
				if (ret < 0) {
					fprintf(stderr, "error parsing IP constraint: %s\n", gnutls_strerror(ret));
					exit(1);
				}
				ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_IPADDRESS, &name);
				if (ret < 0) {
					fprintf(stderr, "error adding constraint: %s\n", gnutls_strerror(ret));
					exit(1);
				}
				free(name.data);
			}
		}

		if (cfg.excluded_nc_ip) {
			for (i = 0; cfg.excluded_nc_ip[i] != NULL; i++) {
				ret = gnutls_x509_cidr_to_rfc5280(cfg.excluded_nc_ip[i], &name);
				if (ret < 0) {
					fprintf(stderr, "error parsing IP constraint: %s\n", gnutls_strerror(ret));
					exit(1);
				}
				ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_IPADDRESS, &name);
				if (ret < 0) {
					fprintf(stderr, "error adding constraint: %s\n", gnutls_strerror(ret));
					exit(1);
				}
				free(name.data);
			}
		}

		if (cfg.permitted_nc_dns) {

			for (i = 0; cfg.permitted_nc_dns[i] != NULL; i++) {

				name.data = (void*)cfg.permitted_nc_dns[i];
				name.size = strlen((char*)name.data);
				ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_DNSNAME, &name);
				if (ret < 0) {
					fprintf(stderr, "error adding constraint: %s\n", gnutls_strerror(ret));
					exit(1);
				}
			}
		}


		if (cfg.excluded_nc_dns) {
			for (i = 0; cfg.excluded_nc_dns[i] != NULL; i++) {
				name.data = (void*)cfg.excluded_nc_dns[i];
				name.size = strlen((char*)name.data);
				ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_DNSNAME, &name);
				if (ret < 0) {
					fprintf(stderr, "error adding constraint: %s\n", gnutls_strerror(ret));
					exit(1);
				}
			}
		}

		if (cfg.permitted_nc_email) {
			for (i = 0; cfg.permitted_nc_email[i] != NULL; i++) {
				name.data = (void*)cfg.permitted_nc_email[i];
				name.size = strlen((char*)name.data);
				ret = gnutls_x509_name_constraints_add_permitted(nc, GNUTLS_SAN_RFC822NAME, &name);
				if (ret < 0) {
					fprintf(stderr, "error adding constraint: %s\n", gnutls_strerror(ret));
					exit(1);
				}
			}
		}

		if (cfg.excluded_nc_email) {
			for (i = 0; cfg.excluded_nc_email[i] != NULL; i++) {
				name.data = (void*)cfg.excluded_nc_email[i];
				name.size = strlen((char*)name.data);
				ret = gnutls_x509_name_constraints_add_excluded(nc, GNUTLS_SAN_RFC822NAME, &name);
				if (ret < 0) {
					fprintf(stderr, "error adding constraint: %s\n", gnutls_strerror(ret));
					exit(1);
				}
			}
		}

		ret = gnutls_x509_crt_set_name_constraints(crt, nc, 1);
		if (ret < 0) {
			fprintf(stderr, "error setting constraints: %s\n", gnutls_strerror(ret));
			exit(1);
		}

		gnutls_x509_name_constraints_deinit(nc);
	}
}

void crt_unique_ids_set(gnutls_x509_crt_t crt)
{
	int ret;

	if (batch) {
		if (cfg.subject_unique_id == NULL && cfg.issuer_unique_id == NULL)
			return; /* nothing to do */

		if (cfg.subject_unique_id) {
			ret = gnutls_x509_crt_set_subject_unique_id(crt, cfg.subject_unique_id, cfg.subject_unique_id_size);
			if (ret < 0) {
				fprintf(stderr, "error setting subject unique ID: %s\n", gnutls_strerror(ret));
				exit(1);
			}
		}

		if (cfg.issuer_unique_id) {
			ret = gnutls_x509_crt_set_issuer_unique_id(crt, cfg.issuer_unique_id, cfg.issuer_unique_id_size);
			if (ret < 0) {
				fprintf(stderr, "error setting issuer unique ID: %s\n", gnutls_strerror(ret));
				exit(1);
			}
		}
	}
}

void get_uid_crt_set(gnutls_x509_crt_t crt)
{
	int ret;

	if (batch) {
		if (!cfg.uid)
			return;
		ret =
		    gnutls_x509_crt_set_dn_by_oid(crt, GNUTLS_OID_LDAP_UID,
						  0, cfg.uid,
						  strlen(cfg.uid));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crt_set(crt, "UID: ", GNUTLS_OID_LDAP_UID);
	}

}

void get_oid_crt_set(gnutls_x509_crt_t crt)
{
	int ret, i;

	if (batch) {
		if (!cfg.dn_oid)
			return;
		for (i = 0; cfg.dn_oid[i] != NULL; i += 2) {
			if (cfg.dn_oid[i + 1] == NULL) {
				fprintf(stderr,
					"dn_oid: %s does not have an argument.\n",
					cfg.dn_oid[i]);
				exit(1);
			}
			ret =
			    gnutls_x509_crt_set_dn_by_oid(crt,
							  cfg.dn_oid[i], 0,
							  cfg.dn_oid[i +
								     1],
							  strlen(cfg.
								 dn_oid[i +
									1]));

			if (ret < 0) {
				fprintf(stderr, "set_dn_oid: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}
	}
}

#define ACTION_NONE  0
#define ENCODE_OCTET_STRING 1
static unsigned char *decode_ext_string(char *str, unsigned int *ret_size)
{
	char *p, *p2;
	unsigned char *tmp;
	unsigned char *raw;
	unsigned int raw_size;
	unsigned action = ACTION_NONE;
	unsigned char tag[ASN1_MAX_TL_SIZE];
	unsigned int tag_len;
	int ret, res;

	p = strchr(str, '(');
	if (p != 0) {
		if (strncmp(str, "octet_string", 12) == 0) {
			action = ENCODE_OCTET_STRING;
		} else {
			fprintf(stderr, "cannot parse: %s\n", str);
			exit(1);
		}
		p++;
		p2 = strchr(p, ')');
		if (p2 == NULL) {
			fprintf(stderr, "there is no terminating parenthesis in: %s\n", str);
			exit(1);
		}
		*p2 = 0;
	} else {
		p = str;
	}

	if (strncmp(p, "0x", 2) == 0)
		p+=2;
	HEX_DECODE(p, raw, raw_size);

	switch(action) {
		case ENCODE_OCTET_STRING:
			tag_len = sizeof(tag);
			res = asn1_encode_simple_der(ASN1_ETYPE_OCTET_STRING, raw, raw_size, tag, &tag_len);
			if (res != ASN1_SUCCESS) {
				fprintf(stderr, "error in DER encoding: %s\n", asn1_strerror(res));
				exit(1);
			}
			tmp = gnutls_malloc(raw_size+tag_len);
			if (tmp == NULL) {
				fprintf(stderr, "error in allocation\n");
				exit(1);
			}
			memcpy(tmp, tag, tag_len);
			memcpy(tmp+tag_len, raw, raw_size);
			gnutls_free(raw);
			raw = tmp;
			raw_size += tag_len;
			break;
	}

	*ret_size = raw_size;
	return raw;
}

void get_extensions_crt_set(int type, void *crt)
{
	int ret, i;
	unsigned char *raw = NULL;
	unsigned raw_size;

	if (batch) {
		if (!cfg.extensions)
			goto check_critical;
		for (i = 0; cfg.extensions[i] != NULL; i += 2) {
			if (cfg.extensions[i + 1] == NULL) {
				fprintf(stderr,
					"extensions: %s does not have an argument.\n",
					cfg.extensions[i]);
				exit(1);
			}

			/* convert hex to bin */
			raw = decode_ext_string(cfg.extensions[i+1], &raw_size);

			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_extension_by_oid(crt,
							  cfg.extensions[i],
							  raw, raw_size, 0);
			else
				ret =
				    gnutls_x509_crq_set_extension_by_oid(crt,
							  cfg.extensions[i],
							  raw, raw_size, 0);

			gnutls_free(raw);
			if (ret < 0) {
				fprintf(stderr, "set_extensions: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}

 check_critical:
		if (!cfg.crit_extensions)
			return;
		for (i = 0; cfg.crit_extensions[i] != NULL; i += 2) {
			if (cfg.crit_extensions[i + 1] == NULL) {
				fprintf(stderr,
					"extensions: %s does not have an argument.\n",
					cfg.crit_extensions[i]);
				exit(1);
			}
			/* convert hex to bin */
			raw = decode_ext_string(cfg.crit_extensions[i+1], &raw_size);

			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_extension_by_oid(crt,
							  cfg.crit_extensions[i],
							  raw, raw_size, 1);
			else
				ret =
				    gnutls_x509_crq_set_extension_by_oid(crt,
							  cfg.crit_extensions[i],
							  raw, raw_size, 1);

			gnutls_free(raw);

			if (ret < 0) {
				fprintf(stderr, "set_extensions: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}
	}
}

void get_key_purpose_set(int type, void *crt)
{
	int ret, i;

	if (batch) {
		if (!cfg.key_purpose_oids)
			return;
		for (i = 0; cfg.key_purpose_oids[i] != NULL; i++) {
			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_key_purpose_oid
				    (crt, cfg.key_purpose_oids[i], 0);
			else
				ret =
				    gnutls_x509_crq_set_key_purpose_oid
				    (crt, cfg.key_purpose_oids[i], 0);

			if (ret < 0) {
				fprintf(stderr,
					"set_key_purpose_oid (%s): %s\n",
					cfg.key_purpose_oids[i],
					gnutls_strerror(ret));
				exit(1);
			}
		}
	}
}

void get_ocsp_issuer_set(gnutls_x509_crt_t crt)
{
	int ret, i;
	gnutls_datum_t uri;

	if (batch) {
		if (!cfg.ocsp_uris)
			return;
		for (i = 0; cfg.ocsp_uris[i] != NULL; i++) {
			uri.data = (void*)cfg.ocsp_uris[i];
			uri.size = strlen(cfg.ocsp_uris[i]);
			ret =
			    gnutls_x509_crt_set_authority_info_access(crt,
								      GNUTLS_IA_OCSP_URI,
								      &uri);
			if (ret < 0) {
				fprintf(stderr, "set OCSP URI (%s): %s\n",
					cfg.ocsp_uris[i],
					gnutls_strerror(ret));
				exit(1);
			}
		}
	}
}

void get_ca_issuers_set(gnutls_x509_crt_t crt)
{
	int ret, i;
	gnutls_datum_t uri;

	if (batch) {
		if (!cfg.ca_issuers_uris)
			return;
		for (i = 0; cfg.ca_issuers_uris[i] != NULL; i++) {
			uri.data = (void*)cfg.ca_issuers_uris[i];
			uri.size = strlen(cfg.ca_issuers_uris[i]);
			ret =
			    gnutls_x509_crt_set_authority_info_access(crt,
								      GNUTLS_IA_CAISSUERS_URI,
								      &uri);
			if (ret < 0) {
				fprintf(stderr,
					"set CA ISSUERS URI (%s): %s\n",
					cfg.ca_issuers_uris[i],
					gnutls_strerror(ret));
				exit(1);
			}
		}
	}
}


void get_pkcs9_email_crt_set(gnutls_x509_crt_t crt)
{
	int ret;

	if (batch) {
		if (!cfg.pkcs9_email)
			return;
		ret =
		    gnutls_x509_crt_set_dn_by_oid(crt,
						  GNUTLS_OID_PKCS9_EMAIL,
						  0, cfg.pkcs9_email,
						  strlen(cfg.pkcs9_email));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crt_set(crt, "E-mail: ", GNUTLS_OID_PKCS9_EMAIL);
	}

}


static
int default_crl_number(unsigned char* serial, size_t *size)
{
	struct timespec ts;
	time_t tv_sec_tmp;
	int i;

	/* default format:
	 * |  5 b |  4 b  |  11b
	 * | secs | nsecs | rnd  |
	 */
	gettime(&ts);

	if (*size < 20) {
		return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

	tv_sec_tmp = ts.tv_sec;
	for (i = 4; i >= 0; i--) {
		serial[i] = tv_sec_tmp & 0xff;
		tv_sec_tmp = tv_sec_tmp >> 8;
	}
	serial[5] = (ts.tv_nsec >> 24) & 0xff;
	serial[6] = (ts.tv_nsec >> 16) & 0xff;
	serial[7] = (ts.tv_nsec >> 8) & 0xff;
	serial[8] = (ts.tv_nsec) & 0xff;
	// crl number must be positive and max 20 octets
	// so we must zero the most significant bit (with MSB set, the DER encoding
	// would be 21 octets long). See RFC 5280, section 5.2.3.
	serial[0] &= 0x7F;
	*size = 20;
	return gnutls_rnd(GNUTLS_RND_NONCE, &serial[9], 11);
}

/**
 * read_serial_value:
 * @serial: pointer to buffer with serial number
 * @size: pointer to actual size of data in buffer
 * @max_size: capacity of the buffer
 * @label: user-facing description of the field we are reading value for
 * @rfc_section: rfc section number that defines the field
 *
 * This function will read a serial number from the user. It takes a buffer
 * that contains a default value that will be displayed to the user and
 * maximum size of the buffer that it can fill. When the function
 * returns, either the buffer is not modified to use the default value
 * or it's contents are changed to reflect the user-entered value.
 **/
static
void read_serial_value(unsigned char *serial, size_t *size, size_t max_size,
		const char *label, const char *rfc_section)
{
	static char input[MAX_INPUT_SIZE];
	int ret;
	size_t input_len;
	gnutls_datum_t decoded;
	gnutls_datum_t serial_datum;
	gnutls_datum_t encoded_default;

	serial_datum.data = serial;
	serial_datum.size = *size;

	ret = gnutls_hex_encode2(&serial_datum, &encoded_default);
	if (ret < 0) {
		fprintf(stderr, "error encoding default to hex: %d\n", ret);
		exit(1);
	}

	while (true) {
		fprintf(stderr,
			"Enter the %s in decimal (123) or hex (0xabcd)\n"
			"(default is 0x%s)\n"
			"value: ",
			label, encoded_default.data);

		if (fgets(input, sizeof(input), stdin) == NULL)
			break;

		input_len = strip_nl(input, strlen(input));

		if (input_len == 0)
			break;

		ret = serial_decode(input, &decoded);
		if (ret < 0) {
			fprintf(stderr, "error parsing %s: %s\n", label, input);
			continue;
		}

		if ((decoded.size == SERIAL_MAX_BYTES && decoded.data[0] & 0x80) ||
				decoded.size > SERIAL_MAX_BYTES) {
			fprintf(stderr, "%s would be encoded in more than 20 bytes,"
				"see RFC 5280, section %s\n", label, rfc_section);
			gnutls_free(decoded.data);
			continue;
		}

		if (decoded.size > max_size) {
			fprintf(stderr, "maximum %zu octets allowed for %s\n",
					max_size, label);
			gnutls_free(decoded.data);
			continue;
		}

		memcpy(serial, decoded.data, decoded.size);
		*size = decoded.size;
		gnutls_free(decoded.data);
		break;
	}

	gnutls_free(encoded_default.data);
}

static
void get_serial_value(unsigned char *serial, size_t *size,
		const unsigned char *config, size_t config_size,
		int (create_default)(unsigned char *, size_t *),
		const char *label, const char *rfc_section)
{
	size_t max_size = *size;
	int ret;

	if (batch && config != NULL) {
		if (config_size > max_size) {
			fprintf(stderr, "maximum %zu octets allowed for %s!\n",
					max_size, label);
			exit(1);
		}
		memcpy(serial, config, config_size);
		*size = config_size;
	} else {
		ret = create_default(serial, size);
		if (ret < 0) {
			fprintf(stderr, "error generating default %s: %s\n",
					label, gnutls_strerror(ret));
			exit(1);
		}
	}

	if (!batch)
		read_serial_value(serial, size, max_size, label, rfc_section);

	if ((*size == SERIAL_MAX_BYTES && serial[0] & 0x80) || *size > SERIAL_MAX_BYTES) {
		fprintf(stderr, "%s would be encoded in more than 20 bytes,"
				"see RFC 5280, section %s\n", label, rfc_section);
		exit(1);
	}
}

static
int default_serial(unsigned char *serial, size_t *size)
{
	int ret;

	if (*size < SERIAL_MAX_BYTES)
		return GNUTLS_E_SHORT_MEMORY_BUFFER;

	ret = gnutls_rnd(GNUTLS_RND_NONCE, serial, SERIAL_MAX_BYTES);
	if (ret < 0)
		return ret;

	// serial must be positive and max 20 octets
	// so we must zero the most significant bit (with MSB set, the DER encoding
	// would be 21 octets long). See RFC 5280, section 4.1.2.2.
	serial[0] &= 0x7F;
	*size = SERIAL_MAX_BYTES;

	return 0;
}

void get_serial(unsigned char *serial, size_t *size)
{
	get_serial_value(serial, size, cfg.serial, cfg.serial_size,
			default_serial, "certificate's serial number", "4.1.2.2");
}

static
time_t get_date(const char* date)
{
	struct timespec r;

	if (date==NULL || parse_datetime(&r, date, NULL) == 0) {
		PRINT_TIME_T_ERROR;
		fprintf(stderr, "Cannot parse date: %s\n", date);
		exit(1);
	}

	return r.tv_sec;
}

time_t get_activation_date(void)
{

	if (batch && cfg.activation_date != NULL) {
		return get_date(cfg.activation_date);
	}

	return time(NULL);
}

time_t get_crl_revocation_date(void)
{

	if (batch && cfg.revocation_date != NULL) {
		return get_date(cfg.revocation_date);
	}

	return time(NULL);
}

time_t get_crl_this_update_date(void)
{

	if (batch && cfg.this_update_date != NULL) {
		return get_date(cfg.this_update_date);
	}

	return time(NULL);
}

static
time_t days_to_secs(int days)
{
time_t secs = days;
time_t now = time(NULL);

	if (secs != (time_t)-1) {
		if (INT_MULTIPLY_OVERFLOW(secs, 24*60*60)) {
			goto overflow;
		} else {
			secs *= 24*60*60;
		}
	}

	if (secs != (time_t)-1) {
		if (INT_ADD_OVERFLOW(secs, now)) {
			goto overflow;
		} else {
			secs += now;
		}
	}

	return secs;
 overflow:
	PRINT_TIME_T_ERROR;
	fprintf(stderr, "Overflow while parsing days\n");
	exit(1);
}

static
time_t get_int_date(const char *txt_val, int int_val, const char *msg)
{
	if (batch) {
		if (txt_val == NULL) {
			time_t secs;

			if (int_val == 0 || int_val < -2)
				secs = days_to_secs(365);
			else {
				secs = days_to_secs(int_val);
			}

			return secs;
		} else
			return get_date(txt_val);
	} else {
		int days;

		do {
			days =
			    read_int(msg);
		}
		while (days == 0);
		return days_to_secs(days);
	}
}

time_t get_expiration_date(void)
{
	return get_int_date(cfg.expiration_date, cfg.expiration_days, "The certificate will expire in (days): ");
}

int get_ca_status(void)
{
	if (batch) {
		return cfg.ca;
	} else {
		return
		    read_yesno
		    ("Does the certificate belong to an authority? (y/N): ",
		     0);
	}
}

int get_crq_extensions_status(void)
{
	if (batch) {
		return cfg.honor_crq_extensions;
	} else {
		return
		    read_yesno
		    ("Do you want to honour all the extensions from the request? (y/N): ",
		     0);
	}
}

void get_crl_number(unsigned char* serial, size_t * size)
{
	get_serial_value(serial, size, cfg.crl_number, cfg.crl_number_size,
			default_crl_number, "CRL's serial number", "5.2.3");
}

int get_path_len(void)
{
	if (batch) {
		return cfg.path_len;
	} else {
		return read_int_with_default
		    ("Path length constraint (decimal, %d for no constraint): ",
		     -1);
	}
}

const char *get_pkcs12_key_name(void)
{
	const char *name;

	if (batch) {
		if (!cfg.pkcs12_key_name)
			return "Anonymous";
		return cfg.pkcs12_key_name;
	} else {
		do {
			name = read_str("Enter a name for the key: ");
		}
		while (name == NULL);
	}
	return name;
}

int get_tls_client_status(void)
{
	if (batch) {
		return cfg.tls_www_client;
	} else {
		return
		    read_yesno
		    ("Is this a TLS web client certificate? (y/N): ", 0);
	}
}

int get_tls_server_status(void)
{
	if (batch) {
		return cfg.tls_www_server;
	} else {
		return
		    read_yesno
		    ("Is this a TLS web server certificate? (y/N): ", 0);
	}
}

/* convert a printable IP to binary */
static int string_to_ip(unsigned char *ip, const char *str)
{
	int len = strlen(str);
	int ret;

#if HAVE_IPV6
	if (strchr(str, ':') != NULL || len > 16) {	/* IPv6 */
		ret = inet_pton(AF_INET6, str, ip);
		if (ret <= 0) {
			fprintf(stderr, "Error in IPv6 address %s\n", str);
			exit(1);
		}

		/* To be done */
		return 16;
	} else
#endif
	{			/* IPv4 */
		ret = inet_pton(AF_INET, str, ip);
		if (ret <= 0) {
			fprintf(stderr, "Error in IPv4 address %s\n", str);
			exit(1);
		}

		return 4;
	}

}

void get_ip_addr_set(int type, void *crt)
{
	int ret = 0, i;
	unsigned char ip[16];
	int len;

	if (batch) {
		if (!cfg.ip_addr)
			return;

		for (i = 0; cfg.ip_addr[i] != NULL; i++) {
			len = string_to_ip(ip, cfg.ip_addr[i]);
			if (len <= 0) {
				fprintf(stderr,
					"Error parsing address: %s\n",
					cfg.ip_addr[i]);
				exit(1);
			}

			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_subject_alt_name
				    (crt, GNUTLS_SAN_IPADDRESS, ip, len,
				     GNUTLS_FSAN_APPEND);
			else
				ret =
				    gnutls_x509_crq_set_subject_alt_name
				    (crt, GNUTLS_SAN_IPADDRESS, ip, len,
				     GNUTLS_FSAN_APPEND);

			if (ret < 0)
				break;
		}
	} else {
		const char *p;

		p = read_str
		    ("Enter the IP address of the subject of the certificate: ");
		if (!p)
			return;

		len = string_to_ip(ip, p);
		if (len <= 0) {
			fprintf(stderr, "Error parsing address: %s\n", p);
			exit(1);
		}

		if (type == TYPE_CRT)
			ret =
			    gnutls_x509_crt_set_subject_alt_name(crt,
								 GNUTLS_SAN_IPADDRESS,
								 ip, len,
								 GNUTLS_FSAN_APPEND);
		else
			ret =
			    gnutls_x509_crq_set_subject_alt_name(crt,
								 GNUTLS_SAN_IPADDRESS,
								 ip, len,
								 GNUTLS_FSAN_APPEND);
	}

	if (ret < 0) {
		fprintf(stderr, "set_subject_alt_name: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}
}

void get_email_set(int type, void *crt)
{
	int ret = 0, i;

	if (batch) {
		if (!cfg.email)
			return;

		for (i = 0; cfg.email[i] != NULL; i++) {
			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_subject_alt_name
				    (crt, GNUTLS_SAN_RFC822NAME,
				     cfg.email[i], strlen(cfg.email[i]),
				     GNUTLS_FSAN_APPEND);
			else
				ret =
				    gnutls_x509_crq_set_subject_alt_name
				    (crt, GNUTLS_SAN_RFC822NAME,
				     cfg.email[i], strlen(cfg.email[i]),
				     GNUTLS_FSAN_APPEND);

			if (ret < 0)
				break;
		}
	} else {
		const char *p;

		p = read_str
		    ("Enter the e-mail of the subject of the certificate: ");
		if (!p)
			return;

		if (type == TYPE_CRT)
			ret =
			    gnutls_x509_crt_set_subject_alt_name(crt,
								 GNUTLS_SAN_RFC822NAME,
								 p,
								 strlen(p),
								 GNUTLS_FSAN_APPEND);
		else
			ret =
			    gnutls_x509_crq_set_subject_alt_name(crt,
								 GNUTLS_SAN_RFC822NAME,
								 p,
								 strlen(p),
								 GNUTLS_FSAN_APPEND);
	}

	if (ret < 0) {
		fprintf(stderr, "set_subject_alt_name: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}
}


void get_dc_set(int type, void *crt)
{
	int ret = 0, i;

	if (batch) {
		if (!cfg.dc)
			return;

		for (i = 0; cfg.dc[i] != NULL; i++) {
			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_dn_by_oid(crt,
								  GNUTLS_OID_LDAP_DC,
								  0,
								  cfg.
								  dc[i],
								  strlen
								  (cfg.
								   dc[i]));
			else
				ret =
				    gnutls_x509_crq_set_dn_by_oid(crt,
								  GNUTLS_OID_LDAP_DC,
								  0,
								  cfg.
								  dc[i],
								  strlen
								  (cfg.
								   dc[i]));

			if (ret < 0)
				break;
		}
	} else {
		const char *p;
		unsigned int counter = 0;

		do {
			if (counter == 0) {
				p = read_str
				    ("Enter the subject's domain component (DC): ");
			} else {
				p = read_str
				    ("Enter an additional domain component (DC): ");
			}
			if (!p)
				return;

			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_dn_by_oid(crt,
								  GNUTLS_OID_LDAP_DC,
								  0, p,
								  strlen
								  (p));
			else
				ret =
				    gnutls_x509_crq_set_dn_by_oid(crt,
								  GNUTLS_OID_LDAP_DC,
								  0, p,
								  strlen
								  (p));
			counter++;
			if (ret < 0)
				break;
		}
		while (p != NULL);
	}

	if (ret < 0) {
		fprintf(stderr, "set_dn_by_oid: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}
}

void get_dns_name_set(int type, void *crt)
{
	int ret = 0, i;

	if (batch) {
		if (!cfg.dns_name)
			return;

		for (i = 0; cfg.dns_name[i] != NULL; i++) {
			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_subject_alt_name
				    (crt, GNUTLS_SAN_DNSNAME,
				     cfg.dns_name[i],
				     strlen(cfg.dns_name[i]),
				     GNUTLS_FSAN_APPEND);
			else
				ret =
				    gnutls_x509_crq_set_subject_alt_name
				    (crt, GNUTLS_SAN_DNSNAME,
				     cfg.dns_name[i],
				     strlen(cfg.dns_name[i]),
				     GNUTLS_FSAN_APPEND);

			if (ret < 0)
				break;
		}
	} else {
		const char *p;
		unsigned int counter = 0;

		do {
			if (counter == 0) {
				p = read_str("Enter a dnsName of the subject of the certificate: ");
			} else {
				p = read_str("Enter an additional dnsName of the subject of the certificate: ");
			}
			if (!p)
				return;

			if (type == TYPE_CRT)
				ret = gnutls_x509_crt_set_subject_alt_name
				    (crt, GNUTLS_SAN_DNSNAME, p, strlen(p),
				     GNUTLS_FSAN_APPEND);
			else
				ret = gnutls_x509_crq_set_subject_alt_name
				    (crt, GNUTLS_SAN_DNSNAME, p, strlen(p),
				     GNUTLS_FSAN_APPEND);
			counter++;
		} while (p);
	}

	if (ret < 0) {
		fprintf(stderr, "set_subject_alt_name: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}
}

static int set_krb5_principal(int type, void *crt)
{
	int ret = 0, i;

	if (batch) {
		if (!cfg.krb5_principal)
			return 0;

		for (i = 0; cfg.krb5_principal[i] != NULL; i ++) {
			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_subject_alt_name
				    (crt, GNUTLS_SAN_OTHERNAME_KRB5PRINCIPAL,
				     cfg.krb5_principal[i], strlen(cfg.krb5_principal[i]),
				     GNUTLS_FSAN_APPEND);
			else
				ret =
				    gnutls_x509_crq_set_subject_alt_name
				    (crt, GNUTLS_SAN_OTHERNAME_KRB5PRINCIPAL,
				     cfg.krb5_principal[i], strlen(cfg.krb5_principal[i]),
				     GNUTLS_FSAN_APPEND);

			if (ret < 0)
				break;
		}
	}

	if (ret < 0) {
		fprintf(stderr, "set_subject_alt_name(GNUTLS_SAN_OTHERNAME_KRB5PRINCIPAL): %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	return ret;
}

static int set_othername(int type, void *crt)
{
	int ret = 0, i;
	uint8_t *binname = NULL;
	unsigned binnamelen = 0;
	const char *oid;

	if (batch) {
		if (!cfg.other_name)
			return 0;

		for (i = 0; cfg.other_name[i] != NULL; i += 2) {
			oid = cfg.other_name[i];

			if (cfg.other_name[i + 1] == NULL) {
				fprintf(stderr,
					"other_name: %s does not have an argument.\n",
					cfg.other_name[i]);
				exit(1);
			}

			HEX_DECODE (cfg.other_name[i+1], binname, binnamelen);
			if (binnamelen == 0)
				break;

			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_subject_alt_othername
				    (crt, oid,
				     binname, binnamelen,
				     GNUTLS_FSAN_APPEND);
			else
				ret =
				    gnutls_x509_crq_set_subject_alt_othername
				    (crt, oid,
				     binname, binnamelen,
				     GNUTLS_FSAN_APPEND);
			free (binname);
			binname = NULL;

			if (ret < 0)
				break;
		}
	}

	if (ret < 0) {
		fprintf(stderr, "set_subject_alt_othername: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	return ret;
}

static int set_othername_utf8(int type, void *crt)
{
	int ret = 0, i;
	const char *oid;

	if (batch) {
		if (!cfg.other_name_utf8)
			return 0;

		for (i = 0; cfg.other_name_utf8[i] != NULL; i += 2) {
			oid = cfg.other_name_utf8[i];

			if (cfg.other_name_utf8[i + 1] == NULL) {
				fprintf(stderr,
					"other_name_utf8: %s does not have an argument.\n",
					cfg.other_name_utf8[i]);
				exit(1);
			}

			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_subject_alt_othername
				    (crt, oid,
				     cfg.other_name_utf8[i + 1], strlen(cfg.other_name_utf8[i + 1]),
				     GNUTLS_FSAN_APPEND|GNUTLS_FSAN_ENCODE_UTF8_STRING);
			else
				ret =
				    gnutls_x509_crq_set_subject_alt_othername
				    (crt, oid,
				     cfg.other_name_utf8[i + 1], strlen(cfg.other_name_utf8[i + 1]),
				     GNUTLS_FSAN_APPEND|GNUTLS_FSAN_ENCODE_UTF8_STRING);

			if (ret < 0)
				break;
		}
	}

	if (ret < 0) {
		fprintf(stderr, "set_subject_alt_othername: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	return ret;
}

static int set_othername_octet(int type, void *crt)
{
	int ret = 0, i;
	const char *oid;

	if (batch) {
		if (!cfg.other_name_octet)
			return 0;

		for (i = 0; cfg.other_name_octet[i] != NULL; i += 2) {
			oid = cfg.other_name_octet[i];

			if (cfg.other_name_octet[i + 1] == NULL) {
				fprintf(stderr,
					"other_name_octet: %s does not have an argument.\n",
					cfg.other_name_octet[i]);
				exit(1);
			}

			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_subject_alt_othername
				    (crt, oid,
				     cfg.other_name_octet[i + 1], strlen(cfg.other_name_octet[i + 1]),
				     GNUTLS_FSAN_APPEND|GNUTLS_FSAN_ENCODE_OCTET_STRING);
			else
				ret =
				    gnutls_x509_crq_set_subject_alt_othername
				    (crt, oid,
				     cfg.other_name_octet[i + 1], strlen(cfg.other_name_octet[i + 1]),
				     GNUTLS_FSAN_APPEND|GNUTLS_FSAN_ENCODE_OCTET_STRING);

			if (ret < 0)
				break;
		}
	}

	if (ret < 0) {
		fprintf(stderr, "set_subject_alt_othername: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	return ret;
}

static int set_xmpp_name(int type, void *crt)
{
	int ret = 0, i;

	if (batch) {
		if (!cfg.xmpp_name)
			return 0;

		for (i = 0; cfg.xmpp_name[i] != NULL; i ++) {
			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_subject_alt_name
				    (crt, GNUTLS_SAN_OTHERNAME_XMPP,
				     cfg.xmpp_name[i], strlen(cfg.xmpp_name[i]),
				     GNUTLS_FSAN_APPEND);
			else
				ret =
				    gnutls_x509_crq_set_subject_alt_name
				    (crt, GNUTLS_SAN_OTHERNAME_XMPP,
				     cfg.xmpp_name[i], strlen(cfg.xmpp_name[i]),
				     GNUTLS_FSAN_APPEND);

			if (ret < 0)
				break;
		}
	}

	if (ret < 0) {
		fprintf(stderr, "set_subject_alt_name(XMPP): %s\n",
			gnutls_strerror(ret));
		exit(1);
	}

	return ret;
}


void get_other_name_set(int type, void *crt)
{
	set_othername(type, crt);
	set_othername_octet(type, crt);
	set_othername_utf8(type, crt);
	set_xmpp_name(type, crt);
	set_krb5_principal(type, crt);
}

void get_policy_set(gnutls_x509_crt_t crt)
{
	int ret = 0, i;
	gnutls_x509_policy_st policy;

	if (batch) {
		if (cfg.skip_certs >= 0) {
			ret = gnutls_x509_crt_set_inhibit_anypolicy(crt, cfg.skip_certs);
			if (ret < 0) {
				fprintf(stderr, "error setting inhibit anypolicy: %s\n", gnutls_strerror(ret));
				exit(1);
			}
		}

		for (i = 0; cfg.policy_oid[i] != NULL; i++) {
			memset(&policy, 0, sizeof(policy));
			policy.oid = cfg.policy_oid[i];

			if (cfg.policy_txt[i] != NULL) {
				policy.qualifier[policy.qualifiers].type =
				    GNUTLS_X509_QUALIFIER_NOTICE;
				policy.qualifier[policy.qualifiers].data =
				    cfg.policy_txt[i];
				policy.qualifier[policy.qualifiers].size =
				    strlen(cfg.policy_txt[i]);
				policy.qualifiers++;
			}

			if (cfg.policy_url[i] != NULL) {
				policy.qualifier[policy.qualifiers].type =
				    GNUTLS_X509_QUALIFIER_URI;
				policy.qualifier[policy.qualifiers].data =
				    cfg.policy_url[i];
				policy.qualifier[policy.qualifiers].size =
				    strlen(cfg.policy_url[i]);
				policy.qualifiers++;
			}

			ret = gnutls_x509_crt_set_policy(crt, &policy, 0);
			if (ret < 0)
				break;
		}
	}

	if (ret < 0) {
		fprintf(stderr, "set_policy: %s\n", gnutls_strerror(ret));
		exit(1);
	}
}

void get_uri_set(int type, void *crt)
{
	int ret = 0, i;

	if (batch) {
		if (!cfg.uri)
			return;

		for (i = 0; cfg.uri[i] != NULL; i++) {
			if (type == TYPE_CRT)
				ret =
				    gnutls_x509_crt_set_subject_alt_name
				    (crt, GNUTLS_SAN_URI, cfg.uri[i],
				     strlen(cfg.uri[i]),
				     GNUTLS_FSAN_APPEND);
			else
				ret =
				    gnutls_x509_crq_set_subject_alt_name
				    (crt, GNUTLS_SAN_URI, cfg.uri[i],
				     strlen(cfg.uri[i]),
				     GNUTLS_FSAN_APPEND);

			if (ret < 0)
				break;
		}
	} else {
		const char *p;
		unsigned int counter = 0;

		do {
			if (counter == 0) {
				p = read_str
				    ("Enter a URI of the subject of the certificate: ");
			} else {
				p = read_str
				    ("Enter an additional URI of the subject of the certificate: ");
			}
			if (!p)
				return;

			if (type == TYPE_CRT)
				ret = gnutls_x509_crt_set_subject_alt_name
				    (crt, GNUTLS_SAN_URI, p, strlen(p),
				     GNUTLS_FSAN_APPEND);
			else
				ret = gnutls_x509_crq_set_subject_alt_name
				    (crt, GNUTLS_SAN_URI, p, strlen(p),
				     GNUTLS_FSAN_APPEND);
			counter++;
			if (ret < 0)
				break;
		}
		while (p);
	}

	if (ret < 0) {
		fprintf(stderr, "set_subject_alt_name: %s\n",
			gnutls_strerror(ret));
		exit(1);
	}
}



int get_sign_status(int server)
{
	const char *msg;

	if (batch) {
		return cfg.signing_key;
	} else {
		if (server)
			msg =
			    "Will the certificate be used for signing (DHE ciphersuites)? (Y/n): ";
		else
			msg =
			    "Will the certificate be used for signing (required for TLS)? (Y/n): ";
		return read_yesno(msg, 1);
	}
}

int get_encrypt_status(int server)
{
	const char *msg;

	if (batch) {
		return cfg.encryption_key;
	} else {
		if (server)
			msg =
			    "Will the certificate be used for encryption (RSA ciphersuites)? (Y/n): ";
		else
			msg =
			    "Will the certificate be used for encryption (not required for TLS)? (Y/n): ";
		return read_yesno(msg, 1);
	}
}

int get_cert_sign_status(void)
{
	if (batch) {
		return cfg.cert_sign_key;
	} else {
		return
		    read_yesno
		    ("Will the certificate be used to sign other certificates? (Y/n): ",
		     1);
	}
}

int get_crl_sign_status(void)
{
	if (batch) {
		return cfg.crl_sign_key;
	} else {
		return
		    read_yesno
		    ("Will the certificate be used to sign CRLs? (y/N): ",
		     0);
	}
}

int get_key_agreement_status(void)
{
	if (batch) {
		return cfg.key_agreement;
	} else {
		/* this option is not asked in interactive mode */
		return 0;
	}
}

int get_non_repudiation_status(void)
{
	if (batch) {
		return cfg.non_repudiation;
	} else {
		/* this option is not asked in interactive mode */
		return 0;
	}
}

int get_data_encipherment_status(void)
{
	if (batch) {
		return cfg.data_encipherment;
	} else {
		return read_yesno("Will the certificate be used for data encryption? (y/N): ", 0);
	}
}

int get_code_sign_status(void)
{
	if (batch) {
		return cfg.code_sign_key;
	} else {
		return
		    read_yesno
		    ("Will the certificate be used to sign code? (y/N): ",
		     0);
	}
}

int get_ocsp_sign_status(void)
{
	if (batch) {
		return cfg.ocsp_sign_key;
	} else {
		return
		    read_yesno
		    ("Will the certificate be used to sign OCSP requests? (y/N): ",
		     0);
	}
}

int get_time_stamp_status(void)
{
	if (batch) {
		return cfg.time_stamping_key;
	} else {
		return
		    read_yesno
		    ("Will the certificate be used for time stamping? (y/N): ",
		     0);
	}
}

int get_email_protection_status(void)
{
	if (batch) {
		return cfg.email_protection_key;
	} else {
		return
		    read_yesno
		    ("Will the certificate be used for email protection? (y/N): ",
		     0);
	}
}

int get_ipsec_ike_status(void)
{
	if (batch) {
		return cfg.ipsec_ike_key;
	} else {
		return
		    read_yesno
		    ("Will the certificate be used for IPsec IKE operations? (y/N): ",
		     0);
	}
}

time_t get_crl_next_update(void)
{
	return get_int_date(cfg.next_update_date, cfg.crl_next_update, "The next CRL will be issued in (days): ");
}

const char *get_proxy_policy(char **policy, size_t * policylen)
{
	const char *ret;

	if (batch) {
		ret = cfg.proxy_policy_language;
		if (!ret)
			ret = "1.3.6.1.5.5.7.21.1";
	} else {
		do {
			ret =
			    read_str
			    ("Enter the OID of the proxy policy language: ");
		}
		while (ret == NULL);
	}

	*policy = NULL;
	*policylen = 0;

	if (strcmp(ret, "1.3.6.1.5.5.7.21.1") != 0 &&
	    strcmp(ret, "1.3.6.1.5.5.7.21.2") != 0) {
		fprintf(stderr,
			"Reading non-standard proxy policy not supported.\n");
	}

	return ret;
}

/* CRQ stuff.
 */
void get_country_crq_set(gnutls_x509_crq_t crq)
{
	int ret;

	if (batch) {
		if (!cfg.country)
			return;
		ret =
		    gnutls_x509_crq_set_dn_by_oid(crq,
						  GNUTLS_OID_X520_COUNTRY_NAME,
						  0, cfg.country,
						  strlen(cfg.country));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crq_set(crq, "Country name (2 chars): ",
			     GNUTLS_OID_X520_COUNTRY_NAME);
	}

}

void get_organization_crq_set(gnutls_x509_crq_t crq)
{
	int ret;
	unsigned i;

	if (batch) {
		if (!cfg.organization)
			return;

		for (i = 0; cfg.organization[i] != NULL; i++) {
			ret =
			    gnutls_x509_crq_set_dn_by_oid(crq,
						  GNUTLS_OID_X520_ORGANIZATION_NAME,
						  0, cfg.organization[i],
						  strlen(cfg.
							 organization[i]));
			if (ret < 0) {
				fprintf(stderr, "set_dn: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}
	} else {
		read_crq_set(crq, "Organization name: ",
			     GNUTLS_OID_X520_ORGANIZATION_NAME);
	}

}

void get_unit_crq_set(gnutls_x509_crq_t crq)
{
	int ret;
	unsigned i;

	if (batch) {
		if (!cfg.unit)
			return;

		for (i = 0; cfg.unit[i] != NULL; i++) {
			ret =
			    gnutls_x509_crq_set_dn_by_oid(crq,
						  GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME,
						  0, cfg.unit[i],
						  strlen(cfg.unit[i]));
			if (ret < 0) {
				fprintf(stderr, "set_dn: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}
	} else {
		read_crq_set(crq, "Organizational unit name: ",
			     GNUTLS_OID_X520_ORGANIZATIONAL_UNIT_NAME);
	}

}

void get_state_crq_set(gnutls_x509_crq_t crq)
{
	int ret;

	if (batch) {
		if (!cfg.state)
			return;
		ret =
		    gnutls_x509_crq_set_dn_by_oid(crq,
						  GNUTLS_OID_X520_STATE_OR_PROVINCE_NAME,
						  0, cfg.state,
						  strlen(cfg.state));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crq_set(crq, "State or province name: ",
			     GNUTLS_OID_X520_STATE_OR_PROVINCE_NAME);
	}

}

void get_locality_crq_set(gnutls_x509_crq_t crq)
{
	int ret;

	if (batch) {
		if (!cfg.locality)
			return;
		ret =
		    gnutls_x509_crq_set_dn_by_oid(crq,
						  GNUTLS_OID_X520_LOCALITY_NAME,
						  0, cfg.locality,
						  strlen(cfg.locality));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crq_set(crq, "Locality name: ",
			     GNUTLS_OID_X520_LOCALITY_NAME);
	}

}

void get_dn_crq_set(gnutls_x509_crq_t crq)
{
	int ret;
	const char *err;

	if (batch) {
		if (!cfg.dn)
			return;
		ret = gnutls_x509_crq_set_dn(crq, cfg.dn, &err);
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s at: %s\n",
				gnutls_strerror(ret), err);
			exit(1);
		}
	}
}

void get_cn_crq_set(gnutls_x509_crq_t crq)
{
	int ret;

	if (batch) {
		if (!cfg.cn)
			return;
		ret =
		    gnutls_x509_crq_set_dn_by_oid(crq,
						  GNUTLS_OID_X520_COMMON_NAME,
						  0, cfg.cn,
						  strlen(cfg.cn));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crq_set(crq, "Common name: ",
			     GNUTLS_OID_X520_COMMON_NAME);
	}

}

void get_uid_crq_set(gnutls_x509_crq_t crq)
{
	int ret;

	if (batch) {
		if (!cfg.uid)
			return;
		ret =
		    gnutls_x509_crq_set_dn_by_oid(crq, GNUTLS_OID_LDAP_UID,
						  0, cfg.uid,
						  strlen(cfg.uid));
		if (ret < 0) {
			fprintf(stderr, "set_dn: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}
	} else {
		read_crq_set(crq, "UID: ", GNUTLS_OID_LDAP_UID);
	}

}

void get_oid_crq_set(gnutls_x509_crq_t crq)
{
	int ret, i;

	if (batch) {
		if (!cfg.dn_oid)
			return;
		for (i = 0; cfg.dn_oid[i] != NULL; i += 2) {
			if (cfg.dn_oid[i + 1] == NULL) {
				fprintf(stderr,
					"dn_oid: %s does not have an argument.\n",
					cfg.dn_oid[i]);
				exit(1);
			}
			ret =
			    gnutls_x509_crq_set_dn_by_oid(crq,
							  cfg.dn_oid[i], 0,
							  cfg.dn_oid[i +
								     1],
							  strlen(cfg.
								 dn_oid[i +
									1]));

			if (ret < 0) {
				fprintf(stderr, "set_dn_oid: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}
	}

}

void get_tlsfeatures_set(int type, void *crt)
{
	int ret, i;
	unsigned int feature;

	if (batch) {
		if (!cfg.tls_features)
			return;

		gnutls_x509_tlsfeatures_t features;
		ret = gnutls_x509_tlsfeatures_init(&features);
		if (ret < 0) {
			fprintf(stderr, "gnutls_x509_tlsfeatures_init: %s\n",
				gnutls_strerror(ret));
			exit(1);
		}

		for (i = 0; cfg.tls_features[i]; ++i) {
			feature = strtoul(cfg.tls_features[i], 0, 10);
			ret = gnutls_x509_tlsfeatures_add(features, feature);
			if (ret < 0) {
				fprintf(stderr, "gnutls_x509_tlsfeatures_add: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}

		if (type == TYPE_CRT) {
			ret = gnutls_x509_crt_set_tlsfeatures(crt, features);
			if (ret < 0) {
				fprintf(stderr, "gnutls_x509_crt_set_tlsfeatures: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}
		else {
			ret = gnutls_x509_crq_set_tlsfeatures(crt, features);
			if (ret < 0) {
				fprintf(stderr, "gnutls_x509_crq_set_tlsfeatures: %s\n",
					gnutls_strerror(ret));
				exit(1);
			}
		}

		gnutls_x509_tlsfeatures_deinit(features);
	}
}

void crq_extensions_set(gnutls_x509_crt_t crt, gnutls_x509_crq_t crq)
{
	int ret, i;

	if (batch) {
		if (!cfg.exts_to_honor)
			return;

		for (i = 0; cfg.exts_to_honor[i]; ++i) {
			ret = gnutls_x509_crt_set_crq_extension_by_oid(crt, crq, cfg.exts_to_honor[i], 0);
			if (ret < 0) {
				fprintf(stderr, "setting extension failed: %s: %s\n", cfg.exts_to_honor[i],
					gnutls_strerror(ret));
			}
		}
	}
}
