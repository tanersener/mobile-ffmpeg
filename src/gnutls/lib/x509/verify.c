/*
 * Copyright (C) 2003-2014 Free Software Foundation, Inc.
 * Copyright (C) 2013 Nikos Mavrogiannopoulos
 * Copyright (C) 2014 Red Hat
 *
 * Author: Nikos Mavrogiannopoulos
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

/* All functions which relate to X.509 certificate verification stuff are
 * included here
 */

#include "gnutls_int.h"
#include "errors.h"
#include <libtasn1.h>
#include <global.h>
#include <num.h>		/* MAX */
#include <tls-sig.h>
#include <str.h>
#include <datum.h>
#include <x509_int.h>
#include <common.h>
#include <pk.h>

/* Checks if two certs have the same name and the same key.  Return 1 on match. 
 * If @is_ca is zero then this function is identical to gnutls_x509_crt_equals()
 */
unsigned
_gnutls_check_if_same_key(gnutls_x509_crt_t cert1,
			  gnutls_x509_crt_t cert2,
			  unsigned is_ca)
{
	int ret;
	unsigned result;

	if (is_ca == 0)
		return gnutls_x509_crt_equals(cert1, cert2);

	ret = _gnutls_is_same_dn(cert1, cert2);
	if (ret == 0)
		return 0;

	if (cert1->raw_spki.size > 0 && (cert1->raw_spki.size == cert2->raw_spki.size) &&
	    (memcmp(cert1->raw_spki.data, cert2->raw_spki.data, cert1->raw_spki.size) == 0))
		result = 1;
	else
		result = 0;

	return result;
}

unsigned
_gnutls_check_if_same_key2(gnutls_x509_crt_t cert1,
			   gnutls_datum_t * cert2bin)
{
	int ret;
	gnutls_x509_crt_t cert2;

	ret = gnutls_x509_crt_init(&cert2);
	if (ret < 0)
		return gnutls_assert_val(0);

	ret = gnutls_x509_crt_import(cert2, cert2bin, GNUTLS_X509_FMT_DER);
	if (ret < 0) {
		gnutls_x509_crt_deinit(cert2);
		return gnutls_assert_val(0);
	}

	ret = _gnutls_check_if_same_key(cert1, cert2, 1);

	gnutls_x509_crt_deinit(cert2);
	return ret;
}


/* Checks if the issuer of a certificate is a
 * Certificate Authority, or if the certificate is the same
 * as the issuer (and therefore it doesn't need to be a CA).
 *
 * Returns true or false, if the issuer is a CA,
 * or not.
 */
static unsigned
check_if_ca(gnutls_x509_crt_t cert, gnutls_x509_crt_t issuer,
	    unsigned int *max_path, unsigned int flags)
{
	gnutls_datum_t cert_signed_data = { NULL, 0 };
	gnutls_datum_t issuer_signed_data = { NULL, 0 };
	gnutls_datum_t cert_signature = { NULL, 0 };
	gnutls_datum_t issuer_signature = { NULL, 0 };
	int pathlen = -1, ret;
	unsigned result;
	unsigned int ca_status = 0;

	/* Check if the issuer is the same with the
	 * certificate. This is added in order for trusted
	 * certificates to be able to verify themselves.
	 */

	ret =
	    _gnutls_x509_get_signed_data(issuer->cert, &issuer->der, "tbsCertificate",
					 &issuer_signed_data);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret =
	    _gnutls_x509_get_signed_data(cert->cert, &cert->der, "tbsCertificate",
					 &cert_signed_data);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret =
	    _gnutls_x509_get_signature(issuer->cert, "signature",
				       &issuer_signature);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	ret =
	    _gnutls_x509_get_signature(cert->cert, "signature",
				       &cert_signature);
	if (ret < 0) {
		gnutls_assert();
		goto fail;
	}

	/* If the subject certificate is the same as the issuer
	 * return true.
	 */
	if (!(flags & GNUTLS_VERIFY_DO_NOT_ALLOW_SAME))
		if (cert_signed_data.size == issuer_signed_data.size) {
			if ((memcmp
			     (cert_signed_data.data,
			      issuer_signed_data.data,
			      cert_signed_data.size) == 0)
			    && (cert_signature.size ==
				issuer_signature.size)
			    &&
			    (memcmp
			     (cert_signature.data, issuer_signature.data,
			      cert_signature.size) == 0)) {
				result = 1;
				goto cleanup;
			}
		}

	ret =
	    gnutls_x509_crt_get_basic_constraints(issuer, NULL, &ca_status,
						  &pathlen);
	if (ret < 0) {
		ca_status = 0;
		pathlen = -1;
	}

	if (ca_status != 0 && pathlen != -1) {
		if ((unsigned) pathlen < *max_path)
			*max_path = pathlen;
	}

	if (ca_status != 0) {
		result = 1;
		goto cleanup;
	}
	/* Handle V1 CAs that do not have a basicConstraint, but accept
	   these certs only if the appropriate flags are set. */
	else if ((ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) &&
		 ((flags & GNUTLS_VERIFY_ALLOW_ANY_X509_V1_CA_CRT) ||
		  (!(flags & GNUTLS_VERIFY_DO_NOT_ALLOW_X509_V1_CA_CRT) &&
		   (gnutls_x509_crt_check_issuer(issuer, issuer) != 0)))) {
		gnutls_assert();
		result = 1;
		goto cleanup;
	} else {
		gnutls_assert();
	}

 fail:
	result = 0;

 cleanup:
	_gnutls_free_datum(&cert_signed_data);
	_gnutls_free_datum(&issuer_signed_data);
	_gnutls_free_datum(&cert_signature);
	_gnutls_free_datum(&issuer_signature);
	return result;
}


/* This function checks if cert's issuer is issuer.
 * This does a straight (DER) compare of the issuer/subject DN fields in
 * the given certificates, as well as check the authority key ID.
 *
 * Returns 1 if they match and (0) if they don't match. 
 */
static unsigned is_issuer(gnutls_x509_crt_t cert, gnutls_x509_crt_t issuer)
{
	uint8_t id1[MAX_KEY_ID_SIZE];
	uint8_t id2[MAX_KEY_ID_SIZE];
	size_t id1_size;
	size_t id2_size;
	int ret;
	unsigned result;

	if (_gnutls_x509_compare_raw_dn
	    (&cert->raw_issuer_dn, &issuer->raw_dn) != 0)
		result = 1;
	else
		result = 0;

	if (result != 0) {
		/* check if the authority key identifier matches the subject key identifier
		 * of the issuer */
		id1_size = sizeof(id1);

		ret =
		    gnutls_x509_crt_get_authority_key_id(cert, id1,
							 &id1_size, NULL);
		if (ret < 0) {
			/* If there is no authority key identifier in the
			 * certificate, assume they match */
			result = 1;
			goto cleanup;
		}

		id2_size = sizeof(id2);
		ret =
		    gnutls_x509_crt_get_subject_key_id(issuer, id2,
						       &id2_size, NULL);
		if (ret < 0) {
			/* If there is no subject key identifier in the
			 * issuer certificate, assume they match */
			result = 1;
			gnutls_assert();
			goto cleanup;
		}

		if (id1_size == id2_size
		    && memcmp(id1, id2, id1_size) == 0)
			result = 1;
		else
			result = 0;
	}

      cleanup:
	return result;
}

/* Check if the given certificate is the issuer of the CRL.
 * Returns 1 on success and 0 otherwise.
 */
static unsigned is_crl_issuer(gnutls_x509_crl_t crl, gnutls_x509_crt_t issuer)
{
	if (_gnutls_x509_compare_raw_dn
	    (&crl->raw_issuer_dn, &issuer->raw_dn) != 0)
		return 1;
	else
		return 0;
}

/* Checks if the DN of two certificates is the same.
 * Returns 1 if they match and (0) if they don't match. Otherwise
 * a negative error code is returned to indicate error.
 */
unsigned _gnutls_is_same_dn(gnutls_x509_crt_t cert1, gnutls_x509_crt_t cert2)
{
	if (_gnutls_x509_compare_raw_dn(&cert1->raw_dn, &cert2->raw_dn) !=
	    0)
		return 1;
	else
		return 0;
}

/* Finds an issuer of the certificate. If multiple issuers
 * are present, returns one that is activated and not expired.
 */
static inline gnutls_x509_crt_t
find_issuer(gnutls_x509_crt_t cert,
	    const gnutls_x509_crt_t * trusted_cas, int tcas_size)
{
	int i;
	gnutls_x509_crt_t issuer = NULL;

	/* this is serial search. 
	 */
	for (i = 0; i < tcas_size; i++) {
		if (is_issuer(cert, trusted_cas[i]) != 0) {
			if (issuer == NULL) {
				issuer = trusted_cas[i];
			} else {
				time_t now = gnutls_time(0);

				if (now <
				    gnutls_x509_crt_get_expiration_time
				    (trusted_cas[i])
				    && now >=
				    gnutls_x509_crt_get_activation_time
				    (trusted_cas[i])) {
					issuer = trusted_cas[i];
				}
			}
		}
	}

	return issuer;
}

static unsigned int check_time_status(gnutls_x509_crt_t crt, time_t now)
{
	int status = 0;
	time_t t;

	t = gnutls_x509_crt_get_activation_time(crt);
	if (t == (time_t) - 1 || now < t) {
		status |= GNUTLS_CERT_NOT_ACTIVATED;
		status |= GNUTLS_CERT_INVALID;
		return status;
	}

	t = gnutls_x509_crt_get_expiration_time(crt);
	if (t == (time_t) - 1 || now > t) {
		status |= GNUTLS_CERT_EXPIRED;
		status |= GNUTLS_CERT_INVALID;
		return status;
	}

	return 0;
}

unsigned _gnutls_is_broken_sig_allowed(gnutls_sign_algorithm_t sig, unsigned int flags)
{
	/* the first two are for backwards compatibility */
	if ((sig == GNUTLS_SIGN_RSA_MD2)
	    && (flags & GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD2))
		return 1;
	if ((sig == GNUTLS_SIGN_RSA_MD5)
	    && (flags & GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD5))
		return 1;
	/* we no longer have individual flags - but rather a catch all */
	if ((flags & GNUTLS_VERIFY_ALLOW_BROKEN) == GNUTLS_VERIFY_ALLOW_BROKEN)
		return 1;
	return 0;
}

#define CASE_SEC_PARAM(profile, level) \
	case profile: \
		sym_bits = gnutls_sec_param_to_symmetric_bits(level); \
		hash = gnutls_sign_get_hash_algorithm(sigalg); \
		entry = mac_to_entry(hash); \
		if (hash <= 0 || entry == NULL) { \
			_gnutls_cert_log("cert", crt); \
			_gnutls_debug_log(#level": certificate's signature hash is unknown\n"); \
			return gnutls_assert_val(0); \
		} \
		if (entry->output_size*8/2 < sym_bits) { \
			_gnutls_cert_log("cert", crt); \
			_gnutls_debug_log(#level": certificate's signature hash strength is unacceptable (is %u bits, needed %u)\n", entry->output_size*8/2, sym_bits); \
			return gnutls_assert_val(0); \
		} \
		sp = gnutls_pk_bits_to_sec_param(pkalg, bits); \
		if (sp < level) { \
			_gnutls_cert_log("cert", crt); \
			_gnutls_debug_log(#level": certificate's security level is unacceptable\n"); \
			return gnutls_assert_val(0); \
		} \
		if (issuer) { \
			sp = gnutls_pk_bits_to_sec_param(issuer_pkalg, issuer_bits); \
			if (sp < level) { \
				_gnutls_cert_log("issuer", issuer); \
				_gnutls_debug_log(#level": certificate's issuer security level is unacceptable\n"); \
				return gnutls_assert_val(0); \
			} \
		} \
		break;

/* Checks whether the provided certificates are acceptable
 * according to verification profile specified.
 *
 * @crt: a certificate
 * @issuer: the certificates issuer (allowed to be NULL)
 * @sigalg: the signature algorithm used
 * @flags: the specified verification flags
 */
static unsigned is_level_acceptable(
	gnutls_x509_crt_t crt, gnutls_x509_crt_t issuer,
	gnutls_sign_algorithm_t sigalg, unsigned flags)
{
	gnutls_certificate_verification_profiles_t profile = GNUTLS_VFLAGS_TO_PROFILE(flags);
	const mac_entry_st *entry;
	int issuer_pkalg = 0, pkalg, ret;
	unsigned bits = 0, issuer_bits = 0, sym_bits = 0;
	gnutls_pk_params_st params;
	gnutls_sec_param_t sp;
	int hash;

	if (profile == 0)
		return 1;

	pkalg = gnutls_x509_crt_get_pk_algorithm(crt, &bits);
	if (pkalg < 0)
		return gnutls_assert_val(0);

	if (issuer) {
		issuer_pkalg = gnutls_x509_crt_get_pk_algorithm(issuer, &issuer_bits);
		if (issuer_pkalg < 0)
			return gnutls_assert_val(0);
	}

	switch (profile) {
		CASE_SEC_PARAM(GNUTLS_PROFILE_VERY_WEAK, GNUTLS_SEC_PARAM_VERY_WEAK);
		CASE_SEC_PARAM(GNUTLS_PROFILE_LOW, GNUTLS_SEC_PARAM_LOW);
		CASE_SEC_PARAM(GNUTLS_PROFILE_LEGACY, GNUTLS_SEC_PARAM_LEGACY);
		CASE_SEC_PARAM(GNUTLS_PROFILE_MEDIUM, GNUTLS_SEC_PARAM_MEDIUM);
		CASE_SEC_PARAM(GNUTLS_PROFILE_HIGH, GNUTLS_SEC_PARAM_HIGH);
		CASE_SEC_PARAM(GNUTLS_PROFILE_ULTRA, GNUTLS_SEC_PARAM_ULTRA);
		case GNUTLS_PROFILE_SUITEB128:
		case GNUTLS_PROFILE_SUITEB192: {
			unsigned curve, issuer_curve;

			/* check suiteB params validity: rfc5759 */

			if (gnutls_x509_crt_get_version(crt) != 3) {
				_gnutls_debug_log("SUITEB: certificate uses an unacceptable version number\n");
				return gnutls_assert_val(0);
			}

			if (sigalg != GNUTLS_SIGN_ECDSA_SHA256 && sigalg != GNUTLS_SIGN_ECDSA_SHA384) {
				_gnutls_debug_log("SUITEB: certificate is not signed using ECDSA-SHA256 or ECDSA-SHA384\n");
				return gnutls_assert_val(0);
			}

			if (pkalg != GNUTLS_PK_EC) {
				_gnutls_debug_log("SUITEB: certificate does not contain ECC parameters\n");
				return gnutls_assert_val(0);
			}

			if (issuer_pkalg != GNUTLS_PK_EC) {
				_gnutls_debug_log("SUITEB: certificate's issuer does not have ECC parameters\n");
				return gnutls_assert_val(0);
			}

			ret = _gnutls_x509_crt_get_mpis(crt, &params);
			if (ret < 0) {
				_gnutls_debug_log("SUITEB: cannot read certificate params\n");
				return gnutls_assert_val(0);
			}

			curve = params.flags;
			gnutls_pk_params_release(&params);

			if (curve != GNUTLS_ECC_CURVE_SECP256R1 &&
				curve != GNUTLS_ECC_CURVE_SECP384R1) {
				_gnutls_debug_log("SUITEB: certificate's ECC params do not contain SECP256R1 or SECP384R1\n");
				return gnutls_assert_val(0);
			}

			if (profile == GNUTLS_PROFILE_SUITEB192) {
				if (curve != GNUTLS_ECC_CURVE_SECP384R1) {
					_gnutls_debug_log("SUITEB192: certificate does not use SECP384R1\n");
					return gnutls_assert_val(0);
				}
			}

			if (issuer != NULL) {
				if (gnutls_x509_crt_get_version(issuer) != 3) {
					_gnutls_debug_log("SUITEB: certificate's issuer uses an unacceptable version number\n");
					return gnutls_assert_val(0);
				}

				ret = _gnutls_x509_crt_get_mpis(issuer, &params);
				if (ret < 0) {
					_gnutls_debug_log("SUITEB: cannot read certificate params\n");
					return gnutls_assert_val(0);
				}

				issuer_curve = params.flags;
				gnutls_pk_params_release(&params);

				if (issuer_curve != GNUTLS_ECC_CURVE_SECP256R1 &&
					issuer_curve != GNUTLS_ECC_CURVE_SECP384R1) {
					_gnutls_debug_log("SUITEB: certificate's issuer ECC params do not contain SECP256R1 or SECP384R1\n");
					return gnutls_assert_val(0);
				}

				if (issuer_curve < curve) {
					_gnutls_debug_log("SUITEB: certificate's issuer ECC params are weaker than the certificate's\n");
					return gnutls_assert_val(0);
				}

				if (sigalg == GNUTLS_SIGN_ECDSA_SHA256 && 
					issuer_curve == GNUTLS_ECC_CURVE_SECP384R1) {
					_gnutls_debug_log("SUITEB: certificate is signed with ECDSA-SHA256 when using SECP384R1\n");
					return gnutls_assert_val(0);
				}
			}

			break;
		}
	}

	return 1;
}

typedef struct verify_state_st {
	time_t now;
	unsigned int max_path;
	gnutls_x509_name_constraints_t nc;
	gnutls_x509_tlsfeatures_t tls_feat;
	gnutls_verify_output_function *func;
} verify_state_st;

#define MARK_INVALID(x) gnutls_assert(); \
	out |= (x|GNUTLS_CERT_INVALID); \
	result = 0

/* 
 * Verifies the given certificate against a certificate list of
 * trusted CAs.
 *
 * Returns only 0 or 1. If 1 it means that the certificate 
 * was successfuly verified.
 *
 * 'flags': an OR of the gnutls_certificate_verify_flags enumeration.
 *
 * Output will hold some extra information about the verification
 * procedure.
 */
static unsigned
verify_crt(gnutls_x509_crt_t cert,
			    const gnutls_x509_crt_t * trusted_cas,
			    int tcas_size, unsigned int flags,
			    unsigned int *output,
			    verify_state_st *vparams,
			    unsigned end_cert)
{
	gnutls_datum_t cert_signed_data = { NULL, 0 };
	gnutls_datum_t cert_signature = { NULL, 0 };
	gnutls_x509_crt_t issuer = NULL;
	int issuer_version, hash_algo;
	unsigned result = 1;
	const mac_entry_st * me;
	unsigned int out = 0, usage;
	int sigalg, ret;

	if (output)
		*output = 0;

	if (vparams->max_path == 0) {
		MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
		/* bail immediately, to avoid inconistency  */
		goto cleanup;
	}
	vparams->max_path--;

	if (tcas_size >= 1)
		issuer = find_issuer(cert, trusted_cas, tcas_size);

	ret =
	    _gnutls_x509_get_signed_data(cert->cert, &cert->der, "tbsCertificate",
					 &cert_signed_data);
	if (ret < 0) {
		MARK_INVALID(0);
		cert_signed_data.data = NULL;
	}

	ret =
	    _gnutls_x509_get_signature(cert->cert, "signature",
				       &cert_signature);
	if (ret < 0) {
		MARK_INVALID(0);
		cert_signature.data = NULL;
	}

	ret =
	    _gnutls_x509_get_signature_algorithm(cert->cert,
						 "signatureAlgorithm.algorithm");
	if (ret < 0) {
		MARK_INVALID(0);
	}
	sigalg = ret;

	/* issuer is not in trusted certificate
	 * authorities.
	 */
	if (issuer == NULL) {
		MARK_INVALID(GNUTLS_CERT_SIGNER_NOT_FOUND);
	} else {
		if (vparams->nc != NULL) {
			/* append the issuer's constraints */
			ret = gnutls_x509_crt_get_name_constraints(issuer, vparams->nc, 
				GNUTLS_NAME_CONSTRAINTS_FLAG_APPEND, NULL);
			if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
				MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
				goto nc_done;
			}

			/* only check name constraints in server certificates, not CAs */
			if (end_cert != 0) {
				ret = gnutls_x509_name_constraints_check_crt(vparams->nc, GNUTLS_SAN_DNSNAME, cert);
				if (ret == 0) {
					MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
					goto nc_done;
				}

				ret = gnutls_x509_name_constraints_check_crt(vparams->nc, GNUTLS_SAN_RFC822NAME, cert);
				if (ret == 0) {
					MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
					goto nc_done;
				}

				ret = gnutls_x509_name_constraints_check_crt(vparams->nc, GNUTLS_SAN_DN, cert);
				if (ret == 0) {
					MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
					goto nc_done;
				}

				ret = gnutls_x509_name_constraints_check_crt(vparams->nc, GNUTLS_SAN_URI, cert);
				if (ret == 0) {
					MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
					goto nc_done;
				}

				ret = gnutls_x509_name_constraints_check_crt(vparams->nc, GNUTLS_SAN_IPADDRESS, cert);
				if (ret == 0) {
					MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
					goto nc_done;
				}
			}
		}

 nc_done:
		if (vparams->tls_feat != NULL) {
			/* append the issuer's constraints */
			ret = gnutls_x509_crt_get_tlsfeatures(issuer, vparams->tls_feat, GNUTLS_EXT_FLAG_APPEND, NULL);
			if (ret < 0 && ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
				MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
				goto feat_done;
			}

			ret = gnutls_x509_tlsfeatures_check_crt(vparams->tls_feat, cert);
			if (ret == 0) {
				MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
				goto feat_done;
			}
		}

 feat_done:
		issuer_version = gnutls_x509_crt_get_version(issuer);

		if (issuer_version < 0) {
			MARK_INVALID(0);
		} else if (!(flags & GNUTLS_VERIFY_DISABLE_CA_SIGN) &&
			   ((flags & GNUTLS_VERIFY_DO_NOT_ALLOW_X509_V1_CA_CRT)
			    || issuer_version != 1)) {
			if (check_if_ca(cert, issuer, &vparams->max_path, flags) != 1) {
				MARK_INVALID(GNUTLS_CERT_SIGNER_NOT_CA);
			}

			ret =
			    gnutls_x509_crt_get_key_usage(issuer, &usage, NULL);
			if (ret != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
				if (ret < 0) {
					MARK_INVALID(0);
				} else if (!(usage & GNUTLS_KEY_KEY_CERT_SIGN)) {
					MARK_INVALID(GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE);
				}
			}
		}

		if (sigalg >= 0) {
			hash_algo = gnutls_sign_get_hash_algorithm(sigalg);
			me = mac_to_entry(hash_algo);
		} else {
			me = NULL;
		}

		if (me == NULL) {
			MARK_INVALID(0);
		} else if (cert_signed_data.data != NULL &&
			   cert_signature.data != NULL) {
			ret =
			    _gnutls_x509_verify_data(me,
						     &cert_signed_data,
						     &cert_signature,
						     issuer);
			if (ret == GNUTLS_E_PK_SIG_VERIFY_FAILED) {
				MARK_INVALID(GNUTLS_CERT_SIGNATURE_FAILURE);
			} else if (ret < 0) {
				MARK_INVALID(0);
			}
		}
	}

	if (sigalg >= 0) {
		if (is_level_acceptable(cert, issuer, sigalg, flags) == 0) {
			MARK_INVALID(GNUTLS_CERT_INSECURE_ALGORITHM);
		}

		/* If the certificate is not self signed check if the algorithms
		 * used are secure. If the certificate is self signed it doesn't
		 * really matter.
		 */
		if (gnutls_sign_is_secure(sigalg) == 0 &&
		    _gnutls_is_broken_sig_allowed(sigalg, flags) == 0 &&
		    is_issuer(cert, cert) == 0) {
			MARK_INVALID(GNUTLS_CERT_INSECURE_ALGORITHM);
		}
	}

	/* Check activation/expiration times
	 */
	if (!(flags & GNUTLS_VERIFY_DISABLE_TIME_CHECKS)) {
		/* check the time of the issuer first */
		if (issuer != NULL &&
		    !(flags & GNUTLS_VERIFY_DISABLE_TRUSTED_TIME_CHECKS)) {
			out |= check_time_status(issuer, vparams->now);
			if (out != 0) {
				gnutls_assert();
				result = 0;
			}
		}

		out |= check_time_status(cert, vparams->now);
		if (out != 0) {
			gnutls_assert();
			result = 0;
		}
	}

      cleanup:
	if (output)
		*output |= out;

	if (vparams->func) {
		if (result == 0) {
			out |= GNUTLS_CERT_INVALID;
		}
		vparams->func(cert, issuer, NULL, out);
	}
	_gnutls_free_datum(&cert_signed_data);
	_gnutls_free_datum(&cert_signature);

	return result;
}

/**
 * gnutls_x509_crt_check_issuer:
 * @cert: is the certificate to be checked
 * @issuer: is the certificate of a possible issuer
 *
 * This function will check if the given certificate was issued by the
 * given issuer. It checks the DN fields and the authority
 * key identifier and subject key identifier fields match.
 *
 * If the same certificate is provided at the @cert and @issuer fields,
 * it will check whether the certificate is self-signed.
 *
 * Returns: It will return true (1) if the given certificate is issued
 *   by the given issuer, and false (0) if not.  
 **/
unsigned
gnutls_x509_crt_check_issuer(gnutls_x509_crt_t cert,
			     gnutls_x509_crt_t issuer)
{
	return is_issuer(cert, issuer);
}

/* Verify X.509 certificate chain.
 *
 * Note that the return value is an OR of GNUTLS_CERT_* elements.
 *
 * This function verifies a X.509 certificate list. The certificate
 * list should lead to a trusted certificate in order to be trusted.
 */
unsigned int
_gnutls_verify_crt_status(const gnutls_x509_crt_t * certificate_list,
				int clist_size,
				const gnutls_x509_crt_t * trusted_cas,
				int tcas_size,
				unsigned int flags,
				const char *purpose,
				gnutls_verify_output_function func)
{
	int i = 0, ret;
	unsigned int status = 0, output;
	time_t now = gnutls_time(0);
	verify_state_st vparams;

	if (clist_size > 1) {
		/* Check if the last certificate in the path is self signed.
		 * In that case ignore it (a certificate is trusted only if it
		 * leads to a trusted party by us, not the server's).
		 *
		 * This prevents from verifying self signed certificates against
		 * themselves. This (although not bad) caused verification
		 * failures on some root self signed certificates that use the
		 * MD2 algorithm.
		 */
		if (gnutls_x509_crt_check_issuer
		    (certificate_list[clist_size - 1],
		     certificate_list[clist_size - 1]) != 0) {
			clist_size--;
		}
	}

	/* We want to shorten the chain by removing the cert that matches
	 * one of the certs we trust and all the certs after that i.e. if
	 * cert chain is A signed-by B signed-by C signed-by D (signed-by
	 * self-signed E but already removed above), and we trust B, remove
	 * B, C and D. */
	if (!(flags & GNUTLS_VERIFY_DO_NOT_ALLOW_SAME))
		i = 0;		/* also replace the first one */
	else
		i = 1;		/* do not replace the first one */

	for (; i < clist_size; i++) {
		int j;

		for (j = 0; j < tcas_size; j++) {
			/* we check for a certificate that may not be identical with the one
			 * sent by the client, but will have the same name and key. That is
			 * because it can happen that a CA certificate is upgraded from intermediate
			 * CA to self-signed CA at some point. */
			if (_gnutls_check_if_same_key
			    (certificate_list[i], trusted_cas[j], i) != 0) {
				/* explicit time check for trusted CA that we remove from
				 * list. GNUTLS_VERIFY_DISABLE_TRUSTED_TIME_CHECKS
				 */

				if (!(flags & GNUTLS_VERIFY_DISABLE_TRUSTED_TIME_CHECKS) &&
					!(flags & GNUTLS_VERIFY_DISABLE_TIME_CHECKS)) {
					status |=
					    check_time_status(trusted_cas[j],
						       now);
					if (status != 0) {
						if (func)
							func(certificate_list[i], trusted_cas[j], NULL, status);
						return status;
					}
				}

				if (func)
					func(certificate_list[i],
					     trusted_cas[j], NULL, status);
				clist_size = i;
				break;
			}
		}
		/* clist_size may have been changed which gets out of loop */
	}

	if (clist_size == 0) {
		/* The certificate is already present in the trusted certificate list.
		 * Nothing to verify. */
		return status;
	}

	memset(&vparams, 0, sizeof(vparams));
	vparams.now = now;
	vparams.max_path = MAX_VERIFY_DEPTH;
	vparams.func = func;

	ret = gnutls_x509_name_constraints_init(&vparams.nc);
	if (ret < 0) {
		gnutls_assert();
		status |= GNUTLS_CERT_INVALID;
		return status;
	}

	ret = gnutls_x509_tlsfeatures_init(&vparams.tls_feat);
	if (ret < 0) {
		gnutls_assert();
		status |= GNUTLS_CERT_INVALID;
		goto cleanup;
	}

	/* Verify the last certificate in the certificate path
	 * against the trusted CA certificate list.
	 *
	 * If no CAs are present returns CERT_INVALID. Thus works
	 * in self signed etc certificates.
	 */
	output = 0;

	ret = verify_crt(certificate_list[clist_size - 1],
					  trusted_cas, tcas_size, flags,
					  &output,
					  &vparams,
					  clist_size==1?1:0);
	if (ret != 1) {
		/* if the last certificate in the certificate
		 * list is invalid, then the certificate is not
		 * trusted.
		 */
		gnutls_assert();
		status |= output;
		status |= GNUTLS_CERT_INVALID;
		goto cleanup;
	}

	/* Verify the certificate path (chain)
	 */
	for (i = clist_size - 1; i > 0; i--) {
		output = 0;
		if (i - 1 < 0)
			break;

		if (purpose != NULL) {
			ret = _gnutls_check_key_purpose(certificate_list[i], purpose, 1);
			if (ret != 1) {
				gnutls_assert();
				status |= GNUTLS_CERT_INVALID;
				status |= GNUTLS_CERT_PURPOSE_MISMATCH;

				if (func)
					func(certificate_list[i-1],
					     certificate_list[i], NULL, status);
				goto cleanup;
			}
		}

		/* note that here we disable this V1 CA flag. So that no version 1
		 * certificates can exist in a supplied chain.
		 */
		if (!(flags & GNUTLS_VERIFY_ALLOW_ANY_X509_V1_CA_CRT)) {
			flags |= GNUTLS_VERIFY_DO_NOT_ALLOW_X509_V1_CA_CRT;
		}

		if ((ret =
		     verify_crt(certificate_list[i - 1],
						 &certificate_list[i], 1,
						 flags, &output,
						 &vparams,
						 i==1?1:0)) != 1) {
			gnutls_assert();
			status |= output;
			status |= GNUTLS_CERT_INVALID;
			goto cleanup;
		}
	}

cleanup:
	gnutls_x509_name_constraints_deinit(vparams.nc);
	gnutls_x509_tlsfeatures_deinit(vparams.tls_feat);
	return status;
}

#define PURPOSE_NSSGC "2.16.840.1.113730.4.1"
#define PURPOSE_VSGC "2.16.840.1.113733.1.8.1"

/* Returns true if the provided purpose is in accordance with the certificate.
 */
unsigned _gnutls_check_key_purpose(gnutls_x509_crt_t cert, const char *purpose, unsigned no_any)
{
	char oid[MAX_OID_SIZE];
	size_t oid_size;
	int ret;
	unsigned critical = 0;
	unsigned check_obsolete_oids = 0;
	unsigned i;

	/* The check_obsolete_oids hack is because of certain very old CA certificates
	 * around which instead of having the GNUTLS_KP_TLS_WWW_SERVER have some old
	 * OIDs for that purpose. Assume these OIDs equal GNUTLS_KP_TLS_WWW_SERVER in
	 * CA certs */
	if (strcmp(purpose, GNUTLS_KP_TLS_WWW_SERVER) == 0) {
		unsigned ca_status;
		ret =
		    gnutls_x509_crt_get_basic_constraints(cert, NULL, &ca_status,
							  NULL);
		if (ret < 0)
			ca_status = 0;

		if (ca_status)
			check_obsolete_oids = 1;
	}

	for (i=0;;i++) {
		oid_size = sizeof(oid);
		ret = gnutls_x509_crt_get_key_purpose_oid(cert, i, oid, &oid_size, &critical);
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
			if (i==0) {
				/* no key purpose in certificate, assume ANY */
				return 1;
			} else {
				gnutls_assert();
				break;
			}
		} else if (ret < 0) {
			gnutls_assert();
			break;
		}

		if (check_obsolete_oids) {
			if (strcmp(oid, PURPOSE_NSSGC) == 0) {
				return 1;
			} else if (strcmp(oid, PURPOSE_VSGC) == 0) {
				return 1;
			}
		}

		if (strcmp(oid, purpose) == 0 || (no_any == 0 && strcmp(oid, GNUTLS_KP_ANY) == 0)) {
			return 1;
		}
		_gnutls_debug_log("looking for key purpose '%s', but have '%s'\n", purpose, oid);
	}
	return 0;
}

#ifdef ENABLE_PKCS11
/* Verify X.509 certificate chain using a PKCS #11 token.
 *
 * Note that the return value is an OR of GNUTLS_CERT_* elements.
 *
 * Unlike the non-PKCS#11 version, this function accepts a key purpose
 * (from GNUTLS_KP_...). That is because in the p11-kit trust modules
 * anchors are mixed and get assigned a purpose.
 *
 * This function verifies a X.509 certificate list. The certificate
 * list should lead to a trusted certificate in order to be trusted.
 */
unsigned int
_gnutls_pkcs11_verify_crt_status(const char* url,
				const gnutls_x509_crt_t * certificate_list,
				unsigned clist_size,
				const char *purpose,
				unsigned int flags,
				gnutls_verify_output_function func)
{
	int ret;
	unsigned int status = 0, i;
	gnutls_x509_crt_t issuer = NULL;
	gnutls_datum_t raw_issuer = {NULL, 0};
	time_t now = gnutls_time(0);

	if (clist_size > 1) {
		/* Check if the last certificate in the path is self signed.
		 * In that case ignore it (a certificate is trusted only if it
		 * leads to a trusted party by us, not the server's).
		 *
		 * This prevents from verifying self signed certificates against
		 * themselves. This (although not bad) caused verification
		 * failures on some root self signed certificates that use the
		 * MD2 algorithm.
		 */
		if (gnutls_x509_crt_check_issuer
		    (certificate_list[clist_size - 1],
		     certificate_list[clist_size - 1]) != 0) {
			clist_size--;
		}
	}

	/* We want to shorten the chain by removing the cert that matches
	 * one of the certs we trust and all the certs after that i.e. if
	 * cert chain is A signed-by B signed-by C signed-by D (signed-by
	 * self-signed E but already removed above), and we trust B, remove
	 * B, C and D. */
	if (!(flags & GNUTLS_VERIFY_DO_NOT_ALLOW_SAME))
		i = 0;		/* also replace the first one */
	else
		i = 1;		/* do not replace the first one */

	for (; i < clist_size; i++) {
		unsigned vflags;

		if (i == 0) /* in the end certificate do full comparison */
			vflags = GNUTLS_PKCS11_OBJ_FLAG_PRESENT_IN_TRUSTED_MODULE|
				GNUTLS_PKCS11_OBJ_FLAG_COMPARE|GNUTLS_PKCS11_OBJ_FLAG_RETRIEVE_TRUSTED;
		else
			vflags = GNUTLS_PKCS11_OBJ_FLAG_PRESENT_IN_TRUSTED_MODULE|
				GNUTLS_PKCS11_OBJ_FLAG_COMPARE_KEY|GNUTLS_PKCS11_OBJ_FLAG_RETRIEVE_TRUSTED;

		if (gnutls_pkcs11_crt_is_known (url, certificate_list[i], vflags) != 0) {

			if (!(flags & GNUTLS_VERIFY_DISABLE_TRUSTED_TIME_CHECKS) &&
				!(flags & GNUTLS_VERIFY_DISABLE_TIME_CHECKS)) {
				status |=
				    check_time_status(certificate_list[i], now);
				if (status != 0) {
					if (func)
						func(certificate_list[i], certificate_list[i], NULL, status);
					return status;
				}
			}
			if (func)
				func(certificate_list[i],
				     certificate_list[i], NULL, status);

			clist_size = i;
			break;
		}
		/* clist_size may have been changed which gets out of loop */
	}

	if (clist_size == 0) {
		/* The certificate is already present in the trusted certificate list.
		 * Nothing to verify. */
		return status;
	}

	/* check for blacklists */
	for (i = 0; i < clist_size; i++) {
		if (gnutls_pkcs11_crt_is_known (url, certificate_list[i], 
			GNUTLS_PKCS11_OBJ_FLAG_PRESENT_IN_TRUSTED_MODULE|
			GNUTLS_PKCS11_OBJ_FLAG_RETRIEVE_DISTRUSTED) != 0) {
			status |= GNUTLS_CERT_INVALID;
			status |= GNUTLS_CERT_REVOKED;
			if (func)
				func(certificate_list[i], certificate_list[i], NULL, status);
			goto cleanup;
		}
	}

	/* check against issuer */
	ret = gnutls_pkcs11_get_raw_issuer(url, certificate_list[clist_size - 1],
					   &raw_issuer, GNUTLS_X509_FMT_DER,
					   GNUTLS_PKCS11_OBJ_FLAG_OVERWRITE_TRUSTMOD_EXT|GNUTLS_PKCS11_OBJ_FLAG_PRESENT_IN_TRUSTED_MODULE);
	if (ret < 0) {
		gnutls_assert();
		if (ret == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE && clist_size > 2) {

			/* check if the last certificate in the chain is present
			 * in our trusted list, and if yes, verify against it. */
			ret = gnutls_pkcs11_crt_is_known(url, certificate_list[clist_size - 1],
				GNUTLS_PKCS11_OBJ_FLAG_RETRIEVE_TRUSTED|GNUTLS_PKCS11_OBJ_FLAG_COMPARE);
			if (ret != 0) {
				return _gnutls_verify_crt_status(certificate_list, clist_size,
					&certificate_list[clist_size - 1], 1, flags,
					purpose, func);
			}
		}

		status |= GNUTLS_CERT_INVALID;
		status |= GNUTLS_CERT_SIGNER_NOT_FOUND;
		/* verify the certificate list against 0 trusted CAs in order
		 * to get, any additional flags from the certificate list (e.g.,
		 * insecure algorithms or expired */
		status |= _gnutls_verify_crt_status(certificate_list, clist_size,
						    NULL, 0, flags, purpose, func);
		goto cleanup;
	}

	ret = gnutls_x509_crt_init(&issuer);
	if (ret < 0) {
		gnutls_assert();
		status |= GNUTLS_CERT_INVALID;
		status |= GNUTLS_CERT_SIGNER_NOT_FOUND;
		goto cleanup;
	}

	ret = gnutls_x509_crt_import(issuer, &raw_issuer, GNUTLS_X509_FMT_DER);
	if (ret < 0) {
		gnutls_assert();
		status |= GNUTLS_CERT_INVALID;
		status |= GNUTLS_CERT_SIGNER_NOT_FOUND;
		goto cleanup;
	}

	/* security modules that provide trust, bundle all certificates (of all purposes)
	 * together. In software that doesn't specify any purpose assume the default to
	 * be www-server. */
	ret = _gnutls_check_key_purpose(issuer, purpose==NULL?GNUTLS_KP_TLS_WWW_SERVER:purpose, 0);
	if (ret != 1) {
		gnutls_assert();
		status |= GNUTLS_CERT_INVALID;
		status |= GNUTLS_CERT_SIGNER_NOT_FOUND;
		goto cleanup;
	}

	status = _gnutls_verify_crt_status(certificate_list, clist_size,
				&issuer, 1, flags, purpose, func);

cleanup:
	gnutls_free(raw_issuer.data);
	if (issuer != NULL)
		gnutls_x509_crt_deinit(issuer);

	return status;
}
#endif

/* verifies if the certificate is properly signed.
 * returns GNUTLS_E_PK_VERIFY_SIG_FAILED on failure and 1 on success.
 * 
 * 'data' is the signed data
 * 'signature' is the signature!
 */
int
_gnutls_x509_verify_data(const mac_entry_st * me,
			 const gnutls_datum_t * data,
			 const gnutls_datum_t * signature,
			 gnutls_x509_crt_t issuer)
{
	gnutls_pk_params_st issuer_params;
	int ret;

	/* Read the MPI parameters from the issuer's certificate.
	 */
	ret = _gnutls_x509_crt_get_mpis(issuer, &issuer_params);
	if (ret < 0) {
		gnutls_assert();
		return ret;
	}

	ret =
	    pubkey_verify_data(gnutls_x509_crt_get_pk_algorithm
			       (issuer, NULL), me, data, signature,
			       &issuer_params);
	if (ret < 0) {
		gnutls_assert();
	}

	/* release all allocated MPIs
	 */
	gnutls_pk_params_release(&issuer_params);

	return ret;
}

/**
 * gnutls_x509_crt_list_verify:
 * @cert_list: is the certificate list to be verified
 * @cert_list_length: holds the number of certificate in cert_list
 * @CA_list: is the CA list which will be used in verification
 * @CA_list_length: holds the number of CA certificate in CA_list
 * @CRL_list: holds a list of CRLs.
 * @CRL_list_length: the length of CRL list.
 * @flags: Flags that may be used to change the verification algorithm. Use OR of the gnutls_certificate_verify_flags enumerations.
 * @verify: will hold the certificate verification output.
 *
 *
 * This function will try to verify the given certificate list and
 * return its status. The details of the verification are the same
 * as in gnutls_x509_trust_list_verify_crt2().
 *
 * You must check the peer's name in order to check if the verified
 * certificate belongs to the actual peer.
 *
 * The certificate verification output will be put in @verify and will
 * be one or more of the gnutls_certificate_status_t enumerated
 * elements bitwise or'd.  For a more detailed verification status use
 * gnutls_x509_crt_verify() per list element.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_list_verify(const gnutls_x509_crt_t * cert_list,
			    unsigned cert_list_length,
			    const gnutls_x509_crt_t * CA_list,
			    unsigned CA_list_length,
			    const gnutls_x509_crl_t * CRL_list,
			    unsigned CRL_list_length, unsigned int flags,
			    unsigned int *verify)
{
	unsigned i;
	int ret;

	if (cert_list == NULL || cert_list_length == 0)
		return GNUTLS_E_NO_CERTIFICATE_FOUND;

	/* Verify certificate 
	 */
	*verify =
	    _gnutls_verify_crt_status(cert_list, cert_list_length,
					    CA_list, CA_list_length,
					    flags, NULL, NULL);

	/* Check for revoked certificates in the chain. 
	 */
	for (i = 0; i < cert_list_length; i++) {
		ret = gnutls_x509_crt_check_revocation(cert_list[i],
						       CRL_list,
						       CRL_list_length);
		if (ret == 1) {	/* revoked */
			*verify |= GNUTLS_CERT_REVOKED;
			*verify |= GNUTLS_CERT_INVALID;
		}
	}

	return 0;
}

/**
 * gnutls_x509_crt_verify:
 * @cert: is the certificate to be verified
 * @CA_list: is one certificate that is considered to be trusted one
 * @CA_list_length: holds the number of CA certificate in CA_list
 * @flags: Flags that may be used to change the verification algorithm. Use OR of the gnutls_certificate_verify_flags enumerations.
 * @verify: will hold the certificate verification output.
 *
 * This function will try to verify the given certificate and return
 * its status. Note that a verification error does not imply a negative
 * return status. In that case the @verify status is set.
 *
 * The details of the verification are the same
 * as in gnutls_x509_trust_list_verify_crt2().
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0) is returned, otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crt_verify(gnutls_x509_crt_t cert,
		       const gnutls_x509_crt_t * CA_list,
		       unsigned CA_list_length, unsigned int flags,
		       unsigned int *verify)
{
	/* Verify certificate 
	 */
	*verify =
	    _gnutls_verify_crt_status(&cert, 1,
					    CA_list, CA_list_length,
					    flags, NULL, NULL);
	return 0;
}

/**
 * gnutls_x509_crl_check_issuer:
 * @crl: is the CRL to be checked
 * @issuer: is the certificate of a possible issuer
 *
 * This function will check if the given CRL was issued by the given
 * issuer certificate.  
 *
 * Returns: true (1) if the given CRL was issued by the given issuer, 
 * and false (0) if not.
 **/
unsigned
gnutls_x509_crl_check_issuer(gnutls_x509_crl_t crl,
			     gnutls_x509_crt_t issuer)
{
	return is_crl_issuer(crl, issuer);
}

static inline gnutls_x509_crt_t
find_crl_issuer(gnutls_x509_crl_t crl,
		const gnutls_x509_crt_t * trusted_cas, int tcas_size)
{
	int i;

	/* this is serial search. 
	 */

	for (i = 0; i < tcas_size; i++) {
		if (is_crl_issuer(crl, trusted_cas[i]) != 0)
			return trusted_cas[i];
	}

	gnutls_assert();
	return NULL;
}

/**
 * gnutls_x509_crl_verify:
 * @crl: is the crl to be verified
 * @trusted_cas: is a certificate list that is considered to be trusted one
 * @tcas_size: holds the number of CA certificates in CA_list
 * @flags: Flags that may be used to change the verification algorithm. Use OR of the gnutls_certificate_verify_flags enumerations.
 * @verify: will hold the crl verification output.
 *
 * This function will try to verify the given crl and return its verification status.
 * See gnutls_x509_crt_list_verify() for a detailed description of
 * return values. Note that since GnuTLS 3.1.4 this function includes
 * the time checks.
 *
 * Note that value in @verify is set only when the return value of this 
 * function is success (i.e, failure to trust a CRL a certificate does not imply 
 * a negative return value).
 *
 * Before GnuTLS 3.5.7 this function would return zero or a positive
 * number on success.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (0), otherwise a
 *   negative error value.
 **/
int
gnutls_x509_crl_verify(gnutls_x509_crl_t crl,
		       const gnutls_x509_crt_t * trusted_cas,
		       unsigned tcas_size, unsigned int flags,
		       unsigned int *verify)
{
/* CRL is ignored for now */
	gnutls_datum_t crl_signed_data = { NULL, 0 };
	gnutls_datum_t crl_signature = { NULL, 0 };
	gnutls_x509_crt_t issuer = NULL;
	int result, hash_algo;
	time_t now = gnutls_time(0);
	unsigned int usage;

	if (verify)
		*verify = 0;

	if (tcas_size >= 1)
		issuer = find_crl_issuer(crl, trusted_cas, tcas_size);

	result =
	    _gnutls_x509_get_signed_data(crl->crl, &crl->der, "tbsCertList",
					 &crl_signed_data);
	if (result < 0) {
		gnutls_assert();
		if (verify)
			*verify |= GNUTLS_CERT_INVALID;
		goto cleanup;
	}

	result =
	    _gnutls_x509_get_signature(crl->crl, "signature",
				       &crl_signature);
	if (result < 0) {
		gnutls_assert();
		if (verify)
			*verify |= GNUTLS_CERT_INVALID;
		goto cleanup;
	}

	result =
	    _gnutls_x509_get_signature_algorithm(crl->crl,
						 "signatureAlgorithm.algorithm");
	if (result < 0) {
		gnutls_assert();
		if (verify)
			*verify |= GNUTLS_CERT_INVALID;
		goto cleanup;
	}

	hash_algo = gnutls_sign_get_hash_algorithm(result);

	/* issuer is not in trusted certificate
	 * authorities.
	 */
	if (issuer == NULL) {
		gnutls_assert();
		if (verify)
			*verify |=
			    GNUTLS_CERT_SIGNER_NOT_FOUND |
			    GNUTLS_CERT_INVALID;
	} else {
		if (!(flags & GNUTLS_VERIFY_DISABLE_CA_SIGN)) {
			if (gnutls_x509_crt_get_ca_status(issuer, NULL) != 1) {
				gnutls_assert();
				if (verify)
					*verify |=
					    GNUTLS_CERT_SIGNER_NOT_CA |
					    GNUTLS_CERT_INVALID;
			}

			result =
			    gnutls_x509_crt_get_key_usage(issuer, &usage, NULL);
			if (result != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE) {
				if (result < 0) {
					gnutls_assert();
					if (verify)
						*verify |= GNUTLS_CERT_INVALID;
				} else if (!(usage & GNUTLS_KEY_CRL_SIGN)) {
					gnutls_assert();
					if (verify)
						*verify |=
						    GNUTLS_CERT_SIGNER_CONSTRAINTS_FAILURE
						    | GNUTLS_CERT_INVALID;
				}
			}
		}

		result =
		    _gnutls_x509_verify_data(mac_to_entry(hash_algo),
					     &crl_signed_data, &crl_signature,
					     issuer);
		if (result == GNUTLS_E_PK_SIG_VERIFY_FAILED) {
			gnutls_assert();
			/* error. ignore it */
			if (verify)
				*verify |= GNUTLS_CERT_SIGNATURE_FAILURE;
			result = 0;
		} else if (result < 0) {
			gnutls_assert();
			if (verify)
				*verify |= GNUTLS_CERT_INVALID;
			goto cleanup;
		} else if (result >= 0) {
			result = 0; /* everything ok */
		}
	}

	{
		int sigalg;

		sigalg = gnutls_x509_crl_get_signature_algorithm(crl);

		if (((sigalg == GNUTLS_SIGN_RSA_MD2) &&
		     !(flags & GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD2)) ||
		    ((sigalg == GNUTLS_SIGN_RSA_MD5) &&
		     !(flags & GNUTLS_VERIFY_ALLOW_SIGN_RSA_MD5))) {
			if (verify)
				*verify |= GNUTLS_CERT_INSECURE_ALGORITHM;
			result = 0;
		}
	}

	if (gnutls_x509_crl_get_this_update(crl) > now && verify)
		*verify |= GNUTLS_CERT_REVOCATION_DATA_ISSUED_IN_FUTURE;

	if (gnutls_x509_crl_get_next_update(crl) < now && verify)
		*verify |= GNUTLS_CERT_REVOCATION_DATA_SUPERSEDED;


      cleanup:
	if (verify && *verify != 0)
		*verify |= GNUTLS_CERT_INVALID;

	_gnutls_free_datum(&crl_signed_data);
	_gnutls_free_datum(&crl_signature);

	return result;
}
