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

/*****************************************************/
/* File: element.c                                   */
/* Description: Functions with the read and write    */
/*   functions.                                      */
/*****************************************************/


#include <int.h>
#include "parser_aux.h"
#include <gstr.h>
#include "structure.h"

#include "element.h"

void
_asn1_hierarchical_name (asn1_node node, char *name, int name_size)
{
  asn1_node p;
  char tmp_name[64];

  p = node;

  name[0] = 0;

  while (p != NULL)
    {
      if (p->name[0] != 0)
	{
	  _asn1_str_cpy (tmp_name, sizeof (tmp_name), name),
	    _asn1_str_cpy (name, name_size, p->name);
	  _asn1_str_cat (name, name_size, ".");
	  _asn1_str_cat (name, name_size, tmp_name);
	}
      p = _asn1_find_up (p);
    }

  if (name[0] == 0)
    _asn1_str_cpy (name, name_size, "ROOT");
}


/******************************************************************/
/* Function : _asn1_convert_integer                               */
/* Description: converts an integer from a null terminated string */
/*              to der decoding. The convertion from a null       */
/*              terminated string to an integer is made with      */
/*              the 'strtol' function.                            */
/* Parameters:                                                    */
/*   value: null terminated string to convert.                    */
/*   value_out: convertion result (memory must be already         */
/*              allocated).                                       */
/*   value_out_size: number of bytes of value_out.                */
/*   len: number of significant byte of value_out.                */
/* Return: ASN1_MEM_ERROR or ASN1_SUCCESS                         */
/******************************************************************/
int
_asn1_convert_integer (const unsigned char *value, unsigned char *value_out,
		       int value_out_size, int *len)
{
  char negative;
  unsigned char val[SIZEOF_UNSIGNED_LONG_INT];
  long valtmp;
  int k, k2;

  valtmp = _asn1_strtol (value, NULL, 10);

  for (k = 0; k < SIZEOF_UNSIGNED_LONG_INT; k++)
    {
      val[SIZEOF_UNSIGNED_LONG_INT - k - 1] = (valtmp >> (8 * k)) & 0xFF;
    }

  if (val[0] & 0x80)
    negative = 1;
  else
    negative = 0;

  for (k = 0; k < SIZEOF_UNSIGNED_LONG_INT - 1; k++)
    {
      if (negative && (val[k] != 0xFF))
	break;
      else if (!negative && val[k])
	break;
    }

  if ((negative && !(val[k] & 0x80)) || (!negative && (val[k] & 0x80)))
    k--;

  *len = SIZEOF_UNSIGNED_LONG_INT - k;

  if (SIZEOF_UNSIGNED_LONG_INT - k > value_out_size)
    /* VALUE_OUT is too short to contain the value conversion */
    return ASN1_MEM_ERROR;

  if (value_out != NULL)
    {
      for (k2 = k; k2 < SIZEOF_UNSIGNED_LONG_INT; k2++)
        value_out[k2 - k] = val[k2];
    }

#if 0
  printf ("_asn1_convert_integer: valueIn=%s, lenOut=%d", value, *len);
  for (k = 0; k < SIZEOF_UNSIGNED_LONG_INT; k++)
    printf (", vOut[%d]=%d", k, value_out[k]);
  printf ("\n");
#endif

  return ASN1_SUCCESS;
}

/* Appends a new element into the sequence (or set) defined by this
 * node. The new element will have a name of '?number', where number
 * is a monotonically increased serial number.
 *
 * The last element in the list may be provided in @pcache, to avoid
 * traversing the list, an expensive operation in long lists.
 *
 * On success it returns in @pcache the added element (which is the 
 * tail in the list of added elements).
 */
int
_asn1_append_sequence_set (asn1_node node, struct node_tail_cache_st *pcache)
{
  asn1_node p, p2;
  char temp[LTOSTR_MAX_SIZE];
  long n;

  if (!node || !(node->down))
    return ASN1_GENERIC_ERROR;

  p = node->down;
  while ((type_field (p->type) == ASN1_ETYPE_TAG)
	 || (type_field (p->type) == ASN1_ETYPE_SIZE))
    p = p->right;

  p2 = _asn1_copy_structure3 (p);
  if (p2 == NULL)
    return ASN1_GENERIC_ERROR;

  if (pcache == NULL || pcache->tail == NULL || pcache->head != node)
    {
      while (p->right)
        {
          p = p->right;
        }
    }
  else
    {
      p = pcache->tail;
    }

  _asn1_set_right (p, p2);
  if (pcache)
    {
      pcache->head = node;
      pcache->tail = p2;
    }

  if (p->name[0] == 0)
    _asn1_str_cpy (temp, sizeof (temp), "?1");
  else
    {
      n = strtol (p->name + 1, NULL, 0);
      n++;
      temp[0] = '?';
      _asn1_ltostr (n, temp + 1);
    }
  _asn1_set_name (p2, temp);
  /*  p2->type |= CONST_OPTION; */

  return ASN1_SUCCESS;
}


/**
 * asn1_write_value:
 * @node_root: pointer to a structure
 * @name: the name of the element inside the structure that you want to set.
 * @ivalue: vector used to specify the value to set. If len is >0,
 *   VALUE must be a two's complement form integer.  if len=0 *VALUE
 *   must be a null terminated string with an integer value.
 * @len: number of bytes of *value to use to set the value:
 *   value[0]..value[len-1] or 0 if value is a null terminated string
 *
 * Set the value of one element inside a structure.
 *
 * If an element is OPTIONAL and you want to delete it, you must use
 * the value=NULL and len=0.  Using "pkix.asn":
 *
 * result=asn1_write_value(cert, "tbsCertificate.issuerUniqueID",
 * NULL, 0);
 *
 * Description for each type:
 *
 * INTEGER: VALUE must contain a two's complement form integer.
 *
 *            value[0]=0xFF ,               len=1 -> integer=-1.
 *            value[0]=0xFF value[1]=0xFF , len=2 -> integer=-1.
 *            value[0]=0x01 ,               len=1 -> integer= 1.
 *            value[0]=0x00 value[1]=0x01 , len=2 -> integer= 1.
 *            value="123"                 , len=0 -> integer= 123.
 *
 * ENUMERATED: As INTEGER (but only with not negative numbers).
 *
 * BOOLEAN: VALUE must be the null terminated string "TRUE" or
 *   "FALSE" and LEN != 0.
 *
 *            value="TRUE" , len=1 -> boolean=TRUE.
 *            value="FALSE" , len=1 -> boolean=FALSE.
 *
 * OBJECT IDENTIFIER: VALUE must be a null terminated string with
 *   each number separated by a dot (e.g. "1.2.3.543.1").  LEN != 0.
 *
 *            value="1 2 840 10040 4 3" , len=1 -> OID=dsa-with-sha.
 *
 * UTCTime: VALUE must be a null terminated string in one of these
 *   formats: "YYMMDDhhmmssZ", "YYMMDDhhmmssZ",
 *   "YYMMDDhhmmss+hh'mm'", "YYMMDDhhmmss-hh'mm'",
 *   "YYMMDDhhmm+hh'mm'", or "YYMMDDhhmm-hh'mm'".  LEN != 0.
 *
 *            value="9801011200Z" , len=1 -> time=Jannuary 1st, 1998
 *            at 12h 00m Greenwich Mean Time
 *
 * GeneralizedTime: VALUE must be in one of this format:
 *   "YYYYMMDDhhmmss.sZ", "YYYYMMDDhhmmss.sZ",
 *   "YYYYMMDDhhmmss.s+hh'mm'", "YYYYMMDDhhmmss.s-hh'mm'",
 *   "YYYYMMDDhhmm+hh'mm'", or "YYYYMMDDhhmm-hh'mm'" where ss.s
 *   indicates the seconds with any precision like "10.1" or "01.02".
 *   LEN != 0
 *
 *            value="2001010112001.12-0700" , len=1 -> time=Jannuary
 *            1st, 2001 at 12h 00m 01.12s Pacific Daylight Time
 *
 * OCTET STRING: VALUE contains the octet string and LEN is the
 *   number of octets.
 *
 *            value="$\backslash$x01$\backslash$x02$\backslash$x03" ,
 *            len=3 -> three bytes octet string
 *
 * GeneralString: VALUE contains the generalstring and LEN is the
 *   number of octets.
 *
 *            value="$\backslash$x01$\backslash$x02$\backslash$x03" ,
 *            len=3 -> three bytes generalstring
 *
 * BIT STRING: VALUE contains the bit string organized by bytes and
 *   LEN is the number of bits.
 *
 *   value="$\backslash$xCF" , len=6 -> bit string="110011" (six
 *   bits)
 *
 * CHOICE: if NAME indicates a choice type, VALUE must specify one of
 *   the alternatives with a null terminated string. LEN != 0. Using
 *   "pkix.asn"\:
 *
 *           result=asn1_write_value(cert,
 *           "certificate1.tbsCertificate.subject", "rdnSequence",
 *           1);
 *
 * ANY: VALUE indicates the der encoding of a structure.  LEN != 0.
 *
 * SEQUENCE OF: VALUE must be the null terminated string "NEW" and
 *   LEN != 0. With this instruction another element is appended in
 *   the sequence. The name of this element will be "?1" if it's the
 *   first one, "?2" for the second and so on.
 *
 *   Using "pkix.asn"\:
 *
 *   result=asn1_write_value(cert,
 *   "certificate1.tbsCertificate.subject.rdnSequence", "NEW", 1);
 *
 * SET OF: the same as SEQUENCE OF.  Using "pkix.asn":
 *
 *           result=asn1_write_value(cert,
 *           "tbsCertificate.subject.rdnSequence.?LAST", "NEW", 1);
 *
 * Returns: %ASN1_SUCCESS if the value was set,
 *   %ASN1_ELEMENT_NOT_FOUND if @name is not a valid element, and
 *   %ASN1_VALUE_NOT_VALID if @ivalue has a wrong format.
 **/
int
asn1_write_value (asn1_node node_root, const char *name,
		  const void *ivalue, int len)
{
  asn1_node node, p, p2;
  unsigned char *temp, *value_temp = NULL, *default_temp = NULL;
  int len2, k, k2, negative;
  size_t i;
  const unsigned char *value = ivalue;
  unsigned int type;

  node = asn1_find_node (node_root, name);
  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  if ((node->type & CONST_OPTION) && (value == NULL) && (len == 0))
    {
      asn1_delete_structure (&node);
      return ASN1_SUCCESS;
    }

  type = type_field (node->type);

  if ((type == ASN1_ETYPE_SEQUENCE_OF || type == ASN1_ETYPE_SET_OF) && (value == NULL) && (len == 0))
    {
      p = node->down;
      while ((type_field (p->type) == ASN1_ETYPE_TAG)
	     || (type_field (p->type) == ASN1_ETYPE_SIZE))
	p = p->right;

      while (p->right)
	asn1_delete_structure (&p->right);

      return ASN1_SUCCESS;
    }

  /* Don't allow element deletion for other types */
  if (value == NULL)
    {
      return ASN1_VALUE_NOT_VALID;
    }

  switch (type)
    {
    case ASN1_ETYPE_BOOLEAN:
      if (!_asn1_strcmp (value, "TRUE"))
	{
	  if (node->type & CONST_DEFAULT)
	    {
	      p = node->down;
	      while (type_field (p->type) != ASN1_ETYPE_DEFAULT)
		p = p->right;
	      if (p->type & CONST_TRUE)
		_asn1_set_value (node, NULL, 0);
	      else
		_asn1_set_value (node, "T", 1);
	    }
	  else
	    _asn1_set_value (node, "T", 1);
	}
      else if (!_asn1_strcmp (value, "FALSE"))
	{
	  if (node->type & CONST_DEFAULT)
	    {
	      p = node->down;
	      while (type_field (p->type) != ASN1_ETYPE_DEFAULT)
		p = p->right;
	      if (p->type & CONST_FALSE)
		_asn1_set_value (node, NULL, 0);
	      else
		_asn1_set_value (node, "F", 1);
	    }
	  else
	    _asn1_set_value (node, "F", 1);
	}
      else
	return ASN1_VALUE_NOT_VALID;
      break;
    case ASN1_ETYPE_INTEGER:
    case ASN1_ETYPE_ENUMERATED:
      if (len == 0)
	{
	  if ((isdigit (value[0])) || (value[0] == '-'))
	    {
	      value_temp = malloc (SIZEOF_UNSIGNED_LONG_INT);
	      if (value_temp == NULL)
		return ASN1_MEM_ALLOC_ERROR;

	      _asn1_convert_integer (value, value_temp,
				     SIZEOF_UNSIGNED_LONG_INT, &len);
	    }
	  else
	    {			/* is an identifier like v1 */
	      if (!(node->type & CONST_LIST))
		return ASN1_VALUE_NOT_VALID;
	      p = node->down;
	      while (p)
		{
		  if (type_field (p->type) == ASN1_ETYPE_CONSTANT)
		    {
		      if (!_asn1_strcmp (p->name, value))
			{
			  value_temp = malloc (SIZEOF_UNSIGNED_LONG_INT);
			  if (value_temp == NULL)
			    return ASN1_MEM_ALLOC_ERROR;

			  _asn1_convert_integer (p->value,
						 value_temp,
						 SIZEOF_UNSIGNED_LONG_INT,
						 &len);
			  break;
			}
		    }
		  p = p->right;
		}
	      if (p == NULL)
		return ASN1_VALUE_NOT_VALID;
	    }
	}
      else
	{			/* len != 0 */
	  value_temp = malloc (len);
	  if (value_temp == NULL)
	    return ASN1_MEM_ALLOC_ERROR;
	  memcpy (value_temp, value, len);
	}

      if (value_temp[0] & 0x80)
	negative = 1;
      else
	negative = 0;

      if (negative && (type_field (node->type) == ASN1_ETYPE_ENUMERATED))
	{
	  free (value_temp);
	  return ASN1_VALUE_NOT_VALID;
	}

      for (k = 0; k < len - 1; k++)
	if (negative && (value_temp[k] != 0xFF))
	  break;
	else if (!negative && value_temp[k])
	  break;

      if ((negative && !(value_temp[k] & 0x80)) ||
	  (!negative && (value_temp[k] & 0x80)))
	k--;

      _asn1_set_value_lv (node, value_temp + k, len - k);

      if (node->type & CONST_DEFAULT)
	{
	  p = node->down;
	  while (type_field (p->type) != ASN1_ETYPE_DEFAULT)
	    p = p->right;
	  if ((isdigit (p->value[0])) || (p->value[0] == '-'))
	    {
	      default_temp = malloc (SIZEOF_UNSIGNED_LONG_INT);
	      if (default_temp == NULL)
		{
		  free (value_temp);
		  return ASN1_MEM_ALLOC_ERROR;
		}

	      _asn1_convert_integer (p->value, default_temp,
				     SIZEOF_UNSIGNED_LONG_INT, &len2);
	    }
	  else
	    {			/* is an identifier like v1 */
	      if (!(node->type & CONST_LIST))
		{
		  free (value_temp);
		  return ASN1_VALUE_NOT_VALID;
		}
	      p2 = node->down;
	      while (p2)
		{
		  if (type_field (p2->type) == ASN1_ETYPE_CONSTANT)
		    {
		      if (!_asn1_strcmp (p2->name, p->value))
			{
			  default_temp = malloc (SIZEOF_UNSIGNED_LONG_INT);
			  if (default_temp == NULL)
			    {
			      free (value_temp);
			      return ASN1_MEM_ALLOC_ERROR;
			    }

			  _asn1_convert_integer (p2->value,
						 default_temp,
						 SIZEOF_UNSIGNED_LONG_INT,
						 &len2);
			  break;
			}
		    }
		  p2 = p2->right;
		}
	      if (p2 == NULL)
		{
		  free (value_temp);
		  return ASN1_VALUE_NOT_VALID;
		}
	    }


	  if ((len - k) == len2)
	    {
	      for (k2 = 0; k2 < len2; k2++)
		if (value_temp[k + k2] != default_temp[k2])
		  {
		    break;
		  }
	      if (k2 == len2)
		_asn1_set_value (node, NULL, 0);
	    }
	  free (default_temp);
	}
      free (value_temp);
      break;
    case ASN1_ETYPE_OBJECT_ID:
      for (i = 0; i < _asn1_strlen (value); i++)
	if ((!isdigit (value[i])) && (value[i] != '.') && (value[i] != '+'))
	  return ASN1_VALUE_NOT_VALID;
      if (node->type & CONST_DEFAULT)
	{
	  p = node->down;
	  while (type_field (p->type) != ASN1_ETYPE_DEFAULT)
	    p = p->right;
	  if (!_asn1_strcmp (value, p->value))
	    {
	      _asn1_set_value (node, NULL, 0);
	      break;
	    }
	}
      _asn1_set_value (node, value, _asn1_strlen (value) + 1);
      break;
    case ASN1_ETYPE_UTC_TIME:
      {
	len = _asn1_strlen (value);
	if (len < 11)
	  return ASN1_VALUE_NOT_VALID;
	for (k = 0; k < 10; k++)
	  if (!isdigit (value[k]))
	    return ASN1_VALUE_NOT_VALID;
	switch (len)
	  {
	  case 11:
	    if (value[10] != 'Z')
	      return ASN1_VALUE_NOT_VALID;
	    break;
	  case 13:
	    if ((!isdigit (value[10])) || (!isdigit (value[11])) ||
		(value[12] != 'Z'))
	      return ASN1_VALUE_NOT_VALID;
	    break;
	  case 15:
	    if ((value[10] != '+') && (value[10] != '-'))
	      return ASN1_VALUE_NOT_VALID;
	    for (k = 11; k < 15; k++)
	      if (!isdigit (value[k]))
		return ASN1_VALUE_NOT_VALID;
	    break;
	  case 17:
	    if ((!isdigit (value[10])) || (!isdigit (value[11])))
	      return ASN1_VALUE_NOT_VALID;
	    if ((value[12] != '+') && (value[12] != '-'))
	      return ASN1_VALUE_NOT_VALID;
	    for (k = 13; k < 17; k++)
	      if (!isdigit (value[k]))
		return ASN1_VALUE_NOT_VALID;
	    break;
	  default:
	    return ASN1_VALUE_NOT_FOUND;
	  }
	_asn1_set_value (node, value, len);
      }
      break;
    case ASN1_ETYPE_GENERALIZED_TIME:
      len = _asn1_strlen (value);
      _asn1_set_value (node, value, len);
      break;
    case ASN1_ETYPE_OCTET_STRING:
    case ASN1_ETYPE_GENERALSTRING:
    case ASN1_ETYPE_NUMERIC_STRING:
    case ASN1_ETYPE_IA5_STRING:
    case ASN1_ETYPE_TELETEX_STRING:
    case ASN1_ETYPE_PRINTABLE_STRING:
    case ASN1_ETYPE_UNIVERSAL_STRING:
    case ASN1_ETYPE_BMP_STRING:
    case ASN1_ETYPE_UTF8_STRING:
    case ASN1_ETYPE_VISIBLE_STRING:
      if (len == 0)
	len = _asn1_strlen (value);
      _asn1_set_value_lv (node, value, len);
      break;
    case ASN1_ETYPE_BIT_STRING:
      if (len == 0)
	len = _asn1_strlen (value);
      asn1_length_der ((len >> 3) + 2, NULL, &len2);
      temp = malloc ((len >> 3) + 2 + len2);
      if (temp == NULL)
	return ASN1_MEM_ALLOC_ERROR;

      asn1_bit_der (value, len, temp, &len2);
      _asn1_set_value_m (node, temp, len2);
      temp = NULL;
      break;
    case ASN1_ETYPE_CHOICE:
      p = node->down;
      while (p)
	{
	  if (!_asn1_strcmp (p->name, value))
	    {
	      p2 = node->down;
	      while (p2)
		{
		  if (p2 != p)
		    {
		      asn1_delete_structure (&p2);
		      p2 = node->down;
		    }
		  else
		    p2 = p2->right;
		}
	      break;
	    }
	  p = p->right;
	}
      if (!p)
	return ASN1_ELEMENT_NOT_FOUND;
      break;
    case ASN1_ETYPE_ANY:
      _asn1_set_value_lv (node, value, len);
      break;
    case ASN1_ETYPE_SEQUENCE_OF:
    case ASN1_ETYPE_SET_OF:
      if (_asn1_strcmp (value, "NEW"))
	return ASN1_VALUE_NOT_VALID;
      _asn1_append_sequence_set (node, NULL);
      break;
    default:
      return ASN1_ELEMENT_NOT_FOUND;
      break;
    }

  return ASN1_SUCCESS;
}


#define PUT_VALUE( ptr, ptr_size, data, data_size) \
	*len = data_size; \
	if (ptr_size < data_size) { \
		return ASN1_MEM_ERROR; \
	} else { \
		if (ptr && data_size > 0) \
		  memcpy (ptr, data, data_size); \
	}

#define PUT_STR_VALUE( ptr, ptr_size, data) \
	*len = _asn1_strlen (data) + 1; \
	if (ptr_size < *len) { \
		return ASN1_MEM_ERROR; \
	} else { \
		/* this strcpy is checked */ \
		if (ptr) { \
		  _asn1_strcpy (ptr, data); \
		} \
	}

#define PUT_AS_STR_VALUE( ptr, ptr_size, data, data_size) \
	*len = data_size + 1; \
	if (ptr_size < *len) { \
		return ASN1_MEM_ERROR; \
	} else { \
		/* this strcpy is checked */ \
		if (ptr) { \
		  if (data_size > 0) \
		    memcpy (ptr, data, data_size); \
		  ptr[data_size] = 0; \
		} \
	}

#define ADD_STR_VALUE( ptr, ptr_size, data) \
        *len += _asn1_strlen(data); \
        if (ptr_size < (int) *len) { \
                (*len)++; \
                return ASN1_MEM_ERROR; \
        } else { \
                /* this strcat is checked */ \
                if (ptr) _asn1_strcat (ptr, data); \
        }

/**
 * asn1_read_value:
 * @root: pointer to a structure.
 * @name: the name of the element inside a structure that you want to read.
 * @ivalue: vector that will contain the element's content, must be a
 *   pointer to memory cells already allocated (may be %NULL).
 * @len: number of bytes of *value: value[0]..value[len-1]. Initialy
 *   holds the sizeof value.
 *
 * Returns the value of one element inside a structure. 
 * If an element is OPTIONAL and this returns
 * %ASN1_ELEMENT_NOT_FOUND, it means that this element wasn't present
 * in the der encoding that created the structure.  The first element
 * of a SEQUENCE_OF or SET_OF is named "?1". The second one "?2" and
 * so on. If the @root provided is a node to specific sequence element,
 * then the keyword "?CURRENT" is also acceptable and indicates the
 * current sequence element of this node.
 *
 * Note that there can be valid values with length zero. In these case
 * this function will succeed and @len will be zero.
 *
 * INTEGER: VALUE will contain a two's complement form integer.
 *
 *            integer=-1  -> value[0]=0xFF , len=1.
 *            integer=1   -> value[0]=0x01 , len=1.
 *
 * ENUMERATED: As INTEGER (but only with not negative numbers).
 *
 * BOOLEAN: VALUE will be the null terminated string "TRUE" or
 *   "FALSE" and LEN=5 or LEN=6.
 *
 * OBJECT IDENTIFIER: VALUE will be a null terminated string with
 *   each number separated by a dot (i.e. "1.2.3.543.1").
 *
 *                      LEN = strlen(VALUE)+1
 *
 * UTCTime: VALUE will be a null terminated string in one of these
 *   formats: "YYMMDDhhmmss+hh'mm'" or "YYMMDDhhmmss-hh'mm'".
 *   LEN=strlen(VALUE)+1.
 *
 * GeneralizedTime: VALUE will be a null terminated string in the
 *   same format used to set the value.
 *
 * OCTET STRING: VALUE will contain the octet string and LEN will be
 *   the number of octets.
 *
 * GeneralString: VALUE will contain the generalstring and LEN will
 *   be the number of octets.
 *
 * BIT STRING: VALUE will contain the bit string organized by bytes
 *   and LEN will be the number of bits.
 *
 * CHOICE: If NAME indicates a choice type, VALUE will specify the
 *   alternative selected.
 *
 * ANY: If NAME indicates an any type, VALUE will indicate the DER
 *   encoding of the structure actually used.
 *
 * Returns: %ASN1_SUCCESS if value is returned,
 *   %ASN1_ELEMENT_NOT_FOUND if @name is not a valid element,
 *   %ASN1_VALUE_NOT_FOUND if there isn't any value for the element
 *   selected, and %ASN1_MEM_ERROR if The value vector isn't big enough
 *   to store the result, and in this case @len will contain the number of
 *   bytes needed. On the occasion that the stored data are of zero-length
 *   this function may return %ASN1_SUCCESS even if the provided @len is zero.
 **/
int
asn1_read_value (asn1_node root, const char *name, void *ivalue, int *len)
{
  return asn1_read_value_type (root, name, ivalue, len, NULL);
}

/**
 * asn1_read_value_type:
 * @root: pointer to a structure.
 * @name: the name of the element inside a structure that you want to read.
 * @ivalue: vector that will contain the element's content, must be a
 *   pointer to memory cells already allocated (may be %NULL).
 * @len: number of bytes of *value: value[0]..value[len-1]. Initialy
 *   holds the sizeof value.
 * @etype: The type of the value read (ASN1_ETYPE)
 *
 * Returns the type and value of one element inside a structure. 
 * If an element is OPTIONAL and this returns
 * %ASN1_ELEMENT_NOT_FOUND, it means that this element wasn't present
 * in the der encoding that created the structure.  The first element
 * of a SEQUENCE_OF or SET_OF is named "?1". The second one "?2" and
 * so on. If the @root provided is a node to specific sequence element,
 * then the keyword "?CURRENT" is also acceptable and indicates the
 * current sequence element of this node.
 *
 * Note that there can be valid values with length zero. In these case
 * this function will succeed and @len will be zero.
 *
 *
 * INTEGER: VALUE will contain a two's complement form integer.
 *
 *            integer=-1  -> value[0]=0xFF , len=1.
 *            integer=1   -> value[0]=0x01 , len=1.
 *
 * ENUMERATED: As INTEGER (but only with not negative numbers).
 *
 * BOOLEAN: VALUE will be the null terminated string "TRUE" or
 *   "FALSE" and LEN=5 or LEN=6.
 *
 * OBJECT IDENTIFIER: VALUE will be a null terminated string with
 *   each number separated by a dot (i.e. "1.2.3.543.1").
 *
 *                      LEN = strlen(VALUE)+1
 *
 * UTCTime: VALUE will be a null terminated string in one of these
 *   formats: "YYMMDDhhmmss+hh'mm'" or "YYMMDDhhmmss-hh'mm'".
 *   LEN=strlen(VALUE)+1.
 *
 * GeneralizedTime: VALUE will be a null terminated string in the
 *   same format used to set the value.
 *
 * OCTET STRING: VALUE will contain the octet string and LEN will be
 *   the number of octets.
 *
 * GeneralString: VALUE will contain the generalstring and LEN will
 *   be the number of octets.
 *
 * BIT STRING: VALUE will contain the bit string organized by bytes
 *   and LEN will be the number of bits.
 *
 * CHOICE: If NAME indicates a choice type, VALUE will specify the
 *   alternative selected.
 *
 * ANY: If NAME indicates an any type, VALUE will indicate the DER
 *   encoding of the structure actually used.
 *
 * Returns: %ASN1_SUCCESS if value is returned,
 *   %ASN1_ELEMENT_NOT_FOUND if @name is not a valid element,
 *   %ASN1_VALUE_NOT_FOUND if there isn't any value for the element
 *   selected, and %ASN1_MEM_ERROR if The value vector isn't big enough
 *   to store the result, and in this case @len will contain the number of
 *   bytes needed. On the occasion that the stored data are of zero-length
 *   this function may return %ASN1_SUCCESS even if the provided @len is zero.
 **/
int
asn1_read_value_type (asn1_node root, const char *name, void *ivalue,
		      int *len, unsigned int *etype)
{
  asn1_node node, p, p2;
  int len2, len3, result;
  int value_size = *len;
  unsigned char *value = ivalue;
  unsigned type;

  node = asn1_find_node (root, name);
  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  type = type_field (node->type);

  if ((type != ASN1_ETYPE_NULL) &&
      (type != ASN1_ETYPE_CHOICE) &&
      !(node->type & CONST_DEFAULT) && !(node->type & CONST_ASSIGN) &&
      (node->value == NULL))
    return ASN1_VALUE_NOT_FOUND;

  if (etype)
    *etype = type;
  switch (type)
    {
    case ASN1_ETYPE_NULL:
      PUT_STR_VALUE (value, value_size, "NULL");
      break;
    case ASN1_ETYPE_BOOLEAN:
      if ((node->type & CONST_DEFAULT) && (node->value == NULL))
	{
	  p = node->down;
	  while (type_field (p->type) != ASN1_ETYPE_DEFAULT)
	    p = p->right;
	  if (p->type & CONST_TRUE)
	    {
	      PUT_STR_VALUE (value, value_size, "TRUE");
	    }
	  else
	    {
	      PUT_STR_VALUE (value, value_size, "FALSE");
	    }
	}
      else if (node->value[0] == 'T')
	{
	  PUT_STR_VALUE (value, value_size, "TRUE");
	}
      else
	{
	  PUT_STR_VALUE (value, value_size, "FALSE");
	}
      break;
    case ASN1_ETYPE_INTEGER:
    case ASN1_ETYPE_ENUMERATED:
      if ((node->type & CONST_DEFAULT) && (node->value == NULL))
	{
	  p = node->down;
	  while (type_field (p->type) != ASN1_ETYPE_DEFAULT)
	    p = p->right;
	  if ((isdigit (p->value[0])) || (p->value[0] == '-')
	      || (p->value[0] == '+'))
	    {
	      result = _asn1_convert_integer
		  (p->value, value, value_size, len);
              if (result != ASN1_SUCCESS)
		return result;
	    }
	  else
	    {			/* is an identifier like v1 */
	      p2 = node->down;
	      while (p2)
		{
		  if (type_field (p2->type) == ASN1_ETYPE_CONSTANT)
		    {
		      if (!_asn1_strcmp (p2->name, p->value))
			{
			  result = _asn1_convert_integer
			      (p2->value, value, value_size,
			       len);
			  if (result != ASN1_SUCCESS)
			    return result;
			  break;
			}
		    }
		  p2 = p2->right;
		}
	    }
	}
      else
	{
	  len2 = -1;
	  result = asn1_get_octet_der
	      (node->value, node->value_len, &len2, value, value_size,
	       len);
          if (result != ASN1_SUCCESS)
	    return result;
	}
      break;
    case ASN1_ETYPE_OBJECT_ID:
      if (node->type & CONST_ASSIGN)
	{
	  *len = 0;
	  if (value)
	    value[0] = 0;
	  p = node->down;
	  while (p)
	    {
	      if (type_field (p->type) == ASN1_ETYPE_CONSTANT)
		{
		  ADD_STR_VALUE (value, value_size, p->value);
		  if (p->right)
		    {
		      ADD_STR_VALUE (value, value_size, ".");
		    }
		}
	      p = p->right;
	    }
	  (*len)++;
	}
      else if ((node->type & CONST_DEFAULT) && (node->value == NULL))
	{
	  p = node->down;
	  while (type_field (p->type) != ASN1_ETYPE_DEFAULT)
	    p = p->right;
	  PUT_STR_VALUE (value, value_size, p->value);
	}
      else
	{
	  PUT_STR_VALUE (value, value_size, node->value);
	}
      break;
    case ASN1_ETYPE_GENERALIZED_TIME:
    case ASN1_ETYPE_UTC_TIME:
      PUT_AS_STR_VALUE (value, value_size, node->value, node->value_len);
      break;
    case ASN1_ETYPE_OCTET_STRING:
    case ASN1_ETYPE_GENERALSTRING:
    case ASN1_ETYPE_NUMERIC_STRING:
    case ASN1_ETYPE_IA5_STRING:
    case ASN1_ETYPE_TELETEX_STRING:
    case ASN1_ETYPE_PRINTABLE_STRING:
    case ASN1_ETYPE_UNIVERSAL_STRING:
    case ASN1_ETYPE_BMP_STRING:
    case ASN1_ETYPE_UTF8_STRING:
    case ASN1_ETYPE_VISIBLE_STRING:
      len2 = -1;
      result = asn1_get_octet_der
	  (node->value, node->value_len, &len2, value, value_size,
	   len);
      if (result != ASN1_SUCCESS)
	return result;
      break;
    case ASN1_ETYPE_BIT_STRING:
      len2 = -1;
      result = asn1_get_bit_der
	  (node->value, node->value_len, &len2, value, value_size,
	   len);
      if (result != ASN1_SUCCESS)
	return result;
      break;
    case ASN1_ETYPE_CHOICE:
      PUT_STR_VALUE (value, value_size, node->down->name);
      break;
    case ASN1_ETYPE_ANY:
      len3 = -1;
      len2 = asn1_get_length_der (node->value, node->value_len, &len3);
      if (len2 < 0)
	return ASN1_DER_ERROR;
      PUT_VALUE (value, value_size, node->value + len3, len2);
      break;
    default:
      return ASN1_ELEMENT_NOT_FOUND;
      break;
    }
  return ASN1_SUCCESS;
}


/**
 * asn1_read_tag:
 * @root: pointer to a structure
 * @name: the name of the element inside a structure.
 * @tagValue:  variable that will contain the TAG value.
 * @classValue: variable that will specify the TAG type.
 *
 * Returns the TAG and the CLASS of one element inside a structure.
 * CLASS can have one of these constants: %ASN1_CLASS_APPLICATION,
 * %ASN1_CLASS_UNIVERSAL, %ASN1_CLASS_PRIVATE or
 * %ASN1_CLASS_CONTEXT_SPECIFIC.
 *
 * Returns: %ASN1_SUCCESS if successful, %ASN1_ELEMENT_NOT_FOUND if
 *   @name is not a valid element.
 **/
int
asn1_read_tag (asn1_node root, const char *name, int *tagValue,
	       int *classValue)
{
  asn1_node node, p, pTag;

  node = asn1_find_node (root, name);
  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node->down;

  /* pTag will points to the IMPLICIT TAG */
  pTag = NULL;
  if (node->type & CONST_TAG)
    {
      while (p)
	{
	  if (type_field (p->type) == ASN1_ETYPE_TAG)
	    {
	      if ((p->type & CONST_IMPLICIT) && (pTag == NULL))
		pTag = p;
	      else if (p->type & CONST_EXPLICIT)
		pTag = NULL;
	    }
	  p = p->right;
	}
    }

  if (pTag)
    {
      *tagValue = _asn1_strtoul (pTag->value, NULL, 10);

      if (pTag->type & CONST_APPLICATION)
	*classValue = ASN1_CLASS_APPLICATION;
      else if (pTag->type & CONST_UNIVERSAL)
	*classValue = ASN1_CLASS_UNIVERSAL;
      else if (pTag->type & CONST_PRIVATE)
	*classValue = ASN1_CLASS_PRIVATE;
      else
	*classValue = ASN1_CLASS_CONTEXT_SPECIFIC;
    }
  else
    {
      unsigned type = type_field (node->type);
      *classValue = ASN1_CLASS_UNIVERSAL;

      switch (type)
	{
	CASE_HANDLED_ETYPES:
	  *tagValue = _asn1_tags[type].tag;
	  break;
	case ASN1_ETYPE_TAG:
	case ASN1_ETYPE_CHOICE:
	case ASN1_ETYPE_ANY:
	  *tagValue = -1;
	  break;
	default:
	  break;
	}
    }

  return ASN1_SUCCESS;
}

/**
 * asn1_read_node_value:
 * @node: pointer to a node.
 * @data: a point to a asn1_data_node_st
 *
 * Returns the value a data node inside a asn1_node structure.
 * The data returned should be handled as constant values.
 *
 * Returns: %ASN1_SUCCESS if the node exists.
 **/
int
asn1_read_node_value (asn1_node node, asn1_data_node_st * data)
{
  data->name = node->name;
  data->value = node->value;
  data->value_len = node->value_len;
  data->type = type_field (node->type);

  return ASN1_SUCCESS;
}
