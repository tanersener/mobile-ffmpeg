/*
 * Copyright (C) 2000-2014 Free Software Foundation, Inc.
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

#ifndef _PARSER_AUX_H
#define _PARSER_AUX_H

/***************************************/
/*  Functions used by ASN.1 parser     */
/***************************************/
asn1_node _asn1_add_static_node (unsigned int type);

asn1_node
_asn1_set_value (asn1_node node, const void *value, unsigned int len);

asn1_node _asn1_set_value_m (asn1_node node, void *value, unsigned int len);

asn1_node
_asn1_set_value_lv (asn1_node node, const void *value, unsigned int len);

asn1_node
_asn1_append_value (asn1_node node, const void *value, unsigned int len);

asn1_node _asn1_set_name (asn1_node node, const char *name);

asn1_node _asn1_cpy_name (asn1_node dst, asn1_node src);

asn1_node _asn1_set_right (asn1_node node, asn1_node right);

asn1_node _asn1_get_last_right (asn1_node node);

void _asn1_remove_node (asn1_node node, unsigned int flags);

void _asn1_delete_list (void);

void _asn1_delete_list_and_nodes (void);

/* Max 64-bit integer length is 20 chars + 1 for sign + 1 for null termination */
#define LTOSTR_MAX_SIZE 22
char *_asn1_ltostr (int64_t v, char str[LTOSTR_MAX_SIZE]);

asn1_node _asn1_find_up (asn1_node node);

int _asn1_change_integer_value (asn1_node node);

int _asn1_expand_object_id (asn1_node node);

int _asn1_type_set_config (asn1_node node);

int _asn1_check_identifier (asn1_node node);

int _asn1_set_default_tag (asn1_node node);

/******************************************************************/
/* Function : _asn1_get_right                                     */
/* Description: returns the element pointed by the RIGHT field of */
/*              a NODE_ASN element.                               */
/* Parameters:                                                    */
/*   node: NODE_ASN element pointer.                              */
/* Return: field RIGHT of NODE.                                   */
/******************************************************************/
inline static asn1_node
_asn1_get_right (asn1_node node)
{
  if (node == NULL)
    return NULL;
  return node->right;
}

/******************************************************************/
/* Function : _asn1_set_down                                      */
/* Description: sets the field DOWN in a NODE_ASN element.        */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   down: pointer to a NODE_ASN element that you want be pointed */
/*          by NODE.                                              */
/* Return: pointer to *NODE.                                      */
/******************************************************************/
inline static asn1_node
_asn1_set_down (asn1_node node, asn1_node down)
{
  if (node == NULL)
    return node;
  node->down = down;
  if (down)
    down->left = node;
  return node;
}

/******************************************************************/
/* Function : _asn1_get_down                                      */
/* Description: returns the element pointed by the DOWN field of  */
/*              a NODE_ASN element.                               */
/* Parameters:                                                    */
/*   node: NODE_ASN element pointer.                              */
/* Return: field DOWN of NODE.                                    */
/******************************************************************/
inline static asn1_node
_asn1_get_down (asn1_node node)
{
  if (node == NULL)
    return NULL;
  return node->down;
}

/******************************************************************/
/* Function : _asn1_get_name                                      */
/* Description: returns the name of a NODE_ASN element.           */
/* Parameters:                                                    */
/*   node: NODE_ASN element pointer.                              */
/* Return: a null terminated string.                              */
/******************************************************************/
inline static char *
_asn1_get_name (asn1_node node)
{
  if (node == NULL)
    return NULL;
  return node->name;
}

/******************************************************************/
/* Function : _asn1_mod_type                                      */
/* Description: change the field TYPE of an NODE_ASN element.     */
/*              The new value is the old one | (bitwise or) the   */
/*              paramener VALUE.                                  */
/* Parameters:                                                    */
/*   node: NODE_ASN element pointer.                              */
/*   value: the integer value that must be or-ed with the current */
/*          value of field TYPE.                                  */
/* Return: NODE pointer.                                          */
/******************************************************************/
inline static asn1_node
_asn1_mod_type (asn1_node node, unsigned int value)
{
  if (node == NULL)
    return node;
  node->type |= value;
  return node;
}

#endif
