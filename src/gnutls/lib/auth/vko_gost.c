/*
 * Copyright (C) 2011-2012 Free Software Foundation, Inc.
 * Copyright (C) 2016 Dmitry Eremin-Solenikov
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
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

#include "gnutls_int.h"
#include "auth.h"
#include "errors.h"
#include "vko.h"
#include <state.h>
#include <datum.h>
#include <ext/signature.h>
#include <ext/supported_groups.h>
#include <auth/cert.h>
#include <pk.h>
#include <abstract_int.h>

#if defined(ENABLE_GOST)
static int gen_vko_gost_client_kx(gnutls_session_t, gnutls_buffer_st *);
static int proc_vko_gost_client_kx(gnutls_session_t session,
				   uint8_t * data, size_t _data_size);

/* VKO GOST Key Exchange:
 * see draft-smyshlyaev-tls12-gost-suites-06, Section 4.2.4
 *
 * Client generates ephemeral key pair, uses server's public key (from
 * certificate), ephemeral private key and additional nonce (UKM) to generate
 * (VKO) shared point/shared secret. This secret is used to encrypt (key wrap)
 * random PMS. Then encrypted PMS and client's ephemeral public key are wrappen
 * in ASN.1 structure and sent in KX message.
 *
 * Server uses decodes ASN.1 structure and uses it's own private key and
 * client's ephemeral public key to unwrap PMS.
 *
 * Note, this KX is not PFS one, despite using ephemeral key pairs on client
 * side.
 */
const mod_auth_st vko_gost_auth_struct = {
	"VKO_GOST",
	_gnutls_gen_cert_server_crt,
	_gnutls_gen_cert_client_crt,
	NULL,
	gen_vko_gost_client_kx,
	_gnutls_gen_cert_client_crt_vrfy,
	_gnutls_gen_cert_server_cert_req,

	_gnutls_proc_crt,
	_gnutls_proc_crt,
	NULL,
	proc_vko_gost_client_kx,
	_gnutls_proc_cert_client_crt_vrfy,
	_gnutls_proc_cert_cert_req
};

#define VKO_GOST_UKM_LEN	8

static int
calc_ukm(gnutls_session_t session, uint8_t *ukm)
{
	gnutls_digest_algorithm_t digalg = GNUTLS_DIG_STREEBOG_256;
	gnutls_hash_hd_t dig;
	int ret;

	ret = gnutls_hash_init(&dig, digalg);
	if (ret < 0)
		return gnutls_assert_val(ret);

	gnutls_hash(dig, session->security_parameters.client_random,
		    sizeof(session->security_parameters.client_random));

	gnutls_hash(dig, session->security_parameters.server_random,
		    sizeof(session->security_parameters.server_random));

	gnutls_hash_deinit(dig, ukm);

	return gnutls_hash_get_len(digalg);
}

static int print_priv_key(gnutls_pk_params_st *params)
{
	int ret;
	uint8_t priv_buf[512/8];
	char buf[512 / 4 + 1];
	size_t bytes = sizeof(priv_buf);

	/* Check if _gnutls_hard_log will print anything */
	if (likely(_gnutls_log_level < 9))
		return GNUTLS_E_SUCCESS;

	ret = _gnutls_mpi_print(params->params[GOST_K],
				priv_buf, &bytes);
	if (ret < 0)
		return gnutls_assert_val(ret);

	_gnutls_hard_log("INT: VKO PRIVATE KEY[%zd]: %s\n",
			 bytes, _gnutls_bin2hex(priv_buf,
						bytes,
						buf, sizeof(buf),
						NULL));
	return 0;
}

static int
vko_prepare_client_keys(gnutls_session_t session,
			gnutls_pk_params_st *pub,
			gnutls_pk_params_st *priv)
{
	int ret;
	gnutls_ecc_curve_t curve;
	const gnutls_group_entry_st *group;
	cert_auth_info_t info;
	gnutls_pcert_st peer_cert;

	info = _gnutls_get_auth_info(session, GNUTLS_CRD_CERTIFICATE);
	if (info == NULL || info->ncerts == 0)
		return gnutls_assert_val(GNUTLS_E_INTERNAL_ERROR);

	ret = _gnutls_get_auth_info_pcert(&peer_cert,
					  session->security_parameters.
					  server_ctype, info);
	if (ret < 0)
		return gnutls_assert_val(ret);

	/* Copy public key contents and free the rest */
	memcpy(pub, &peer_cert.pubkey->params, sizeof(gnutls_pk_params_st));
	gnutls_free(peer_cert.pubkey);
	peer_cert.pubkey = NULL;
	gnutls_pcert_deinit(&peer_cert);

	curve = pub->curve;
	group = _gnutls_id_to_group(_gnutls_ecc_curve_get_group(curve));
	if (group == NULL) {
		_gnutls_debug_log("received unknown curve %d\n", curve);
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	} else {
		_gnutls_debug_log("received curve %s\n", group->name);
	}

	ret = _gnutls_session_supports_group(session, group->id);
	if (ret < 0)
		return gnutls_assert_val(ret);

	if (pub->algo == GNUTLS_PK_GOST_12_512) {
		gnutls_sign_algorithm_set_server(session, GNUTLS_SIGN_GOST_512);
	} else {
		gnutls_sign_algorithm_set_server(session, GNUTLS_SIGN_GOST_256);
	}

	_gnutls_session_group_set(session, group);

	ret =  _gnutls_pk_generate_keys(pub->algo,
					curve,
					priv, 1);
	if (ret < 0)
		return gnutls_assert_val(ret);

	priv->gost_params = pub->gost_params;

	print_priv_key(priv);

	session->key.key.size = 32; /* GOST key size */
	session->key.key.data = gnutls_malloc(session->key.key.size);
	if (session->key.key.data == NULL) {
		gnutls_assert();
		session->key.key.size = 0;
		return GNUTLS_E_MEMORY_ERROR;
	}

	/* Generate random */
	ret = gnutls_rnd(GNUTLS_RND_RANDOM, session->key.key.data,
			 session->key.key.size);
	if (ret < 0) {
		gnutls_assert();
		gnutls_free(session->key.key.data);
		session->key.key.size = 0;
		return ret;
	}

	return 0;
}

/* KX message is:
   TLSGostKeyTransportBlob ::= SEQUENCE {
        keyBlob GostR3410-KeyTransport,
        proxyKeyBlobs SEQUENCE OF TLSProxyKeyTransportBlob OPTIONAL
   }

   draft-smyshlyaev-tls12-gost-suites does not define proxyKeyBlobs, but old
   CSPs still send additional information after keyBlob.

   We only need keyBlob and we completely ignore the rest of the structure.

   _gnutls_gost_keytrans_decrypt will decrypt GostR3410-KeyTransport
   */

static int
proc_vko_gost_client_kx(gnutls_session_t session,
			uint8_t * data, size_t _data_size)
{
	int ret, i = 0;
	ssize_t data_size = _data_size;
	gnutls_privkey_t privkey = session->internals.selected_key;
	uint8_t ukm_data[MAX_HASH_SIZE];
	gnutls_datum_t ukm = {ukm_data, VKO_GOST_UKM_LEN};
	gnutls_datum_t cek;
	int len;

	if (!privkey || privkey->type != GNUTLS_PRIVKEY_X509)
		return gnutls_assert_val(GNUTLS_E_INVALID_REQUEST);

	/* Skip TLSGostKeyTransportBlob tag and length */
	DECR_LEN(data_size, 1);
	if (data[0] != (ASN1_TAG_SEQUENCE | ASN1_CLASS_STRUCTURED))
		return gnutls_assert_val(GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER);
	i += 1;

	ret = asn1_get_length_der(&data[i], data_size, &len);
	if (ret < 0)
		return gnutls_assert_val(GNUTLS_E_ASN1_DER_ERROR);
	DECR_LEN(data_size, len);
	i += len;

	/* Check that nothing is left after TLSGostKeyTransportBlob */
	DECR_LEN_FINAL(data_size, ret);

	/* Point data to GostR3410-KeyTransport */
	data_size = ret;
	data += i;

	/* Now do the tricky part: determine length of GostR3410-KeyTransport */
	DECR_LEN(data_size, 1); /* tag */
	ret = asn1_get_length_der(&data[1], data_size, &len);
	DECR_LEN_FINAL(data_size, len + ret);

	cek.data = data;
	cek.size = ret + len + 1;

	ret = calc_ukm(session, ukm_data);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_gost_keytrans_decrypt(&privkey->key.x509->params,
					    &cek, &ukm,
					    &session->key.key);
	if (ret < 0)
		return gnutls_assert_val(ret);

	return 0;
}

static int
gen_vko_gost_client_kx(gnutls_session_t session,
		       gnutls_buffer_st * data)
{
	int ret;
	gnutls_datum_t out = {};
	uint8_t ukm_data[MAX_HASH_SIZE];
	gnutls_datum_t ukm = {ukm_data, VKO_GOST_UKM_LEN};
	gnutls_pk_params_st pub;
	gnutls_pk_params_st priv;
	uint8_t tl[1 + ASN1_MAX_LENGTH_SIZE];
	int len;

	ret = calc_ukm(session, ukm_data);
	if (ret < 0)
		return gnutls_assert_val(ret);

	gnutls_pk_params_init(&pub);
	gnutls_pk_params_init(&priv);
	ret = vko_prepare_client_keys(session, &pub, &priv);
	if (ret < 0)
		return gnutls_assert_val(ret);

	ret = _gnutls_gost_keytrans_encrypt(&pub,
					    &priv,
					    &session->key.key,
					    &ukm, &out);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	tl[0] = ASN1_TAG_SEQUENCE | ASN1_CLASS_STRUCTURED;
	asn1_length_der(out.size, tl + 1, &len);
	ret = gnutls_buffer_append_data(data, tl, len + 1);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = gnutls_buffer_append_data(data, out.data, out.size);
	if (ret < 0) {
		gnutls_assert();
		goto cleanup;
	}

	ret = data->length;
 cleanup:
	/* no longer needed */
	gnutls_pk_params_release(&priv);
	gnutls_pk_params_release(&pub);

	_gnutls_free_datum(&out);

	return ret;
}
#endif
