/* new-packet.c - packet handling (freeing, copying, ...)
 * Copyright (C) 2001-2012 Free Software Foundation, Inc.
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
#include <string.h>
#include <stdio.h>

#include "opencdk.h"
#include "main.h"
#include "packet.h"


/* Release an array of MPI values. */
void _cdk_free_mpibuf(size_t n, bigint_t * array)
{
	while (n--) {
		_gnutls_mpi_release(&array[n]);
	}
}


/**
 * cdk_pkt_new:
 * @r_pkt: the new packet
 * 
 * Allocate a new packet.
 **/
cdk_error_t cdk_pkt_new(cdk_packet_t * r_pkt)
{
	cdk_packet_t pkt;

	if (!r_pkt)
		return CDK_Inv_Value;
	pkt = cdk_calloc(1, sizeof *pkt);
	if (!pkt)
		return CDK_Out_Of_Core;
	*r_pkt = pkt;
	return 0;
}


static void free_pubkey_enc(cdk_pkt_pubkey_enc_t enc)
{
	size_t nenc;

	if (!enc)
		return;

	nenc = cdk_pk_get_nenc(enc->pubkey_algo);
	_cdk_free_mpibuf(nenc, enc->mpi);
	cdk_free(enc);
}


static void free_literal(cdk_pkt_literal_t pt)
{
	if (!pt)
		return;
	/* The buffer which is referenced in this packet is closed
	   elsewhere. To close it here would cause a double close. */
	cdk_free(pt);
}


void _cdk_free_userid(cdk_pkt_userid_t uid)
{
	if (!uid)
		return;

	cdk_free(uid->prefs);
	uid->prefs = NULL;
	cdk_free(uid->attrib_img);
	uid->attrib_img = NULL;
	cdk_free(uid);
}


void _cdk_free_signature(cdk_pkt_signature_t sig)
{
	cdk_desig_revoker_t r;
	size_t nsig;

	if (!sig)
		return;

	nsig = cdk_pk_get_nsig(sig->pubkey_algo);
	_cdk_free_mpibuf(nsig, sig->mpi);

	cdk_subpkt_free(sig->hashed);
	sig->hashed = NULL;
	cdk_subpkt_free(sig->unhashed);
	sig->unhashed = NULL;
	while (sig->revkeys) {
		r = sig->revkeys->next;
		cdk_free(sig->revkeys);
		sig->revkeys = r;
	}
	cdk_free(sig);
}


void cdk_pk_release(cdk_pubkey_t pk)
{
	size_t npkey;

	if (!pk)
		return;

	npkey = cdk_pk_get_npkey(pk->pubkey_algo);
	_cdk_free_userid(pk->uid);
	pk->uid = NULL;
	cdk_free(pk->prefs);
	pk->prefs = NULL;
	_cdk_free_mpibuf(npkey, pk->mpi);
	cdk_free(pk);
}


void cdk_sk_release(cdk_seckey_t sk)
{
	size_t nskey;

	if (!sk)
		return;

	nskey = cdk_pk_get_nskey(sk->pubkey_algo);
	_cdk_free_mpibuf(nskey, sk->mpi);
	cdk_free(sk->encdata);
	sk->encdata = NULL;
	cdk_pk_release(sk->pk);
	sk->pk = NULL;
	cdk_s2k_free(sk->protect.s2k);
	sk->protect.s2k = NULL;
	cdk_free(sk);
}


/* Detach the openpgp packet from the packet structure
   and release the packet structure itself. */
void _cdk_pkt_detach_free(cdk_packet_t pkt, int *r_pkttype, void **ctx)
{
	/* For now we just allow this for keys. */
	switch (pkt->pkttype) {
	case CDK_PKT_PUBLIC_KEY:
	case CDK_PKT_PUBLIC_SUBKEY:
		*ctx = pkt->pkt.public_key;
		break;

	case CDK_PKT_SECRET_KEY:
	case CDK_PKT_SECRET_SUBKEY:
		*ctx = pkt->pkt.secret_key;
		break;

	default:
		*r_pkttype = 0;
		return;
	}

	/* The caller might expect a specific packet type and
	   is not interested to store it for later use. */
	if (r_pkttype)
		*r_pkttype = pkt->pkttype;

	cdk_free(pkt);
}


void cdk_pkt_free(cdk_packet_t pkt)
{
	if (!pkt)
		return;

	switch (pkt->pkttype) {
	case CDK_PKT_ATTRIBUTE:
	case CDK_PKT_USER_ID:
		_cdk_free_userid(pkt->pkt.user_id);
		break;
	case CDK_PKT_PUBLIC_KEY:
	case CDK_PKT_PUBLIC_SUBKEY:
		cdk_pk_release(pkt->pkt.public_key);
		break;
	case CDK_PKT_SECRET_KEY:
	case CDK_PKT_SECRET_SUBKEY:
		cdk_sk_release(pkt->pkt.secret_key);
		break;
	case CDK_PKT_SIGNATURE:
		_cdk_free_signature(pkt->pkt.signature);
		break;
	case CDK_PKT_PUBKEY_ENC:
		free_pubkey_enc(pkt->pkt.pubkey_enc);
		break;
	case CDK_PKT_MDC:
		cdk_free(pkt->pkt.mdc);
		break;
	case CDK_PKT_ONEPASS_SIG:
		cdk_free(pkt->pkt.onepass_sig);
		break;
	case CDK_PKT_LITERAL:
		free_literal(pkt->pkt.literal);
		break;
	case CDK_PKT_COMPRESSED:
		cdk_free(pkt->pkt.compressed);
		break;
	default:
		break;
	}

	/* Reset the packet type to avoid, when cdk_pkt_release() will be
	   used, that the second cdk_pkt_free() call will double free the data. */
	pkt->pkttype = 0;
}


/**
 * cdk_pkt_release:
 * @pkt: the packet
 * 
 * Free the contents of the given package and
 * release the memory of the structure.
 **/
void cdk_pkt_release(cdk_packet_t pkt)
{
	if (!pkt)
		return;
	cdk_pkt_free(pkt);
	cdk_free(pkt);
}


/**
 * cdk_pkt_alloc:
 * @r_pkt: output is the new packet
 * @pkttype: the requested packet type
 * 
 * Allocate a new packet structure with the given packet type.
 **/
cdk_error_t cdk_pkt_alloc(cdk_packet_t * r_pkt, cdk_packet_type_t pkttype)
{
	cdk_packet_t pkt;
	int rc;

	if (!r_pkt)
		return CDK_Inv_Value;

	rc = cdk_pkt_new(&pkt);
	if (rc)
		return rc;

	switch (pkttype) {
	case CDK_PKT_USER_ID:
		pkt->pkt.user_id = cdk_calloc(1, sizeof *pkt->pkt.user_id);
		if (!pkt->pkt.user_id)
			return CDK_Out_Of_Core;
		pkt->pkt.user_id->name = NULL;
		break;

	case CDK_PKT_PUBLIC_KEY:
	case CDK_PKT_PUBLIC_SUBKEY:
		pkt->pkt.public_key =
		    cdk_calloc(1, sizeof *pkt->pkt.public_key);
		if (!pkt->pkt.public_key)
			return CDK_Out_Of_Core;
		break;

	case CDK_PKT_SECRET_KEY:
	case CDK_PKT_SECRET_SUBKEY:
		pkt->pkt.secret_key =
		    cdk_calloc(1, sizeof *pkt->pkt.secret_key);
		if (!pkt->pkt.secret_key)
			return CDK_Out_Of_Core;

		pkt->pkt.secret_key->pk =
		    cdk_calloc(1, sizeof *pkt->pkt.secret_key->pk);
		if (!pkt->pkt.secret_key->pk) {
			cdk_free(pkt->pkt.secret_key);
			pkt->pkt.secret_key = NULL;
			return CDK_Out_Of_Core;
		}
		break;

	case CDK_PKT_SIGNATURE:
		pkt->pkt.signature =
		    cdk_calloc(1, sizeof *pkt->pkt.signature);
		if (!pkt->pkt.signature)
			return CDK_Out_Of_Core;
		break;

	case CDK_PKT_PUBKEY_ENC:
		pkt->pkt.pubkey_enc =
		    cdk_calloc(1, sizeof *pkt->pkt.pubkey_enc);
		if (!pkt->pkt.pubkey_enc)
			return CDK_Out_Of_Core;
		break;

	case CDK_PKT_MDC:
		pkt->pkt.mdc = cdk_calloc(1, sizeof *pkt->pkt.mdc);
		if (!pkt->pkt.mdc)
			return CDK_Out_Of_Core;
		break;

	case CDK_PKT_ONEPASS_SIG:
		pkt->pkt.onepass_sig =
		    cdk_calloc(1, sizeof *pkt->pkt.onepass_sig);
		if (!pkt->pkt.onepass_sig)
			return CDK_Out_Of_Core;
		break;

	case CDK_PKT_LITERAL:
		/* FIXME: We would need the size of the file name to allocate extra
		   bytes, otherwise the result would be useless. */
		pkt->pkt.literal = cdk_calloc(1, sizeof *pkt->pkt.literal);
		if (!pkt->pkt.literal)
			return CDK_Out_Of_Core;
		pkt->pkt.literal->name = NULL;
		break;

	default:
		return CDK_Not_Implemented;
	}
	pkt->pkttype = pkttype;
	*r_pkt = pkt;
	return 0;
}


cdk_prefitem_t _cdk_copy_prefs(const cdk_prefitem_t prefs)
{
	size_t n = 0;
	struct cdk_prefitem_s *new_prefs;

	if (!prefs)
		return NULL;

	for (n = 0; prefs[n].type; n++);
	new_prefs = cdk_calloc(1, sizeof *new_prefs * (n + 1));
	if (!new_prefs)
		return NULL;
	for (n = 0; prefs[n].type; n++) {
		new_prefs[n].type = prefs[n].type;
		new_prefs[n].value = prefs[n].value;
	}
	new_prefs[n].type = CDK_PREFTYPE_NONE;
	new_prefs[n].value = 0;
	return new_prefs;
}


cdk_error_t _cdk_copy_userid(cdk_pkt_userid_t * dst, cdk_pkt_userid_t src)
{
	cdk_pkt_userid_t u;

	if (!dst || !src)
		return CDK_Inv_Value;

	*dst = NULL;
	u = cdk_calloc(1, sizeof *u + strlen(src->name) + 2);
	if (!u)
		return CDK_Out_Of_Core;
	u->name = (char *) u + sizeof(*u);

	memcpy(u, src, sizeof *u);
	memcpy(u->name, src->name, strlen(src->name));
	u->prefs = _cdk_copy_prefs(src->prefs);
	if (src->selfsig)
		_cdk_copy_signature(&u->selfsig, src->selfsig);
	*dst = u;

	return 0;
}


cdk_error_t _cdk_copy_pubkey(cdk_pkt_pubkey_t * dst, cdk_pkt_pubkey_t src)
{
	cdk_pkt_pubkey_t k;
	int i;

	if (!dst || !src)
		return CDK_Inv_Value;

	*dst = NULL;
	k = cdk_calloc(1, sizeof *k);
	if (!k)
		return CDK_Out_Of_Core;
	memcpy(k, src, sizeof *k);
	if (src->uid)
		_cdk_copy_userid(&k->uid, src->uid);
	if (src->prefs)
		k->prefs = _cdk_copy_prefs(src->prefs);
	for (i = 0; i < cdk_pk_get_npkey(src->pubkey_algo); i++)
		k->mpi[i] = _gnutls_mpi_copy(src->mpi[i]);
	*dst = k;

	return 0;
}


cdk_error_t _cdk_copy_seckey(cdk_pkt_seckey_t * dst, cdk_pkt_seckey_t src)
{
	cdk_pkt_seckey_t k;
	int i;

	if (!dst || !src)
		return CDK_Inv_Value;

	*dst = NULL;
	k = cdk_calloc(1, sizeof *k);
	if (!k)
		return CDK_Out_Of_Core;
	memcpy(k, src, sizeof *k);
	_cdk_copy_pubkey(&k->pk, src->pk);

	if (src->encdata) {
		k->encdata = cdk_calloc(1, src->enclen + 1);
		if (!k->encdata)
			return CDK_Out_Of_Core;
		memcpy(k->encdata, src->encdata, src->enclen);
	}

	_cdk_s2k_copy(&k->protect.s2k, src->protect.s2k);
	for (i = 0; i < cdk_pk_get_nskey(src->pubkey_algo); i++) {
		k->mpi[i] = _gnutls_mpi_copy(src->mpi[i]);
	}

	*dst = k;
	return 0;
}


cdk_error_t _cdk_copy_pk_to_sk(cdk_pkt_pubkey_t pk, cdk_pkt_seckey_t sk)
{
	if (!pk || !sk)
		return CDK_Inv_Value;

	sk->version = pk->version;
	sk->expiredate = pk->expiredate;
	sk->pubkey_algo = _pgp_pub_algo_to_cdk(pk->pubkey_algo);
	sk->has_expired = pk->has_expired;
	sk->is_revoked = pk->is_revoked;
	sk->main_keyid[0] = pk->main_keyid[0];
	sk->main_keyid[1] = pk->main_keyid[1];
	sk->keyid[0] = pk->keyid[0];
	sk->keyid[1] = pk->keyid[1];

	return 0;
}


cdk_error_t
_cdk_copy_signature(cdk_pkt_signature_t * dst, cdk_pkt_signature_t src)
{
	cdk_pkt_signature_t s;

	if (!dst || !src)
		return CDK_Inv_Value;

	*dst = NULL;
	s = cdk_calloc(1, sizeof *s);
	if (!s)
		return CDK_Out_Of_Core;
	memcpy(s, src, sizeof *src);
	_cdk_subpkt_copy(&s->hashed, src->hashed);
	_cdk_subpkt_copy(&s->unhashed, src->unhashed);
	/* FIXME: Copy MPI parts */
	*dst = s;

	return 0;
}


cdk_error_t _cdk_pubkey_compare(cdk_pkt_pubkey_t a, cdk_pkt_pubkey_t b)
{
	int na, nb, i;

	if (a->timestamp != b->timestamp
	    || a->pubkey_algo != b->pubkey_algo)
		return -1;
	if (a->version < 4 && a->expiredate != b->expiredate)
		return -1;
	na = cdk_pk_get_npkey(a->pubkey_algo);
	nb = cdk_pk_get_npkey(b->pubkey_algo);
	if (na != nb)
		return -1;

	for (i = 0; i < na; i++) {
		if (_gnutls_mpi_cmp(a->mpi[i], b->mpi[i]))
			return -1;
	}

	return 0;
}


/**
 * cdk_subpkt_free:
 * @ctx: the sub packet node to free
 *
 * Release the context.
 **/
void cdk_subpkt_free(cdk_subpkt_t ctx)
{
	cdk_subpkt_t s;

	while (ctx) {
		s = ctx->next;
		cdk_free(ctx);
		ctx = s;
	}
}


/**
 * cdk_subpkt_find:
 * @ctx: the sub packet node
 * @type: the packet type to find
 *
 * Find the given packet type in the node. If no packet with this
 * type was found, return null otherwise pointer to the node.
 **/
cdk_subpkt_t cdk_subpkt_find(cdk_subpkt_t ctx, size_t type)
{
	return cdk_subpkt_find_nth(ctx, type, 0);
}

/**
 * cdk_subpkt_type_count:
 * @ctx: The sub packet context
 * @type: The sub packet type.
 * 
 * Return the amount of sub packets with this type.
 **/
size_t cdk_subpkt_type_count(cdk_subpkt_t ctx, size_t type)
{
	cdk_subpkt_t s;
	size_t count;

	count = 0;
	for (s = ctx; s; s = s->next) {
		if (s->type == type)
			count++;
	}

	return count;
}


/**
 * cdk_subpkt_find_nth:
 * @ctx: The sub packet context
 * @type: The sub packet type
 * @index: The nth packet to retrieve, 0 means the first
 * 
 * Return the nth sub packet of the given type.
 **/
cdk_subpkt_t cdk_subpkt_find_nth(cdk_subpkt_t ctx, size_t type, size_t idx)
{
	cdk_subpkt_t s;
	size_t pos;

	pos = 0;
	for (s = ctx; s; s = s->next) {
		if (s->type == type && pos++ == idx)
			return s;
	}

	return NULL;
}


/**
 * cdk_subpkt_new:
 * @size: the size of the new context
 *
 * Create a new sub packet node with the size of @size.
 **/
cdk_subpkt_t cdk_subpkt_new(size_t size)
{
	cdk_subpkt_t s;

	if (!size)
		return NULL;
	s = cdk_calloc(1, sizeof *s + size + 2);
	if (!s)
		return NULL;
	s->d = (byte *) s + sizeof(*s);

	return s;
}


/**
 * cdk_subpkt_get_data:
 * @ctx: the sub packet node
 * @r_type: pointer store the packet type
 * @r_nbytes: pointer to store the packet size
 *
 * Extract the data from the given sub packet. The type is returned
 * in @r_type and the size in @r_nbytes.
 **/
const byte *cdk_subpkt_get_data(cdk_subpkt_t ctx, size_t * r_type,
				size_t * r_nbytes)
{
	if (!ctx || !r_nbytes)
		return NULL;
	if (r_type)
		*r_type = ctx->type;
	*r_nbytes = ctx->size;
	return ctx->d;
}


/**
 * cdk_subpkt_add:
 * @root: the root node
 * @node: the node to add
 *
 * Add the node in @node to the root node @root.
 **/
cdk_error_t cdk_subpkt_add(cdk_subpkt_t root, cdk_subpkt_t node)
{
	cdk_subpkt_t n1;

	if (!root)
		return CDK_Inv_Value;
	for (n1 = root; n1->next; n1 = n1->next);
	n1->next = node;
	return 0;
}


byte *_cdk_subpkt_get_array(cdk_subpkt_t s, int count, size_t * r_nbytes)
{
	cdk_subpkt_t list;
	byte *buf;
	size_t n, nbytes;

	if (!s) {
		if (r_nbytes)
			*r_nbytes = 0;
		return NULL;
	}

	for (n = 0, list = s; list; list = list->next) {
		n++;		/* type */
		n += list->size;
		if (list->size < 192)
			n++;
		else if (list->size < 8384)
			n += 2;
		else
			n += 5;
	}
	buf = cdk_calloc(1, n + 1);
	if (!buf)
		return NULL;

	n = 0;
	for (list = s; list; list = list->next) {
		nbytes = 1 + list->size;	/* type */
		if (nbytes < 192)
			buf[n++] = nbytes;
		else if (nbytes < 8384) {
			nbytes -= 192;
			buf[n++] = nbytes / 256 + 192;
			buf[n++] = nbytes & 0xff;
		} else {
			buf[n++] = 0xFF;
			buf[n++] = nbytes >> 24;
			buf[n++] = nbytes >> 16;
			buf[n++] = nbytes >> 8;
			buf[n++] = nbytes;
		}

		buf[n++] = list->type;
		memcpy(buf + n, list->d, list->size);
		n += list->size;
	}

	if (count) {
		cdk_free(buf);
		buf = NULL;
	}
	if (r_nbytes)
		*r_nbytes = n;
	return buf;
}


cdk_error_t _cdk_subpkt_copy(cdk_subpkt_t * r_dst, cdk_subpkt_t src)
{
	cdk_subpkt_t root, p, node;

	if (!src || !r_dst)
		return CDK_Inv_Value;

	root = NULL;
	for (p = src; p; p = p->next) {
		node = cdk_subpkt_new(p->size);
		if (node) {
			memcpy(node->d, p->d, p->size);
			node->type = p->type;
			node->size = p->size;
		}
		if (!root)
			root = node;
		else
			cdk_subpkt_add(root, node);
	}
	*r_dst = root;
	return 0;
}


/**
 * cdk_subpkt_init:
 * @node: the sub packet node
 * @type: type of the packet which data should be initialized
 * @buf: the buffer with the actual data
 * @buflen: the size of the data
 *
 * Set the packet data of the given root and set the type of it.
 **/
void
cdk_subpkt_init(cdk_subpkt_t node, size_t type,
		const void *buf, size_t buflen)
{
	if (!node)
		return;
	node->type = type;
	node->size = buflen;
	memcpy(node->d, buf, buflen);
}


/* FIXME: We need to think of a public interface for it. */
const byte *cdk_key_desig_revoker_walk(cdk_desig_revoker_t root,
				       cdk_desig_revoker_t * ctx,
				       int *r_class, int *r_algid)
{
	cdk_desig_revoker_t n;

	if (!*ctx) {
		*ctx = root;
		n = root;
	} else {
		n = (*ctx)->next;
		*ctx = n;
	}

	if (n && r_class && r_algid) {
		*r_class = n->r_class;
		*r_algid = n->algid;
	}

	return n ? n->fpr : NULL;
}


/**
 * cdk_subpkt_find_next:
 * @root: the base where to begin the iteration
 * @type: the type to find or 0 for the next node.
 * 
 * Try to find the next node after @root with type.
 * If type is 0, the next node will be returned.
 **/
cdk_subpkt_t cdk_subpkt_find_next(cdk_subpkt_t root, size_t type)
{
	cdk_subpkt_t node;

	for (node = root->next; node; node = node->next) {
		if (!type)
			return node;
		else if (node->type == type)
			return node;
	}

	return NULL;
}
