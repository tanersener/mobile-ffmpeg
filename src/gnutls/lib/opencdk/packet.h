/* packet.h
 * Copyright (C) 2002-2012 Free Software Foundation, Inc.
 *
 * Author: Timo Schulz
 *
 * This file is part of OpenCDK.
 *
 * This library is free software; you can redistribute it and/or
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

#ifndef CDK_PACKET_H
#define CDK_PACKET_H

struct cdk_kbnode_s {
	struct cdk_kbnode_s *next;
	cdk_packet_t pkt;
	unsigned int is_deleted:1;
	unsigned int is_cloned:1;
};

/*-- new-packet.c --*/
void _cdk_free_mpibuf(size_t n, bigint_t * array);
void _cdk_free_userid(cdk_pkt_userid_t uid);
void _cdk_free_signature(cdk_pkt_signature_t sig);
cdk_prefitem_t _cdk_copy_prefs(const cdk_prefitem_t prefs);
cdk_error_t _cdk_copy_userid(cdk_pkt_userid_t * dst, cdk_pkt_userid_t src);
cdk_error_t _cdk_copy_pubkey(cdk_pkt_pubkey_t * dst, cdk_pkt_pubkey_t src);
cdk_error_t _cdk_copy_seckey(cdk_pkt_seckey_t * dst, cdk_pkt_seckey_t src);
cdk_error_t _cdk_copy_pk_to_sk(cdk_pkt_pubkey_t pk, cdk_pkt_seckey_t sk);
cdk_error_t _cdk_copy_signature(cdk_pkt_signature_t * dst,
				cdk_pkt_signature_t src);
cdk_error_t _cdk_pubkey_compare(cdk_pkt_pubkey_t a, cdk_pkt_pubkey_t b);

#endif				/* CDK_PACKET_H */
