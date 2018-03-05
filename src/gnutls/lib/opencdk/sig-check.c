/* sig-check.c - Check signatures
 * Copyright (C) 1998-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz
 *
 * This file is part of OpenCDK.
 *
 * The OpenCDK library is free software; you can redistribute it and/or
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include "gnutls_int.h"
#include <algorithms.h>

#include "opencdk.h"
#include "main.h"
#include "packet.h"

/* Hash all multi precision integers of the key PK with the given
   message digest context MD. */
static int hash_mpibuf(cdk_pubkey_t pk, digest_hd_st * md, int usefpr)
{
	byte buf[MAX_MPI_BYTES];	/* FIXME: do not use hardcoded length. */
	size_t nbytes;
	size_t i, npkey;
	int err;

	/* We have to differ between two modes for v3 keys. To form the
	   fingerprint, we hash the MPI values without the length prefix.
	   But if we calculate the hash for verifying/signing we use all data. */
	npkey = cdk_pk_get_npkey(pk->pubkey_algo);
	for (i = 0; i < npkey; i++) {
		nbytes = MAX_MPI_BYTES;
		err = _gnutls_mpi_print_pgp(pk->mpi[i], buf, &nbytes);

		if (err < 0) {
			gnutls_assert();
			return map_gnutls_error(err);
		}

		if (!usefpr || pk->version == 4)
			_gnutls_hash(md, buf, nbytes);
		else		/* without the prefix. */
			_gnutls_hash(md, buf + 2, nbytes - 2);
	}
	return 0;
}


/* Hash an entire public key PK with the given message digest context
   MD. The @usefpr param is only valid for version 3 keys because of
   the different way to calculate the fingerprint. */
cdk_error_t
_cdk_hash_pubkey(cdk_pubkey_t pk, digest_hd_st * md, int usefpr)
{
	byte buf[12];
	size_t i, n, npkey;

	if (!pk || !md)
		return CDK_Inv_Value;

	if (usefpr && pk->version < 4 && is_RSA(pk->pubkey_algo))
		return hash_mpibuf(pk, md, 1);

	/* The version 4 public key packet does not have the 2 octets for
	   the expiration date. */
	n = pk->version < 4 ? 8 : 6;
	npkey = cdk_pk_get_npkey(pk->pubkey_algo);
	for (i = 0; i < npkey; i++)
		n = n + (_gnutls_mpi_get_nbits(pk->mpi[i]) + 7) / 8 + 2;

	i = 0;
	buf[i++] = 0x99;
	buf[i++] = n >> 8;
	buf[i++] = n >> 0;
	buf[i++] = pk->version;
	buf[i++] = pk->timestamp >> 24;
	buf[i++] = pk->timestamp >> 16;
	buf[i++] = pk->timestamp >> 8;
	buf[i++] = pk->timestamp >> 0;

	if (pk->version < 4) {
		u16 a = 0;

		/* Convert the expiration date into days. */
		if (pk->expiredate)
			a = (u16) ((pk->expiredate -
				    pk->timestamp) / 86400L);
		buf[i++] = a >> 8;
		buf[i++] = a;
	}
	buf[i++] = pk->pubkey_algo;
	_gnutls_hash(md, buf, i);
	return hash_mpibuf(pk, md, 0);
}


/* Hash the user ID @uid with the given message digest @md.
   Use openpgp mode if @is_v4 is 1. */
cdk_error_t
_cdk_hash_userid(cdk_pkt_userid_t uid, int is_v4, digest_hd_st * md)
{
	const byte *data;
	byte buf[5];
	u32 dlen;

	if (!uid || !md)
		return CDK_Inv_Value;

	if (!is_v4) {
		_gnutls_hash(md, (byte *) uid->name, uid->len);
		return 0;
	}

	dlen = uid->attrib_img ? uid->attrib_len : uid->len;
	data = uid->attrib_img ? uid->attrib_img : (byte *) uid->name;
	buf[0] = uid->attrib_img ? 0xD1 : 0xB4;
	buf[1] = dlen >> 24;
	buf[2] = dlen >> 16;
	buf[3] = dlen >> 8;
	buf[4] = dlen >> 0;
	_gnutls_hash(md, buf, 5);
	_gnutls_hash(md, data, dlen);
	return 0;
}


/* Hash all parts of the signature which are needed to derive
   the correct message digest to verify the sig. */
cdk_error_t _cdk_hash_sig_data(cdk_pkt_signature_t sig, digest_hd_st * md)
{
	byte buf[4];
	byte tmp;

	if (!sig || !md)
		return CDK_Inv_Value;

	if (sig->version == 4)
		_gnutls_hash(md, &sig->version, 1);

	_gnutls_hash(md, &sig->sig_class, 1);
	if (sig->version < 4) {
		buf[0] = sig->timestamp >> 24;
		buf[1] = sig->timestamp >> 16;
		buf[2] = sig->timestamp >> 8;
		buf[3] = sig->timestamp >> 0;
		_gnutls_hash(md, buf, 4);
	} else {
		size_t n;

		tmp = _cdk_pub_algo_to_pgp(sig->pubkey_algo);
		_gnutls_hash(md, &tmp, 1);
		tmp = _gnutls_hash_algo_to_pgp(sig->digest_algo);
		_gnutls_hash(md, &tmp, 1);
		if (sig->hashed != NULL) {
			byte *p =
			    _cdk_subpkt_get_array(sig->hashed, 0, &n);
			if (p == NULL)
				return gnutls_assert_val(CDK_Inv_Value);

			buf[0] = n >> 8;
			buf[1] = n >> 0;
			_gnutls_hash(md, buf, 2);
			_gnutls_hash(md, p, n);
			cdk_free(p);
			sig->hashed_size = n;
			n = sig->hashed_size + 6;
		} else {
			tmp = 0x00;
			_gnutls_hash(md, &tmp, 1);
			_gnutls_hash(md, &tmp, 1);
			n = 6;
		}
		_gnutls_hash(md, &sig->version, 1);
		tmp = 0xff;
		_gnutls_hash(md, &tmp, 1);
		buf[0] = n >> 24;
		buf[1] = n >> 16;
		buf[2] = n >> 8;
		buf[3] = n >> 0;
		_gnutls_hash(md, buf, 4);
	}
	return 0;
}


/* Cache the signature result and store it inside the sig. */
static void cache_sig_result(cdk_pkt_signature_t sig, int res)
{
	sig->flags.checked = 0;
	sig->flags.valid = 0;
	if (res == 0) {
		sig->flags.checked = 1;
		sig->flags.valid = 1;
	} else if (res == CDK_Bad_Sig) {
		sig->flags.checked = 1;
		sig->flags.valid = 0;
	}
}


/* Check the given signature @sig with the public key @pk.
   Use the digest handle @digest. */
cdk_error_t
_cdk_sig_check(cdk_pubkey_t pk, cdk_pkt_signature_t sig,
	       digest_hd_st * digest, int *r_expired)
{
	cdk_error_t rc;
	byte md[MAX_DIGEST_LEN];
	time_t cur_time = (u32) gnutls_time(NULL);

	if (!pk || !sig || !digest) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	if (sig->flags.checked)
		return sig->flags.valid ? 0 : CDK_Bad_Sig;
	if (!KEY_CAN_SIGN(pk->pubkey_algo))
		return CDK_Inv_Algo;
	if (pk->timestamp > sig->timestamp || pk->timestamp > cur_time)
		return CDK_Time_Conflict;

	if (r_expired && pk->expiredate
	    && (pk->expiredate + pk->timestamp) > cur_time)
		*r_expired = 1;

	_cdk_hash_sig_data(sig, digest);
	_gnutls_hash_output(digest, md);

	if (md[0] != sig->digest_start[0] || md[1] != sig->digest_start[1]) {
		gnutls_assert();
		return CDK_Chksum_Error;
	}

	rc = cdk_pk_verify(pk, sig, md);
	cache_sig_result(sig, rc);
	return rc;
}


/* Check the given key signature.
   @knode is the key node and @snode the signature node. */
cdk_error_t
_cdk_pk_check_sig(cdk_keydb_hd_t keydb,
		  cdk_kbnode_t knode, cdk_kbnode_t snode, int *is_selfsig,
		  char **ret_uid)
{
	digest_hd_st md;
	int err;
	cdk_pubkey_t pk;
	cdk_pkt_signature_t sig;
	cdk_kbnode_t node;
	cdk_error_t rc = 0;
	int is_expired;

	if (!knode || !snode) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	if (is_selfsig)
		*is_selfsig = 0;
	if ((knode->pkt->pkttype != CDK_PKT_PUBLIC_KEY &&
	     knode->pkt->pkttype != CDK_PKT_PUBLIC_SUBKEY) ||
	    snode->pkt->pkttype != CDK_PKT_SIGNATURE) {
		gnutls_assert();
		return CDK_Inv_Value;
	}
	pk = knode->pkt->pkt.public_key;
	sig = snode->pkt->pkt.signature;

	err = _gnutls_hash_init(&md, mac_to_entry(sig->digest_algo));
	if (err < 0) {
		gnutls_assert();
		return map_gnutls_error(err);
	}

	is_expired = 0;
	if (sig->sig_class == 0x20) {	/* key revocation */
		cdk_kbnode_hash(knode, &md, 0, 0, 0);
		rc = _cdk_sig_check(pk, sig, &md, &is_expired);
	} else if (sig->sig_class == 0x28) {	/* subkey revocation */
		node =
		    cdk_kbnode_find_prev(knode, snode,
					 CDK_PKT_PUBLIC_SUBKEY);
		if (!node) {	/* no subkey for subkey revocation packet */
			gnutls_assert();
			rc = CDK_Error_No_Key;
			goto fail;
		}
		cdk_kbnode_hash(knode, &md, 0, 0, 0);
		cdk_kbnode_hash(node, &md, 0, 0, 0);
		rc = _cdk_sig_check(pk, sig, &md, &is_expired);
	} else if (sig->sig_class == 0x18 || sig->sig_class == 0x19) {	/* primary/secondary key binding */
		node =
		    cdk_kbnode_find_prev(knode, snode,
					 CDK_PKT_PUBLIC_SUBKEY);
		if (!node) {	/* no subkey for subkey binding packet */
			gnutls_assert();
			rc = CDK_Error_No_Key;
			goto fail;
		}
		cdk_kbnode_hash(knode, &md, 0, 0, 0);
		cdk_kbnode_hash(node, &md, 0, 0, 0);
		rc = _cdk_sig_check(pk, sig, &md, &is_expired);
	} else if (sig->sig_class == 0x1F) {	/* direct key signature */
		cdk_kbnode_hash(knode, &md, 0, 0, 0);
		rc = _cdk_sig_check(pk, sig, &md, &is_expired);
	} else {		/* all other classes */
		cdk_pkt_userid_t uid;
		node = cdk_kbnode_find_prev(knode, snode, CDK_PKT_USER_ID);
		if (!node) {	/* no user ID for key signature packet */
			gnutls_assert();
			rc = CDK_Error_No_Key;
			goto fail;
		}

		uid = node->pkt->pkt.user_id;
		if (ret_uid) {
			*ret_uid = uid->name;
		}
		cdk_kbnode_hash(knode, &md, 0, 0, 0);
		cdk_kbnode_hash(node, &md, sig->version == 4, 0, 0);

		if (pk->keyid[0] == sig->keyid[0]
		    && pk->keyid[1] == sig->keyid[1]) {
			rc = _cdk_sig_check(pk, sig, &md, &is_expired);
			if (is_selfsig)
				*is_selfsig = 1;
		} else if (keydb != NULL) {
			cdk_pubkey_t sig_pk;
			rc = cdk_keydb_get_pk(keydb, sig->keyid, &sig_pk);
			if (!rc)
				rc = _cdk_sig_check(sig_pk, sig, &md,
						    &is_expired);
			cdk_pk_release(sig_pk);
		}
	}
      fail:
	_gnutls_hash_deinit(&md, NULL);
	return rc;
}

struct verify_uid {
	const char *name;
	int nsigs;
	struct verify_uid *next;
};

static int
uid_list_add_sig(struct verify_uid **list, const char *uid,
		 unsigned int flag)
{
	if (*list == NULL) {
		*list = cdk_calloc(1, sizeof(struct verify_uid));
		if (*list == NULL)
			return CDK_Out_Of_Core;
		(*list)->name = uid;

		if (flag != 0)
			(*list)->nsigs++;
	} else {
		struct verify_uid *p, *prev_p = NULL;
		int found = 0;

		p = *list;

		while (p != NULL) {
			if (strcmp(uid, p->name) == 0) {
				found = 1;
				break;
			}
			prev_p = p;
			p = p->next;
		}

		if (found == 0) {	/* not found add to the last */
			prev_p->next =
			    cdk_calloc(1, sizeof(struct verify_uid));
			if (prev_p->next == NULL)
				return CDK_Out_Of_Core;
			prev_p->next->name = uid;
			if (flag != 0)
				prev_p->next->nsigs++;
		} else {	/* found... increase sigs */
			if (flag != 0)
				p->nsigs++;
		}
	}

	return CDK_Success;
}

static void uid_list_free(struct verify_uid *list)
{
	struct verify_uid *p, *p1;

	p = list;
	while (p != NULL) {
		p1 = p->next;
		cdk_free(p);
		p = p1;
	}
}

/* returns non (0) if all UIDs in the list have at least one
 * signature. If the list is empty or no signatures are present
 * a (0) value is returned.
 */
static int uid_list_all_signed(struct verify_uid *list)
{
	struct verify_uid *p;

	if (list == NULL)
		return 0;

	p = list;
	while (p != NULL) {
		if (p->nsigs == 0) {
			return 0;
		}
		p = p->next;
	}
	return 1;		/* all signed */
}

/**
 * cdk_pk_check_sigs:
 * @key: the public key
 * @hd: an optinal key database handle
 * @r_status: variable to store the status of the key
 *
 * Check all signatures. When no key is available for checking, the
 * sigstat is marked as 'NOKEY'. The @r_status contains the key flags
 * which are or-ed or (0) when there are no flags.
 **/
cdk_error_t
cdk_pk_check_sigs(cdk_kbnode_t key, cdk_keydb_hd_t keydb, int *r_status)
{
	cdk_pkt_signature_t sig;
	cdk_kbnode_t node;
	cdk_error_t rc;
	u32 keyid;
	int key_status, is_selfsig = 0;
	struct verify_uid *uid_list = NULL;
	char *uid_name = NULL;

	if (!key || !r_status) {
		gnutls_assert();
		return CDK_Inv_Value;
	}

	*r_status = 0;
	node = cdk_kbnode_find(key, CDK_PKT_PUBLIC_KEY);
	if (!node) {
		gnutls_assert();
		return CDK_Error_No_Key;
	}

	key_status = 0;
	/* Continue with the signature check but adjust the
	   key status flags accordingly. */
	if (node->pkt->pkt.public_key->is_revoked)
		key_status |= CDK_KEY_REVOKED;
	if (node->pkt->pkt.public_key->has_expired)
		key_status |= CDK_KEY_EXPIRED;
	rc = 0;

	keyid = cdk_pk_get_keyid(node->pkt->pkt.public_key, NULL);
	for (node = key; node; node = node->next) {
		if (node->pkt->pkttype != CDK_PKT_SIGNATURE)
			continue;
		sig = node->pkt->pkt.signature;
		rc = _cdk_pk_check_sig(keydb, key, node, &is_selfsig,
				       &uid_name);

		if (rc && rc != CDK_Error_No_Key) {
			/* It might be possible that a single signature has been
			   corrupted, thus we do not consider it a problem when
			   one ore more signatures are bad. But at least the self
			   signature has to be valid. */
			if (is_selfsig) {
				key_status |= CDK_KEY_INVALID;
				break;
			}
		}

		_cdk_log_debug("signature %s: signer %08X keyid %08X\n",
			       rc == CDK_Bad_Sig ? "BAD" : "good",
			       (unsigned int) sig->keyid[1],
			       (unsigned int) keyid);

		if (IS_UID_SIG(sig) && uid_name != NULL) {
			/* add every uid in the uid list. Only consider valid:
			 * - verification was ok
			 * - not a selfsig
			 */
			rc = uid_list_add_sig(&uid_list, uid_name,
					      (rc == CDK_Success
					       && is_selfsig ==
					       0) ? 1 : 0);
			if (rc != CDK_Success) {
				gnutls_assert();
				goto exit;
			}
		}

	}

	if (uid_list_all_signed(uid_list) == 0)
		key_status |= CDK_KEY_NOSIGNER;
	*r_status = key_status;
	if (rc == CDK_Error_No_Key)
		rc = 0;

      exit:
	uid_list_free(uid_list);
	return rc;
}


/**
 * cdk_pk_check_self_sig:
 * @key: the key node
 * @r_status: output the status of the key.
 *
 * A convenient function to make sure the key is valid.
 * Valid means the self signature is ok.
 **/
cdk_error_t cdk_pk_check_self_sig(cdk_kbnode_t key, int *r_status)
{
	cdk_pkt_signature_t sig;
	cdk_kbnode_t node;
	cdk_error_t rc;
	u32 keyid[2], sigid[2];
	int is_selfsig, sig_ok;
	cdk_kbnode_t p, ctx = NULL;
	cdk_packet_t pkt;

	if (!key || !r_status)
		return CDK_Inv_Value;

	cdk_pk_get_keyid(key->pkt->pkt.public_key, keyid);

	while ((p = cdk_kbnode_walk(key, &ctx, 0))) {
		pkt = cdk_kbnode_get_packet(p);
		if (pkt->pkttype != CDK_PKT_PUBLIC_SUBKEY
		    && pkt->pkttype != CDK_PKT_PUBLIC_KEY)
			continue;

		/* FIXME: we should set expire/revoke here also but callers
		   expect CDK_KEY_VALID=0 if the key is okay. */
		sig_ok = 0;
		for (node = p; node; node = node->next) {
			if (node->pkt->pkttype != CDK_PKT_SIGNATURE)
				continue;
			sig = node->pkt->pkt.signature;

			cdk_sig_get_keyid(sig, sigid);
			if (sigid[0] != keyid[0] || sigid[1] != keyid[1])
				continue;
			/* FIXME: Now we check all self signatures. */
			rc = _cdk_pk_check_sig(NULL, p, node, &is_selfsig,
					       NULL);
			if (rc) {
				*r_status = CDK_KEY_INVALID;
				return rc;
			} else	/* For each valid self sig we increase this counter. */
				sig_ok++;
		}

		/* A key without a self signature is not valid. At least one
		 * signature for the given key has to be found.
		 */
		if (!sig_ok) {
			*r_status = CDK_KEY_INVALID;
			return CDK_General_Error;
		}
	}

	/* No flags indicate a valid key. */
	*r_status = CDK_KEY_VALID;

	return 0;
}
