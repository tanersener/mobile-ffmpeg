/*
 * Copyright (C) 2002-2014 Free Software Foundation, Inc.
 *
 * This file is part of LIBTASN1.
 *
 * The LIBTASN1 library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */

/*************************************************/
/* File: structure.h                             */
/* Description: list of exported object by       */
/*   "structure.c"                               */
/*************************************************/

#ifndef _STRUCTURE_H
#define _STRUCTURE_H

int _asn1_create_static_structure (asn1_node pointer,
				   char *output_file_name, char *vector_name);

asn1_node _asn1_copy_structure3 (asn1_node source_node);

asn1_node _asn1_add_single_node (unsigned int type);

asn1_node _asn1_find_left (asn1_node node);

#endif
