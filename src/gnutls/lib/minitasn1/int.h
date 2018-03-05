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

#ifndef INT_H
#define INT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include <libtasn1.h>

#define ASN1_SMALL_VALUE_SIZE 16

/* This structure is also in libtasn1.h, but then contains less
   fields.  You cannot make any modifications to these first fields
   without breaking ABI.  */
struct asn1_node_st
{
  /* public fields: */
  char name[ASN1_MAX_NAME_SIZE + 1];	/* Node name */
  unsigned int name_hash;
  unsigned int type;		/* Node type */
  unsigned char *value;		/* Node value */
  int value_len;
  asn1_node down;		/* Pointer to the son node */
  asn1_node right;		/* Pointer to the brother node */
  asn1_node left;		/* Pointer to the next list element */
  /* private fields: */
  unsigned char small_value[ASN1_SMALL_VALUE_SIZE];	/* For small values */

  /* values used during decoding/coding */
  int tmp_ival;
  unsigned start; /* the start of the DER sequence - if decoded */
  unsigned end; /* the end of the DER sequence - if decoded */
};

typedef struct tag_and_class_st
{
  unsigned tag;
  unsigned class;
  const char *desc;
} tag_and_class_st;

/* the types that are handled in _asn1_tags */
#define CASE_HANDLED_ETYPES \
	case ASN1_ETYPE_NULL: \
	case ASN1_ETYPE_BOOLEAN: \
	case ASN1_ETYPE_INTEGER: \
	case ASN1_ETYPE_ENUMERATED: \
	case ASN1_ETYPE_OBJECT_ID: \
	case ASN1_ETYPE_OCTET_STRING: \
	case ASN1_ETYPE_GENERALSTRING: \
        case ASN1_ETYPE_NUMERIC_STRING: \
        case ASN1_ETYPE_IA5_STRING: \
        case ASN1_ETYPE_TELETEX_STRING: \
        case ASN1_ETYPE_PRINTABLE_STRING: \
        case ASN1_ETYPE_UNIVERSAL_STRING: \
        case ASN1_ETYPE_BMP_STRING: \
        case ASN1_ETYPE_UTF8_STRING: \
        case ASN1_ETYPE_VISIBLE_STRING: \
	case ASN1_ETYPE_BIT_STRING: \
	case ASN1_ETYPE_SEQUENCE: \
	case ASN1_ETYPE_SEQUENCE_OF: \
	case ASN1_ETYPE_SET: \
	case ASN1_ETYPE_UTC_TIME: \
	case ASN1_ETYPE_GENERALIZED_TIME: \
	case ASN1_ETYPE_SET_OF

#define ETYPE_TAG(etype) (_asn1_tags[etype].tag)
#define ETYPE_CLASS(etype) (_asn1_tags[etype].class)
#define ETYPE_OK(etype) (((etype) != ASN1_ETYPE_INVALID && \
                          (etype) <= _asn1_tags_size && \
                          _asn1_tags[(etype)].desc != NULL)?1:0)

#define ETYPE_IS_STRING(etype) ((etype == ASN1_ETYPE_GENERALSTRING || \
	etype == ASN1_ETYPE_NUMERIC_STRING || etype == ASN1_ETYPE_IA5_STRING || \
	etype == ASN1_ETYPE_TELETEX_STRING || etype == ASN1_ETYPE_PRINTABLE_STRING || \
	etype == ASN1_ETYPE_UNIVERSAL_STRING || etype == ASN1_ETYPE_BMP_STRING || \
	etype == ASN1_ETYPE_UTF8_STRING || etype == ASN1_ETYPE_VISIBLE_STRING || \
	etype == ASN1_ETYPE_OCTET_STRING)?1:0)

extern unsigned int _asn1_tags_size;
extern const tag_and_class_st _asn1_tags[];

#define _asn1_strlen(s) strlen((const char *) s)
#define _asn1_strtol(n,e,b) strtol((const char *) n, e, b)
#define _asn1_strtoul(n,e,b) strtoul((const char *) n, e, b)
#define _asn1_strcmp(a,b) strcmp((const char *)a, (const char *)b)
#define _asn1_strcpy(a,b) strcpy((char *)a, (const char *)b)
#define _asn1_strcat(a,b) strcat((char *)a, (const char *)b)

#if SIZEOF_UNSIGNED_LONG_INT == 8
# define _asn1_strtou64(n,e,b) strtoul((const char *) n, e, b)
#else
# define _asn1_strtou64(n,e,b) strtoull((const char *) n, e, b)
#endif

#define MAX_LOG_SIZE 1024	/* maximum number of characters of a log message */

/* Define used for visiting trees. */
#define UP     1
#define RIGHT  2
#define DOWN   3

/***********************************************************************/
/* List of constants to better specify the type of typedef asn1_node_st.   */
/***********************************************************************/
/*  Used with TYPE_TAG  */
#define CONST_UNIVERSAL   (1<<8)
#define CONST_PRIVATE     (1<<9)
#define CONST_APPLICATION (1<<10)
#define CONST_EXPLICIT    (1<<11)
#define CONST_IMPLICIT    (1<<12)

#define CONST_TAG         (1<<13)	/*  Used in ASN.1 assignement  */
#define CONST_OPTION      (1<<14)
#define CONST_DEFAULT     (1<<15)
#define CONST_TRUE        (1<<16)
#define CONST_FALSE       (1<<17)

#define CONST_LIST        (1<<18)	/*  Used with TYPE_INTEGER and TYPE_BIT_STRING  */
#define CONST_MIN_MAX     (1<<19)

#define CONST_1_PARAM     (1<<20)

#define CONST_SIZE        (1<<21)

#define CONST_DEFINED_BY  (1<<22)

/* Those two are deprecated and used for backwards compatibility */
#define CONST_GENERALIZED (1<<23)
#define CONST_UTC         (1<<24)

/* #define CONST_IMPORTS     (1<<25) */

#define CONST_NOT_USED    (1<<26)
#define CONST_SET         (1<<27)
#define CONST_ASSIGN      (1<<28)

#define CONST_DOWN        (1<<29)
#define CONST_RIGHT       (1<<30)


#define ASN1_ETYPE_TIME 17
/****************************************/
/* Returns the first 8 bits.            */
/* Used with the field type of asn1_node_st */
/****************************************/
inline static unsigned int
type_field (unsigned int ntype)
{
  return (ntype & 0xff);
}

/* To convert old types from a static structure */
inline static unsigned int
convert_old_type (unsigned int ntype)
{
  unsigned int type = ntype & 0xff;
  if (type == ASN1_ETYPE_TIME)
    {
      if (ntype & CONST_UTC)
	type = ASN1_ETYPE_UTC_TIME;
      else
	type = ASN1_ETYPE_GENERALIZED_TIME;

      ntype &= ~(CONST_UTC | CONST_GENERALIZED);
      ntype &= 0xffffff00;
      ntype |= type;

      return ntype;
    }
  else
    return ntype;
}

static inline
void *_asn1_realloc(void *ptr, size_t size)
{
  void *ret;

  if (size == 0)
    return ptr;

  ret = realloc(ptr, size);
  if (ret == NULL)
    {
      free(ptr);
    }
  return ret;
}

#endif /* INT_H */
