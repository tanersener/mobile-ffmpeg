/*
 * Copyright (C) 2002-2016 Free Software Foundation, Inc.
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
/* File: decoding.c                                  */
/* Description: Functions to manage DER decoding     */
/*****************************************************/

#include <int.h>
#include <parser_aux.h>
#include <gstr.h>
#include <structure.h>
#include <element.h>
#include <limits.h>
#include <intprops.h>

#ifdef DEBUG
# define warn() fprintf(stderr, "%s: %d\n", __func__, __LINE__)
#else
# define warn()
#endif

#define IS_ERR(len, flags) (len < -1 || ((flags & ASN1_DECODE_FLAG_STRICT_DER) && len < 0))

#define HAVE_TWO(x) (x>=2?1:0)

#define DECODE_FLAG_HAVE_TAG 1
#define DECODE_FLAG_INDEFINITE (1<<1)
/* On indefinite string decoding, allow this maximum levels
 * of recursion. Allowing infinite recursion, makes the BER
 * decoder susceptible to stack exhaustion due to that recursion.
 */
#define DECODE_FLAG_LEVEL1 (1<<2)
#define DECODE_FLAG_LEVEL2 (1<<3)
#define DECODE_FLAG_LEVEL3 (1<<4)

#define DECR_LEN(l, s) do { \
	  l -= s; \
	  if (l < 0) { \
	    warn(); \
	    result = ASN1_DER_ERROR; \
	    goto cleanup; \
	  } \
	} while (0)

static int
_asn1_get_indefinite_length_string (const unsigned char *der, int der_len, int *len);

static int
_asn1_decode_simple_ber (unsigned int etype, const unsigned char *der,
			unsigned int _der_len, unsigned char **str,
			unsigned int *str_len, unsigned int *ber_len,
			unsigned dflags);

static int
_asn1_decode_simple_der (unsigned int etype, const unsigned char *der,
			unsigned int _der_len, const unsigned char **str,
			unsigned int *str_len, unsigned dflags);

static void
_asn1_error_description_tag_error (asn1_node node, char *ErrorDescription)
{

  Estrcpy (ErrorDescription, ":: tag error near element '");
  _asn1_hierarchical_name (node, ErrorDescription + strlen (ErrorDescription),
			   ASN1_MAX_ERROR_DESCRIPTION_SIZE - 40);
  Estrcat (ErrorDescription, "'");

}

/**
 * asn1_get_length_der:
 * @der: DER data to decode.
 * @der_len: Length of DER data to decode.
 * @len: Output variable containing the length of the DER length field.
 *
 * Extract a length field from DER data.
 *
 * Returns: Return the decoded length value, or -1 on indefinite
 *   length, or -2 when the value was too big to fit in a int, or -4
 *   when the decoded length value plus @len would exceed @der_len.
 **/
long
asn1_get_length_der (const unsigned char *der, int der_len, int *len)
{
  unsigned int ans;
  int k, punt, sum;

  *len = 0;
  if (der_len <= 0)
    return 0;

  if (!(der[0] & 128))
    {
      /* short form */
      *len = 1;
      ans = der[0];
    }
  else
    {
      /* Long form */
      k = der[0] & 0x7F;
      punt = 1;
      if (k)
	{ /* definite length method */
	  ans = 0;
	  while (punt <= k && punt < der_len)
	    {
	      if (INT_MULTIPLY_OVERFLOW (ans, 256))
		return -2;
	      ans *= 256;

	      if (INT_ADD_OVERFLOW (ans, ((unsigned) der[punt])))
		return -2;
	      ans += der[punt];
	      punt++;
	    }
	}
      else
	{			/* indefinite length method */
	  *len = punt;
	  return -1;
	}

      *len = punt;
    }

  sum = ans;
  if (ans >= INT_MAX || INT_ADD_OVERFLOW (sum, (*len)))
    return -2;
  sum += *len;

  if (sum > der_len)
    return -4;

  return ans;
}

/**
 * asn1_get_tag_der:
 * @der: DER data to decode.
 * @der_len: Length of DER data to decode.
 * @cls: Output variable containing decoded class.
 * @len: Output variable containing the length of the DER TAG data.
 * @tag: Output variable containing the decoded tag (may be %NULL).
 *
 * Decode the class and TAG from DER code.
 *
 * Returns: Returns %ASN1_SUCCESS on success, or an error.
 **/
int
asn1_get_tag_der (const unsigned char *der, int der_len,
		  unsigned char *cls, int *len, unsigned long *tag)
{
  unsigned int ris;
  int punt;

  if (der == NULL || der_len < 2 || len == NULL)
    return ASN1_DER_ERROR;

  *cls = der[0] & 0xE0;
  if ((der[0] & 0x1F) != 0x1F)
    {
      /* short form */
      *len = 1;
      ris = der[0] & 0x1F;
    }
  else
    {
      /* Long form */
      punt = 1;
      ris = 0;
      while (punt < der_len && der[punt] & 128)
	{

	  if (INT_MULTIPLY_OVERFLOW (ris, 128))
	    return ASN1_DER_ERROR;
	  ris *= 128;

	  if (INT_ADD_OVERFLOW (ris, ((unsigned) (der[punt] & 0x7F))))
	    return ASN1_DER_ERROR;
	  ris += (der[punt] & 0x7F);
	  punt++;
	}

      if (punt >= der_len)
	return ASN1_DER_ERROR;

      if (INT_MULTIPLY_OVERFLOW (ris, 128))
	return ASN1_DER_ERROR;
      ris *= 128;

      if (INT_ADD_OVERFLOW (ris, ((unsigned) (der[punt] & 0x7F))))
	return ASN1_DER_ERROR;
      ris += (der[punt] & 0x7F);
      punt++;

      *len = punt;
    }

  if (tag)
    *tag = ris;
  return ASN1_SUCCESS;
}

/**
 * asn1_get_length_ber:
 * @ber: BER data to decode.
 * @ber_len: Length of BER data to decode.
 * @len: Output variable containing the length of the BER length field.
 *
 * Extract a length field from BER data.  The difference to
 * asn1_get_length_der() is that this function will return a length
 * even if the value has indefinite encoding.
 *
 * Returns: Return the decoded length value, or negative value when
 *   the value was too big.
 *
 * Since: 2.0
 **/
long
asn1_get_length_ber (const unsigned char *ber, int ber_len, int *len)
{
  int ret;
  long err;

  ret = asn1_get_length_der (ber, ber_len, len);
  if (ret == -1 && ber_len > 1)
    {				/* indefinite length method */
      err = _asn1_get_indefinite_length_string (ber + 1, ber_len-1, &ret);
      if (err != ASN1_SUCCESS)
	return -3;
    }

  return ret;
}

/**
 * asn1_get_octet_der:
 * @der: DER data to decode containing the OCTET SEQUENCE.
 * @der_len: The length of the @der data to decode.
 * @ret_len: Output variable containing the encoded length of the DER data.
 * @str: Pre-allocated output buffer to put decoded OCTET SEQUENCE in.
 * @str_size: Length of pre-allocated output buffer.
 * @str_len: Output variable containing the length of the contents of the OCTET SEQUENCE.
 *
 * Extract an OCTET SEQUENCE from DER data. Note that this function
 * expects the DER data past the tag field, i.e., the length and
 * content octets.
 *
 * Returns: Returns %ASN1_SUCCESS on success, or an error.
 **/
int
asn1_get_octet_der (const unsigned char *der, int der_len,
		    int *ret_len, unsigned char *str, int str_size,
		    int *str_len)
{
  int len_len = 0;

  if (der_len <= 0)
    return ASN1_GENERIC_ERROR;

  *str_len = asn1_get_length_der (der, der_len, &len_len);

  if (*str_len < 0)
    return ASN1_DER_ERROR;

  *ret_len = *str_len + len_len;
  if (str_size >= *str_len)
    {
      if (*str_len > 0 && str != NULL)
        memcpy (str, der + len_len, *str_len);
    }
  else
    {
      return ASN1_MEM_ERROR;
    }

  return ASN1_SUCCESS;
}


/*- 
 * _asn1_get_time_der:
 * @type: %ASN1_ETYPE_GENERALIZED_TIME or %ASN1_ETYPE_UTC_TIME
 * @der: DER data to decode containing the time
 * @der_len: Length of DER data to decode.
 * @ret_len: Output variable containing the length of the DER data.
 * @str: Pre-allocated output buffer to put the textual time in.
 * @str_size: Length of pre-allocated output buffer.
 * @flags: Zero or %ASN1_DECODE_FLAG_STRICT_DER
 *
 * Performs basic checks in the DER encoded time object and returns its textual form.
 * The textual form will be in the YYYYMMDD000000Z format for GeneralizedTime
 * and YYMMDD000000Z for UTCTime.
 *
 * Returns: %ASN1_SUCCESS on success, or an error.
 -*/
static int
_asn1_get_time_der (unsigned type, const unsigned char *der, int der_len, int *ret_len,
		    char *str, int str_size, unsigned flags)
{
  int len_len, str_len;
  unsigned i;
  unsigned sign_count = 0;
  unsigned dot_count = 0;
  const unsigned char *p;

  if (der_len <= 0 || str == NULL)
    return ASN1_DER_ERROR;

  str_len = asn1_get_length_der (der, der_len, &len_len);
  if (str_len <= 0 || str_size < str_len)
    return ASN1_DER_ERROR;

  /* perform some sanity checks on the data */
  if (str_len < 8)
    {
      warn();
      return ASN1_TIME_ENCODING_ERROR;
    }

  if ((flags & ASN1_DECODE_FLAG_STRICT_DER) && !(flags & ASN1_DECODE_FLAG_ALLOW_INCORRECT_TIME))
    {
      p = &der[len_len];
      for (i=0;i<(unsigned)(str_len-1);i++)
         {
           if (isdigit(p[i]) == 0)
             {
               if (type == ASN1_ETYPE_GENERALIZED_TIME)
                 {
                   /* tolerate lax encodings */
                   if (p[i] == '.' && dot_count == 0)
                     {
                       dot_count++;
                       continue;
                     }

               /* This is not really valid DER, but there are
                * structures using that */
                   if (!(flags & ASN1_DECODE_FLAG_STRICT_DER) &&
                       (p[i] == '+' || p[i] == '-') && sign_count == 0)
                     {
                       sign_count++;
                       continue;
                     }
                 }

               warn();
               return ASN1_TIME_ENCODING_ERROR;
             }
         }

      if (sign_count == 0 && p[str_len-1] != 'Z')
        {
          warn();
          return ASN1_TIME_ENCODING_ERROR;
        }
    }
  memcpy (str, der + len_len, str_len);
  str[str_len] = 0;
  *ret_len = str_len + len_len;

  return ASN1_SUCCESS;
}

/**
 * asn1_get_objectid_der:
 * @der: DER data to decode containing the OBJECT IDENTIFIER
 * @der_len: Length of DER data to decode.
 * @ret_len: Output variable containing the length of the DER data.
 * @str: Pre-allocated output buffer to put the textual object id in.
 * @str_size: Length of pre-allocated output buffer.
 *
 * Converts a DER encoded object identifier to its textual form. This
 * function expects the DER object identifier without the tag.
 *
 * Returns: %ASN1_SUCCESS on success, or an error.
 **/
int
asn1_get_object_id_der (const unsigned char *der, int der_len, int *ret_len,
			char *str, int str_size)
{
  int len_len, len, k;
  int leading;
  char temp[LTOSTR_MAX_SIZE];
  uint64_t val, val1;

  *ret_len = 0;
  if (str && str_size > 0)
    str[0] = 0;			/* no oid */

  if (str == NULL || der_len <= 0)
    return ASN1_GENERIC_ERROR;

  len = asn1_get_length_der (der, der_len, &len_len);

  if (len <= 0 || len + len_len > der_len)
    return ASN1_DER_ERROR;

  val1 = der[len_len] / 40;
  val = der[len_len] - val1 * 40;

  _asn1_str_cpy (str, str_size, _asn1_ltostr (val1, temp));
  _asn1_str_cat (str, str_size, ".");
  _asn1_str_cat (str, str_size, _asn1_ltostr (val, temp));

  val = 0;
  leading = 1;
  for (k = 1; k < len; k++)
    {
      /* X.690 mandates that the leading byte must never be 0x80
       */
      if (leading != 0 && der[len_len + k] == 0x80)
	return ASN1_DER_ERROR;
      leading = 0;

      /* check for wrap around */
      if (INT_LEFT_SHIFT_OVERFLOW (val, 7))
	return ASN1_DER_ERROR;

      val = val << 7;
      val |= der[len_len + k] & 0x7F;

      if (!(der[len_len + k] & 0x80))
	{
	  _asn1_str_cat (str, str_size, ".");
	  _asn1_str_cat (str, str_size, _asn1_ltostr (val, temp));
	  val = 0;
	  leading = 1;
	}
    }

  if (INT_ADD_OVERFLOW (len, len_len))
    return ASN1_DER_ERROR;

  *ret_len = len + len_len;

  return ASN1_SUCCESS;
}

/**
 * asn1_get_bit_der:
 * @der: DER data to decode containing the BIT SEQUENCE.
 * @der_len: Length of DER data to decode.
 * @ret_len: Output variable containing the length of the DER data.
 * @str: Pre-allocated output buffer to put decoded BIT SEQUENCE in.
 * @str_size: Length of pre-allocated output buffer.
 * @bit_len: Output variable containing the size of the BIT SEQUENCE.
 *
 * Extract a BIT SEQUENCE from DER data.
 *
 * Returns: %ASN1_SUCCESS on success, or an error.
 **/
int
asn1_get_bit_der (const unsigned char *der, int der_len,
		  int *ret_len, unsigned char *str, int str_size,
		  int *bit_len)
{
  int len_len = 0, len_byte;

  if (der_len <= 0)
    return ASN1_GENERIC_ERROR;

  len_byte = asn1_get_length_der (der, der_len, &len_len) - 1;
  if (len_byte < 0)
    return ASN1_DER_ERROR;

  *ret_len = len_byte + len_len + 1;
  *bit_len = len_byte * 8 - der[len_len];

  if (*bit_len < 0)
    return ASN1_DER_ERROR;

  if (str_size >= len_byte)
    {
      if (len_byte > 0 && str)
        memcpy (str, der + len_len + 1, len_byte);
    }
  else
    {
      return ASN1_MEM_ERROR;
    }

  return ASN1_SUCCESS;
}

/* tag_len: the total tag length (explicit+inner)
 * inner_tag_len: the inner_tag length
 */
static int
_asn1_extract_tag_der (asn1_node node, const unsigned char *der, int der_len,
		       int *tag_len, int *inner_tag_len, unsigned flags)
{
  asn1_node p;
  int counter, len2, len3, is_tag_implicit;
  int result;
  unsigned long tag, tag_implicit = 0;
  unsigned char class, class2, class_implicit = 0;

  if (der_len <= 0)
    return ASN1_GENERIC_ERROR;

  counter = is_tag_implicit = 0;

  if (node->type & CONST_TAG)
    {
      p = node->down;
      while (p)
	{
	  if (type_field (p->type) == ASN1_ETYPE_TAG)
	    {
	      if (p->type & CONST_APPLICATION)
		class2 = ASN1_CLASS_APPLICATION;
	      else if (p->type & CONST_UNIVERSAL)
		class2 = ASN1_CLASS_UNIVERSAL;
	      else if (p->type & CONST_PRIVATE)
		class2 = ASN1_CLASS_PRIVATE;
	      else
		class2 = ASN1_CLASS_CONTEXT_SPECIFIC;

	      if (p->type & CONST_EXPLICIT)
		{
		  if (asn1_get_tag_der
		      (der + counter, der_len, &class, &len2,
		       &tag) != ASN1_SUCCESS)
		    return ASN1_DER_ERROR;

                  DECR_LEN(der_len, len2);
		  counter += len2;

		  if (flags & ASN1_DECODE_FLAG_STRICT_DER)
		    len3 =
		      asn1_get_length_der (der + counter, der_len,
					 &len2);
		  else
		    len3 =
		      asn1_get_length_ber (der + counter, der_len,
					 &len2);
		  if (len3 < 0)
		    return ASN1_DER_ERROR;

                  DECR_LEN(der_len, len2);
		  counter += len2;

		  if (!is_tag_implicit)
		    {
		      if ((class != (class2 | ASN1_CLASS_STRUCTURED)) ||
			  (tag != strtoul ((char *) p->value, NULL, 10)))
			return ASN1_TAG_ERROR;
		    }
		  else
		    {		/* ASN1_TAG_IMPLICIT */
		      if ((class != class_implicit) || (tag != tag_implicit))
			return ASN1_TAG_ERROR;
		    }
		  is_tag_implicit = 0;
		}
	      else
		{		/* ASN1_TAG_IMPLICIT */
		  if (!is_tag_implicit)
		    {
		      if ((type_field (node->type) == ASN1_ETYPE_SEQUENCE) ||
			  (type_field (node->type) == ASN1_ETYPE_SEQUENCE_OF)
			  || (type_field (node->type) == ASN1_ETYPE_SET)
			  || (type_field (node->type) == ASN1_ETYPE_SET_OF))
			class2 |= ASN1_CLASS_STRUCTURED;
		      class_implicit = class2;
		      tag_implicit = strtoul ((char *) p->value, NULL, 10);
		      is_tag_implicit = 1;
		    }
		}
	    }
	  p = p->right;
	}
    }

  if (is_tag_implicit)
    {
      if (asn1_get_tag_der
	  (der + counter, der_len, &class, &len2,
	   &tag) != ASN1_SUCCESS)
	return ASN1_DER_ERROR;

      DECR_LEN(der_len, len2);

      if ((class != class_implicit) || (tag != tag_implicit))
	{
	  if (type_field (node->type) == ASN1_ETYPE_OCTET_STRING)
	    {
	      class_implicit |= ASN1_CLASS_STRUCTURED;
	      if ((class != class_implicit) || (tag != tag_implicit))
		return ASN1_TAG_ERROR;
	    }
	  else
	    return ASN1_TAG_ERROR;
	}
    }
  else
    {
      unsigned type = type_field (node->type);
      if (type == ASN1_ETYPE_TAG)
	{
	  *tag_len = 0;
	  if (inner_tag_len)
	    *inner_tag_len = 0;
	  return ASN1_SUCCESS;
	}

      if (asn1_get_tag_der
	  (der + counter, der_len, &class, &len2,
	   &tag) != ASN1_SUCCESS)
	return ASN1_DER_ERROR;

      DECR_LEN(der_len, len2);

      switch (type)
	{
	case ASN1_ETYPE_NULL:
	case ASN1_ETYPE_BOOLEAN:
	case ASN1_ETYPE_INTEGER:
	case ASN1_ETYPE_ENUMERATED:
	case ASN1_ETYPE_OBJECT_ID:
	case ASN1_ETYPE_GENERALSTRING:
	case ASN1_ETYPE_NUMERIC_STRING:
	case ASN1_ETYPE_IA5_STRING:
	case ASN1_ETYPE_TELETEX_STRING:
	case ASN1_ETYPE_PRINTABLE_STRING:
	case ASN1_ETYPE_UNIVERSAL_STRING:
	case ASN1_ETYPE_BMP_STRING:
	case ASN1_ETYPE_UTF8_STRING:
	case ASN1_ETYPE_VISIBLE_STRING:
	case ASN1_ETYPE_BIT_STRING:
	case ASN1_ETYPE_SEQUENCE:
	case ASN1_ETYPE_SEQUENCE_OF:
	case ASN1_ETYPE_SET:
	case ASN1_ETYPE_SET_OF:
	case ASN1_ETYPE_GENERALIZED_TIME:
	case ASN1_ETYPE_UTC_TIME:
	  if ((class != _asn1_tags[type].class)
	      || (tag != _asn1_tags[type].tag))
	    return ASN1_DER_ERROR;
	  break;

	case ASN1_ETYPE_OCTET_STRING:
	  /* OCTET STRING is handled differently to allow
	   * BER encodings (structured class). */
	  if (((class != ASN1_CLASS_UNIVERSAL)
	       && (class != (ASN1_CLASS_UNIVERSAL | ASN1_CLASS_STRUCTURED)))
	      || (tag != ASN1_TAG_OCTET_STRING))
	    return ASN1_DER_ERROR;
	  break;
	case ASN1_ETYPE_ANY:
	  counter -= len2;
	  break;
	case ASN1_ETYPE_CHOICE:
	  counter -= len2;
	  break;
	default:
	  return ASN1_DER_ERROR;
	  break;
	}
    }

  counter += len2;
  *tag_len = counter;
  if (inner_tag_len)
    *inner_tag_len = len2;
  return ASN1_SUCCESS;

cleanup:
  return result;
}

static int
extract_tag_der_recursive(asn1_node node, const unsigned char *der, int der_len,
		       int *ret_len, int *inner_len, unsigned flags)
{
asn1_node p;
int ris = ASN1_DER_ERROR;

  if (type_field (node->type) == ASN1_ETYPE_CHOICE)
    {
      p = node->down;
      while (p)
        {
          ris = _asn1_extract_tag_der (p, der, der_len, ret_len, inner_len, flags);
          if (ris == ASN1_SUCCESS)
            break;
          p = p->right;
	}

      *ret_len = 0;
      return ris;
    }
  else
    return _asn1_extract_tag_der (node, der, der_len, ret_len, inner_len, flags);
}

static int
_asn1_delete_not_used (asn1_node node)
{
  asn1_node p, p2;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node;
  while (p)
    {
      if (p->type & CONST_NOT_USED)
	{
	  p2 = NULL;
	  if (p != node)
	    {
	      p2 = _asn1_find_left (p);
	      if (!p2)
		p2 = _asn1_find_up (p);
	    }
	  asn1_delete_structure (&p);
	  p = p2;
	}

      if (!p)
	break;			/* reach node */

      if (p->down)
	{
	  p = p->down;
	}
      else
	{
	  if (p == node)
	    p = NULL;
	  else if (p->right)
	    p = p->right;
	  else
	    {
	      while (1)
		{
		  p = _asn1_find_up (p);
		  if (p == node)
		    {
		      p = NULL;
		      break;
		    }
		  if (p->right)
		    {
		      p = p->right;
		      break;
		    }
		}
	    }
	}
    }
  return ASN1_SUCCESS;
}

static int
_asn1_get_indefinite_length_string (const unsigned char *der,
				    int der_len, int *len)
{
  int len2, len3, counter, indefinite;
  int result;
  unsigned long tag;
  unsigned char class;

  counter = indefinite = 0;

  while (1)
    {
      if (HAVE_TWO(der_len) && (der[counter] == 0) && (der[counter + 1] == 0))
	{
	  counter += 2;
	  DECR_LEN(der_len, 2);

	  indefinite--;
	  if (indefinite <= 0)
	    break;
	  else
	    continue;
	}

      if (asn1_get_tag_der
	  (der + counter, der_len, &class, &len2,
	   &tag) != ASN1_SUCCESS)
	return ASN1_DER_ERROR;

      DECR_LEN(der_len, len2);
      counter += len2;

      len2 = asn1_get_length_der (der + counter, der_len, &len3);
      if (len2 < -1)
	return ASN1_DER_ERROR;

      if (len2 == -1)
	{
	  indefinite++;
	  counter += 1;
          DECR_LEN(der_len, 1);
	}
      else
	{
	  counter += len2 + len3;
          DECR_LEN(der_len, len2+len3);
	}
    }

  *len = counter;
  return ASN1_SUCCESS;

cleanup:
  return result;
}

static void delete_unneeded_choice_fields(asn1_node p)
{
  asn1_node p2;

  while (p->right)
    {
      p2 = p->right;
      asn1_delete_structure (&p2);
    }
}


/**
 * asn1_der_decoding2
 * @element: pointer to an ASN1 structure.
 * @ider: vector that contains the DER encoding.
 * @max_ider_len: pointer to an integer giving the information about the
 *   maximal number of bytes occupied by *@ider. The real size of the DER
 *   encoding is returned through this pointer.
 * @flags: flags controlling the behaviour of the function.
 * @errorDescription: null-terminated string contains details when an
 *   error occurred.
 *
 * Fill the structure *@element with values of a DER encoding string. The
 * structure must just be created with function asn1_create_element().
 *
 * If %ASN1_DECODE_FLAG_ALLOW_PADDING flag is set then the function will ignore
 * padding after the decoded DER data. Upon a successful return the value of
 * *@max_ider_len will be set to the number of bytes decoded.
 *
 * If %ASN1_DECODE_FLAG_STRICT_DER flag is set then the function will
 * not decode any BER-encoded elements.
 *
 * Returns: %ASN1_SUCCESS if DER encoding OK, %ASN1_ELEMENT_NOT_FOUND
 *   if @ELEMENT is %NULL, and %ASN1_TAG_ERROR or
 *   %ASN1_DER_ERROR if the der encoding doesn't match the structure
 *   name (*@ELEMENT deleted).
 **/
int
asn1_der_decoding2 (asn1_node *element, const void *ider, int *max_ider_len,
		    unsigned int flags, char *errorDescription)
{
  asn1_node node, p, p2, p3;
  char temp[128];
  int counter, len2, len3, len4, move, ris, tlen;
  struct node_tail_cache_st tcache = {NULL, NULL};
  unsigned char class;
  unsigned long tag;
  int tag_len;
  int indefinite, result, total_len = *max_ider_len, ider_len = *max_ider_len;
  int inner_tag_len;
  unsigned char *ptmp;
  const unsigned char *ptag;
  const unsigned char *der = ider;

  node = *element;

  if (errorDescription != NULL)
    errorDescription[0] = 0;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  if (node->type & CONST_OPTION)
    {
      result = ASN1_GENERIC_ERROR;
      warn();
      goto cleanup;
    }

  counter = 0;
  move = DOWN;
  p = node;
  while (1)
    {
      tag_len = 0;
      inner_tag_len = 0;
      ris = ASN1_SUCCESS;
      if (move != UP)
	{
	  if (p->type & CONST_SET)
	    {
	      p2 = _asn1_find_up (p);
	      len2 = p2->tmp_ival;
	      if (len2 == -1)
		{
		  if (HAVE_TWO(ider_len) && !der[counter] && !der[counter + 1])
		    {
		      p = p2;
		      move = UP;
		      counter += 2;
		      DECR_LEN(ider_len, 2);
		      continue;
		    }
		}
	      else if (counter == len2)
		{
		  p = p2;
		  move = UP;
		  continue;
		}
	      else if (counter > len2)
		{
		  result = ASN1_DER_ERROR;
                  warn();
		  goto cleanup;
		}
	      p2 = p2->down;
	      while (p2)
		{
		  if ((p2->type & CONST_SET) && (p2->type & CONST_NOT_USED))
		    {
		      ris =
			  extract_tag_der_recursive (p2, der + counter,
						     ider_len, &len2, NULL, flags);
		      if (ris == ASN1_SUCCESS)
			{
			  p2->type &= ~CONST_NOT_USED;
			  p = p2;
			  break;
			}
		    }
		  p2 = p2->right;
		}
	      if (p2 == NULL)
		{
		  result = ASN1_DER_ERROR;
                  warn();
		  goto cleanup;
		}
	    }

	  /* the position in the DER structure this starts */
	  p->start = counter;
	  p->end = total_len - 1;

	  if ((p->type & CONST_OPTION) || (p->type & CONST_DEFAULT))
	    {
	      p2 = _asn1_find_up (p);
	      len2 = p2->tmp_ival;
	      if (counter == len2)
		{
		  if (p->right)
		    {
		      p2 = p->right;
		      move = RIGHT;
		    }
		  else
		    move = UP;

		  if (p->type & CONST_OPTION)
		    asn1_delete_structure (&p);

		  p = p2;
		  continue;
		}
	    }

	  if (type_field (p->type) == ASN1_ETYPE_CHOICE)
	    {
	      while (p->down)
		{
		  ris =
		      extract_tag_der_recursive (p->down, der + counter,
					         ider_len, &len2, NULL, flags);

		  if (ris == ASN1_SUCCESS)
		    {
		      delete_unneeded_choice_fields(p->down);
		      break;
		    }
		  else if (ris == ASN1_ERROR_TYPE_ANY)
		    {
		      result = ASN1_ERROR_TYPE_ANY;
                      warn();
		      goto cleanup;
		    }
		  else
		    {
		      p2 = p->down;
		      asn1_delete_structure (&p2);
		    }
		}

	      if (p->down == NULL)
		{
		  if (!(p->type & CONST_OPTION))
		    {
		      result = ASN1_DER_ERROR;
                      warn();
		      goto cleanup;
		    }
		}
	      else if (type_field (p->type) != ASN1_ETYPE_CHOICE)
		p = p->down;

	      p->start = counter;
	    }

	  if ((p->type & CONST_OPTION) || (p->type & CONST_DEFAULT))
	    {
	      p2 = _asn1_find_up (p);
	      len2 = p2->tmp_ival;

	      if ((len2 != -1) && (counter > len2))
		ris = ASN1_TAG_ERROR;
	    }

	  if (ris == ASN1_SUCCESS)
	    ris =
	      extract_tag_der_recursive (p, der + counter, ider_len, 
	                                 &tag_len, &inner_tag_len, flags);

	  if (ris != ASN1_SUCCESS)
	    {
	      if (p->type & CONST_OPTION)
		{
		  p->type |= CONST_NOT_USED;
		  move = RIGHT;
		}
	      else if (p->type & CONST_DEFAULT)
		{
		  _asn1_set_value (p, NULL, 0);
		  move = RIGHT;
		}
	      else
		{
		  if (errorDescription != NULL)
		    _asn1_error_description_tag_error (p, errorDescription);

		  result = ASN1_TAG_ERROR;
                  warn();
		  goto cleanup;
		}
	    }
	  else
	    {
	      DECR_LEN(ider_len, tag_len);
	      counter += tag_len;
	    }
	}

      if (ris == ASN1_SUCCESS)
	{
	  switch (type_field (p->type))
	    {
	    case ASN1_ETYPE_NULL:
	      DECR_LEN(ider_len, 1);
	      if (der[counter])
		{
		  result = ASN1_DER_ERROR;
                  warn();
		  goto cleanup;
		}
	      counter++;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_BOOLEAN:
	      DECR_LEN(ider_len, 2);

	      if (der[counter++] != 1)
		{
		  result = ASN1_DER_ERROR;
                  warn();
		  goto cleanup;
		}
	      if (der[counter++] == 0)
		_asn1_set_value (p, "F", 1);
	      else
		_asn1_set_value (p, "T", 1);
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_INTEGER:
	    case ASN1_ETYPE_ENUMERATED:
	      len2 =
		asn1_get_length_der (der + counter, ider_len, &len3);
	      if (len2 < 0)
		{
		  result = ASN1_DER_ERROR;
                  warn();
		  goto cleanup;
		}

	      DECR_LEN(ider_len, len3+len2);

	      _asn1_set_value (p, der + counter, len3 + len2);
	      counter += len3 + len2;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_OBJECT_ID:
	      result =
		asn1_get_object_id_der (der + counter, ider_len, &len2,
					temp, sizeof (temp));
	      if (result != ASN1_SUCCESS)
	        {
                  warn();
		  goto cleanup;
		}

	      DECR_LEN(ider_len, len2);

	      tlen = strlen (temp);
	      if (tlen > 0)
		_asn1_set_value (p, temp, tlen + 1);

	      counter += len2;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_GENERALIZED_TIME:
	    case ASN1_ETYPE_UTC_TIME:
	      result =
		_asn1_get_time_der (type_field (p->type), der + counter, ider_len, &len2, temp,
				    sizeof (temp) - 1, flags);
	      if (result != ASN1_SUCCESS)
	        {
                  warn();
                  goto cleanup;
                }

	      DECR_LEN(ider_len, len2);

	      tlen = strlen (temp);
	      if (tlen > 0)
		_asn1_set_value (p, temp, tlen);

	      counter += len2;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_OCTET_STRING:
	      if (counter < inner_tag_len)
	        {
		  result = ASN1_DER_ERROR;
                  warn();
		  goto cleanup;
	        }

              ptag = der + counter - inner_tag_len;
              if (flags & ASN1_DECODE_FLAG_STRICT_DER || !(ptag[0] & ASN1_CLASS_STRUCTURED))
                {
	          len2 =
		    asn1_get_length_der (der + counter, ider_len, &len3);
	          if (len2 < 0)
		    {
		      result = ASN1_DER_ERROR;
                      warn();
		      goto cleanup;
		    }

	          DECR_LEN(ider_len, len3+len2);

	          _asn1_set_value (p, der + counter, len3 + len2);
	          counter += len3 + len2;
                }
              else
                {
                  unsigned dflags = 0, vlen, ber_len;

                  if (ptag[0] & ASN1_CLASS_STRUCTURED)
                    dflags |= DECODE_FLAG_INDEFINITE;

                  result = _asn1_decode_simple_ber(type_field (p->type), der+counter, ider_len, &ptmp, &vlen, &ber_len, dflags);
                  if (result != ASN1_SUCCESS)
	            {
                      warn();
		      goto cleanup;
		    }

		  DECR_LEN(ider_len, ber_len);

		  _asn1_set_value_lv (p, ptmp, vlen);

	          counter += ber_len;
	          free(ptmp);
                }
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_GENERALSTRING:
	    case ASN1_ETYPE_NUMERIC_STRING:
	    case ASN1_ETYPE_IA5_STRING:
	    case ASN1_ETYPE_TELETEX_STRING:
	    case ASN1_ETYPE_PRINTABLE_STRING:
	    case ASN1_ETYPE_UNIVERSAL_STRING:
	    case ASN1_ETYPE_BMP_STRING:
	    case ASN1_ETYPE_UTF8_STRING:
	    case ASN1_ETYPE_VISIBLE_STRING:
	    case ASN1_ETYPE_BIT_STRING:
	      len2 =
		asn1_get_length_der (der + counter, ider_len, &len3);
	      if (len2 < 0)
		{
		  result = ASN1_DER_ERROR;
                  warn();
		  goto cleanup;
		}

	      DECR_LEN(ider_len, len3+len2);

	      _asn1_set_value (p, der + counter, len3 + len2);
	      counter += len3 + len2;
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_SEQUENCE:
	    case ASN1_ETYPE_SET:
	      if (move == UP)
		{
		  len2 = p->tmp_ival;
		  p->tmp_ival = 0;
		  if (len2 == -1)
		    {		/* indefinite length method */
		      DECR_LEN(ider_len, 2);
		      if ((der[counter]) || der[counter + 1])
		        {
		          result = ASN1_DER_ERROR;
                          warn();
		          goto cleanup;
			}
		      counter += 2;
		    }
		  else
		    {		/* definite length method */
		      if (len2 != counter)
			{
			  result = ASN1_DER_ERROR;
                          warn();
			  goto cleanup;
			}
		    }
		  move = RIGHT;
		}
	      else
		{		/* move==DOWN || move==RIGHT */
		  len3 =
		    asn1_get_length_der (der + counter, ider_len, &len2);
                  if (IS_ERR(len3, flags))
		    {
		      result = ASN1_DER_ERROR;
                      warn();
		      goto cleanup;
		    }

	          DECR_LEN(ider_len, len2);
		  counter += len2;

		  if (len3 > 0)
		    {
		      p->tmp_ival = counter + len3;
		      move = DOWN;
		    }
		  else if (len3 == 0)
		    {
		      p2 = p->down;
		      while (p2)
			{
			  if (type_field (p2->type) != ASN1_ETYPE_TAG)
			    {
			      p3 = p2->right;
			      asn1_delete_structure (&p2);
			      p2 = p3;
			    }
			  else
			    p2 = p2->right;
			}
		      move = RIGHT;
		    }
		  else
		    {		/* indefinite length method */
		      p->tmp_ival = -1;
		      move = DOWN;
		    }
		}
	      break;
	    case ASN1_ETYPE_SEQUENCE_OF:
	    case ASN1_ETYPE_SET_OF:
	      if (move == UP)
		{
		  len2 = p->tmp_ival;
		  if (len2 == -1)
		    {		/* indefinite length method */
		      if (!HAVE_TWO(ider_len) || ((der[counter]) || der[counter + 1]))
			{
			  result = _asn1_append_sequence_set (p, &tcache);
			  if (result != 0)
			    {
                              warn();
		              goto cleanup;
		            }
			  p = tcache.tail;
			  move = RIGHT;
			  continue;
			}

		      p->tmp_ival = 0;
		      tcache.tail = NULL; /* finished decoding this structure */
		      tcache.head = NULL;
		      DECR_LEN(ider_len, 2);
		      counter += 2;
		    }
		  else
		    {		/* definite length method */
		      if (len2 > counter)
			{
			  result = _asn1_append_sequence_set (p, &tcache);
			  if (result != 0)
			    {
                              warn();
		              goto cleanup;
		            }
			  p = tcache.tail;
			  move = RIGHT;
			  continue;
			}

		      p->tmp_ival = 0;
		      tcache.tail = NULL; /* finished decoding this structure */
		      tcache.head = NULL;

		      if (len2 != counter)
			{
			  result = ASN1_DER_ERROR;
                          warn();
			  goto cleanup;
			}
		    }
		}
	      else
		{		/* move==DOWN || move==RIGHT */
		  len3 =
		    asn1_get_length_der (der + counter, ider_len, &len2);
                  if (IS_ERR(len3, flags))
		    {
		      result = ASN1_DER_ERROR;
                      warn();
		      goto cleanup;
		    }

		  DECR_LEN(ider_len, len2);
		  counter += len2;
		  if (len3)
		    {
		      if (len3 > 0)
			{	/* definite length method */
		          p->tmp_ival = counter + len3;
			}
		      else
			{	/* indefinite length method */
		          p->tmp_ival = -1;
			}

		      p2 = p->down;
                      if (p2 == NULL)
		        {
		          result = ASN1_DER_ERROR;
                          warn();
		          goto cleanup;
		        }

		      while ((type_field (p2->type) == ASN1_ETYPE_TAG)
			     || (type_field (p2->type) == ASN1_ETYPE_SIZE))
			p2 = p2->right;
		      if (p2->right == NULL)
		        {
			  result = _asn1_append_sequence_set (p, &tcache);
			  if (result != 0)
			    {
                              warn();
		              goto cleanup;
		            }
			}
		      p = p2;
		    }
		}
	      move = RIGHT;
	      break;
	    case ASN1_ETYPE_ANY:
	      /* Check indefinite lenth method in an EXPLICIT TAG */
              
	      if (!(flags & ASN1_DECODE_FLAG_STRICT_DER) && (p->type & CONST_TAG) && 
	          tag_len == 2 && (der[counter - 1] == 0x80))
		indefinite = 1;
	      else
	        indefinite = 0;

	      if (asn1_get_tag_der
		  (der + counter, ider_len, &class, &len2,
		   &tag) != ASN1_SUCCESS)
		{
		  result = ASN1_DER_ERROR;
                  warn();
		  goto cleanup;
		}

	      DECR_LEN(ider_len, len2);

	      len4 =
		asn1_get_length_der (der + counter + len2,
				     ider_len, &len3);
              if (IS_ERR(len4, flags))
		{
		  result = ASN1_DER_ERROR;
                  warn();
		  goto cleanup;
		}
	      if (len4 != -1) /* definite */
		{
		  len2 += len4;

	          DECR_LEN(ider_len, len4+len3);
		  _asn1_set_value_lv (p, der + counter, len2 + len3);
		  counter += len2 + len3;
		}
	      else /* == -1 */
		{		/* indefinite length */
		  ider_len += len2; /* undo DECR_LEN */

		  if (counter == 0)
		    {
		      result = ASN1_DER_ERROR;
                      warn();
		      goto cleanup;
		    }

		  result =
		    _asn1_get_indefinite_length_string (der + counter, ider_len, &len2);
		  if (result != ASN1_SUCCESS)
		    {
                      warn();
                      goto cleanup;
                    }

	          DECR_LEN(ider_len, len2);
		  _asn1_set_value_lv (p, der + counter, len2);
		  counter += len2;

		}

	        /* Check if a couple of 0x00 are present due to an EXPLICIT TAG with
	           an indefinite length method. */
	        if (indefinite)
		  {
	            DECR_LEN(ider_len, 2);
		    if (!der[counter] && !der[counter + 1])
		      {
		        counter += 2;
		      }
		    else
		      {
		        result = ASN1_DER_ERROR;
                        warn();
		        goto cleanup;
		      }
		  }

	      move = RIGHT;
	      break;
	    default:
	      move = (move == UP) ? RIGHT : DOWN;
	      break;
	    }
	}

      if (p)
        {
          p->end = counter - 1;
        }

      if (p == node && move != DOWN)
	break;

      if (move == DOWN)
	{
	  if (p->down)
	    p = p->down;
	  else
	    move = RIGHT;
	}
      if ((move == RIGHT) && !(p->type & CONST_SET))
	{
	  if (p->right)
	    p = p->right;
	  else
	    move = UP;
	}
      if (move == UP)
	p = _asn1_find_up (p);
    }

  _asn1_delete_not_used (*element);

  if ((ider_len < 0) ||
      (!(flags & ASN1_DECODE_FLAG_ALLOW_PADDING) && (ider_len != 0)))
    {
      warn();
      result = ASN1_DER_ERROR;
      goto cleanup;
    }

  *max_ider_len = total_len - ider_len;

  return ASN1_SUCCESS;

cleanup:
  asn1_delete_structure (element);
  return result;
}


/**
 * asn1_der_decoding:
 * @element: pointer to an ASN1 structure.
 * @ider: vector that contains the DER encoding.
 * @ider_len: number of bytes of *@ider: @ider[0]..@ider[len-1].
 * @errorDescription: null-terminated string contains details when an
 *   error occurred.
 *
 * Fill the structure *@element with values of a DER encoding
 * string. The structure must just be created with function
 * asn1_create_element(). 
 *
 * Note that the *@element variable is provided as a pointer for
 * historical reasons.
 *
 * Returns: %ASN1_SUCCESS if DER encoding OK, %ASN1_ELEMENT_NOT_FOUND
 *   if @ELEMENT is %NULL, and %ASN1_TAG_ERROR or
 *   %ASN1_DER_ERROR if the der encoding doesn't match the structure
 *   name (*@ELEMENT deleted).
 **/
int
asn1_der_decoding (asn1_node * element, const void *ider, int ider_len,
		   char *errorDescription)
{
  return asn1_der_decoding2 (element, ider, &ider_len, 0, errorDescription);
}

/**
 * asn1_der_decoding_element:
 * @structure: pointer to an ASN1 structure
 * @elementName: name of the element to fill
 * @ider: vector that contains the DER encoding of the whole structure.
 * @len: number of bytes of *der: der[0]..der[len-1]
 * @errorDescription: null-terminated string contains details when an
 *   error occurred.
 *
 * Fill the element named @ELEMENTNAME with values of a DER encoding
 * string.  The structure must just be created with function
 * asn1_create_element().  The DER vector must contain the encoding
 * string of the whole @STRUCTURE.  If an error occurs during the
 * decoding procedure, the *@STRUCTURE is deleted and set equal to
 * %NULL.
 *
 * This function is deprecated and may just be an alias to asn1_der_decoding
 * in future versions. Use asn1_der_decoding() instead.
 *
 * Returns: %ASN1_SUCCESS if DER encoding OK, %ASN1_ELEMENT_NOT_FOUND
 *   if ELEMENT is %NULL or @elementName == NULL, and
 *   %ASN1_TAG_ERROR or %ASN1_DER_ERROR if the der encoding doesn't
 *   match the structure @structure (*ELEMENT deleted).
 **/
int
asn1_der_decoding_element (asn1_node * structure, const char *elementName,
			   const void *ider, int len, char *errorDescription)
{
  return asn1_der_decoding(structure, ider, len, errorDescription);
}

/**
 * asn1_der_decoding_startEnd:
 * @element: pointer to an ASN1 element
 * @ider: vector that contains the DER encoding.
 * @ider_len: number of bytes of *@ider: @ider[0]..@ider[len-1]
 * @name_element: an element of NAME structure.
 * @start: the position of the first byte of NAME_ELEMENT decoding
 *   (@ider[*start])
 * @end: the position of the last byte of NAME_ELEMENT decoding
 *  (@ider[*end])
 *
 * Find the start and end point of an element in a DER encoding
 * string. I mean that if you have a der encoding and you have already
 * used the function asn1_der_decoding() to fill a structure, it may
 * happen that you want to find the piece of string concerning an
 * element of the structure.
 *
 * One example is the sequence "tbsCertificate" inside an X509
 * certificate.
 *
 * Note that since libtasn1 3.7 the @ider and @ider_len parameters
 * can be omitted, if the element is already decoded using asn1_der_decoding().
 *
 * Returns: %ASN1_SUCCESS if DER encoding OK, %ASN1_ELEMENT_NOT_FOUND
 *   if ELEMENT is %asn1_node EMPTY or @name_element is not a valid
 *   element, %ASN1_TAG_ERROR or %ASN1_DER_ERROR if the der encoding
 *   doesn't match the structure ELEMENT.
 **/
int
asn1_der_decoding_startEnd (asn1_node element, const void *ider, int ider_len,
			    const char *name_element, int *start, int *end)
{
  asn1_node node, node_to_find;
  int result = ASN1_DER_ERROR;

  node = element;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  node_to_find = asn1_find_node (node, name_element);

  if (node_to_find == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  *start = node_to_find->start;
  *end = node_to_find->end;

  if (*start == 0 && *end == 0)
    {
      if (ider == NULL || ider_len == 0)
        return ASN1_GENERIC_ERROR;

      /* it seems asn1_der_decoding() wasn't called before. Do it now */
      result = asn1_der_decoding (&node, ider, ider_len, NULL);
      if (result != ASN1_SUCCESS)
        {
          warn();
          return result;
        }

      node_to_find = asn1_find_node (node, name_element);
      if (node_to_find == NULL)
        return ASN1_ELEMENT_NOT_FOUND;

      *start = node_to_find->start;
      *end = node_to_find->end;
    }

  if (*end < *start)
    return ASN1_GENERIC_ERROR;

  return ASN1_SUCCESS;
}

/**
 * asn1_expand_any_defined_by:
 * @definitions: ASN1 definitions
 * @element: pointer to an ASN1 structure
 *
 * Expands every "ANY DEFINED BY" element of a structure created from
 * a DER decoding process (asn1_der_decoding function). The element
 * ANY must be defined by an OBJECT IDENTIFIER. The type used to
 * expand the element ANY is the first one following the definition of
 * the actual value of the OBJECT IDENTIFIER.
 *
 * Returns: %ASN1_SUCCESS if Substitution OK, %ASN1_ERROR_TYPE_ANY if
 *   some "ANY DEFINED BY" element couldn't be expanded due to a
 *   problem in OBJECT_ID -> TYPE association, or other error codes
 *   depending on DER decoding.
 **/
int
asn1_expand_any_defined_by (asn1_node definitions, asn1_node * element)
{
  char name[2 * ASN1_MAX_NAME_SIZE + 1],
    value[ASN1_MAX_NAME_SIZE];
  int retCode = ASN1_SUCCESS, result;
  int len, len2, len3;
  asn1_node p, p2, p3, aux = NULL;
  char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];
  const char *definitionsName;

  if ((definitions == NULL) || (*element == NULL))
    return ASN1_ELEMENT_NOT_FOUND;

  definitionsName = definitions->name;

  p = *element;
  while (p)
    {

      switch (type_field (p->type))
	{
	case ASN1_ETYPE_ANY:
	  if ((p->type & CONST_DEFINED_BY) && (p->value))
	    {
	      /* search the "DEF_BY" element */
	      p2 = p->down;
	      while ((p2) && (type_field (p2->type) != ASN1_ETYPE_CONSTANT))
		p2 = p2->right;

	      if (!p2)
		{
		  retCode = ASN1_ERROR_TYPE_ANY;
		  break;
		}

	      p3 = _asn1_find_up (p);

	      if (!p3)
		{
		  retCode = ASN1_ERROR_TYPE_ANY;
		  break;
		}

	      p3 = p3->down;
	      while (p3)
		{
		  if (!(strcmp (p3->name, p2->name)))
		    break;
		  p3 = p3->right;
		}

	      if ((!p3) || (type_field (p3->type) != ASN1_ETYPE_OBJECT_ID) ||
		  (p3->value == NULL))
		{

		  p3 = _asn1_find_up (p);
		  p3 = _asn1_find_up (p3);

		  if (!p3)
		    {
		      retCode = ASN1_ERROR_TYPE_ANY;
		      break;
		    }

		  p3 = p3->down;

		  while (p3)
		    {
		      if (!(strcmp (p3->name, p2->name)))
			break;
		      p3 = p3->right;
		    }

		  if ((!p3) || (type_field (p3->type) != ASN1_ETYPE_OBJECT_ID)
		      || (p3->value == NULL))
		    {
		      retCode = ASN1_ERROR_TYPE_ANY;
		      break;
		    }
		}

	      /* search the OBJECT_ID into definitions */
	      p2 = definitions->down;
	      while (p2)
		{
		  if ((type_field (p2->type) == ASN1_ETYPE_OBJECT_ID) &&
		      (p2->type & CONST_ASSIGN))
		    {
		      snprintf(name, sizeof(name), "%s.%s", definitionsName, p2->name);

		      len = ASN1_MAX_NAME_SIZE;
		      result =
			asn1_read_value (definitions, name, value, &len);

		      if ((result == ASN1_SUCCESS)
			  && (!_asn1_strcmp (p3->value, value)))
			{
			  p2 = p2->right;	/* pointer to the structure to
						   use for expansion */
			  while ((p2) && (p2->type & CONST_ASSIGN))
			    p2 = p2->right;

			  if (p2)
			    {
			      snprintf(name, sizeof(name), "%s.%s", definitionsName, p2->name);

			      result =
				asn1_create_element (definitions, name, &aux);
			      if (result == ASN1_SUCCESS)
				{
				  _asn1_cpy_name (aux, p);
				  len2 =
				    asn1_get_length_der (p->value,
							 p->value_len, &len3);
				  if (len2 < 0)
				    return ASN1_DER_ERROR;

				  result =
				    asn1_der_decoding (&aux, p->value + len3,
						       len2,
						       errorDescription);
				  if (result == ASN1_SUCCESS)
				    {

				      _asn1_set_right (aux, p->right);
				      _asn1_set_right (p, aux);

				      result = asn1_delete_structure (&p);
				      if (result == ASN1_SUCCESS)
					{
					  p = aux;
					  aux = NULL;
					  break;
					}
				      else
					{	/* error with asn1_delete_structure */
					  asn1_delete_structure (&aux);
					  retCode = result;
					  break;
					}
				    }
				  else
				    {	/* error with asn1_der_decoding */
				      retCode = result;
				      break;
				    }
				}
			      else
				{	/* error with asn1_create_element */
				  retCode = result;
				  break;
				}
			    }
			  else
			    {	/* error with the pointer to the structure to exapand */
			      retCode = ASN1_ERROR_TYPE_ANY;
			      break;
			    }
			}
		    }
		  p2 = p2->right;
		}		/* end while */

	      if (!p2)
		{
		  retCode = ASN1_ERROR_TYPE_ANY;
		  break;
		}

	    }
	  break;
	default:
	  break;
	}


      if (p->down)
	{
	  p = p->down;
	}
      else if (p == *element)
	{
	  p = NULL;
	  break;
	}
      else if (p->right)
	p = p->right;
      else
	{
	  while (1)
	    {
	      p = _asn1_find_up (p);
	      if (p == *element)
		{
		  p = NULL;
		  break;
		}
	      if (p->right)
		{
		  p = p->right;
		  break;
		}
	    }
	}
    }

  return retCode;
}

/**
 * asn1_expand_octet_string:
 * @definitions: ASN1 definitions
 * @element: pointer to an ASN1 structure
 * @octetName: name of the OCTECT STRING field to expand.
 * @objectName: name of the OBJECT IDENTIFIER field to use to define
 *    the type for expansion.
 *
 * Expands an "OCTET STRING" element of a structure created from a DER
 * decoding process (the asn1_der_decoding() function).  The type used
 * for expansion is the first one following the definition of the
 * actual value of the OBJECT IDENTIFIER indicated by OBJECTNAME.
 *
 * Returns: %ASN1_SUCCESS if substitution OK, %ASN1_ELEMENT_NOT_FOUND
 *   if @objectName or @octetName are not correct,
 *   %ASN1_VALUE_NOT_VALID if it wasn't possible to find the type to
 *   use for expansion, or other errors depending on DER decoding.
 **/
int
asn1_expand_octet_string (asn1_node definitions, asn1_node * element,
			  const char *octetName, const char *objectName)
{
  char name[2 * ASN1_MAX_NAME_SIZE + 1], value[ASN1_MAX_NAME_SIZE];
  int retCode = ASN1_SUCCESS, result;
  int len, len2, len3;
  asn1_node p2, aux = NULL;
  asn1_node octetNode = NULL, objectNode = NULL;
  char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

  if ((definitions == NULL) || (*element == NULL))
    return ASN1_ELEMENT_NOT_FOUND;

  octetNode = asn1_find_node (*element, octetName);
  if (octetNode == NULL)
    return ASN1_ELEMENT_NOT_FOUND;
  if (type_field (octetNode->type) != ASN1_ETYPE_OCTET_STRING)
    return ASN1_ELEMENT_NOT_FOUND;
  if (octetNode->value == NULL)
    return ASN1_VALUE_NOT_FOUND;

  objectNode = asn1_find_node (*element, objectName);
  if (objectNode == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  if (type_field (objectNode->type) != ASN1_ETYPE_OBJECT_ID)
    return ASN1_ELEMENT_NOT_FOUND;

  if (objectNode->value == NULL)
    return ASN1_VALUE_NOT_FOUND;


  /* search the OBJECT_ID into definitions */
  p2 = definitions->down;
  while (p2)
    {
      if ((type_field (p2->type) == ASN1_ETYPE_OBJECT_ID) &&
	  (p2->type & CONST_ASSIGN))
	{
	  strcpy (name, definitions->name);
	  strcat (name, ".");
	  strcat (name, p2->name);

	  len = sizeof (value);
	  result = asn1_read_value (definitions, name, value, &len);

	  if ((result == ASN1_SUCCESS)
	      && (!_asn1_strcmp (objectNode->value, value)))
	    {

	      p2 = p2->right;	/* pointer to the structure to
				   use for expansion */
	      while ((p2) && (p2->type & CONST_ASSIGN))
		p2 = p2->right;

	      if (p2)
		{
		  strcpy (name, definitions->name);
		  strcat (name, ".");
		  strcat (name, p2->name);

		  result = asn1_create_element (definitions, name, &aux);
		  if (result == ASN1_SUCCESS)
		    {
		      _asn1_cpy_name (aux, octetNode);
		      len2 =
			asn1_get_length_der (octetNode->value,
					     octetNode->value_len, &len3);
		      if (len2 < 0)
			return ASN1_DER_ERROR;

		      result =
			asn1_der_decoding (&aux, octetNode->value + len3,
					   len2, errorDescription);
		      if (result == ASN1_SUCCESS)
			{

			  _asn1_set_right (aux, octetNode->right);
			  _asn1_set_right (octetNode, aux);

			  result = asn1_delete_structure (&octetNode);
			  if (result == ASN1_SUCCESS)
			    {
			      aux = NULL;
			      break;
			    }
			  else
			    {	/* error with asn1_delete_structure */
			      asn1_delete_structure (&aux);
			      retCode = result;
			      break;
			    }
			}
		      else
			{	/* error with asn1_der_decoding */
			  retCode = result;
			  break;
			}
		    }
		  else
		    {		/* error with asn1_create_element */
		      retCode = result;
		      break;
		    }
		}
	      else
		{		/* error with the pointer to the structure to exapand */
		  retCode = ASN1_VALUE_NOT_VALID;
		  break;
		}
	    }
	}

      p2 = p2->right;

    }

  if (!p2)
    retCode = ASN1_VALUE_NOT_VALID;

  return retCode;
}

/*-
 * _asn1_decode_simple_der:
 * @etype: The type of the string to be encoded (ASN1_ETYPE_)
 * @der: the encoded string
 * @_der_len: the bytes of the encoded string
 * @str: a pointer to the data
 * @str_len: the length of the data
 * @dflags: DECODE_FLAG_*
 *
 * Decodes a simple DER encoded type (e.g. a string, which is not constructed).
 * The output is a pointer inside the @der.
 *
 * Returns: %ASN1_SUCCESS if successful or an error value.
 -*/
static int
_asn1_decode_simple_der (unsigned int etype, const unsigned char *der,
			unsigned int _der_len, const unsigned char **str,
			unsigned int *str_len, unsigned dflags)
{
  int tag_len, len_len;
  const unsigned char *p;
  int der_len = _der_len;
  unsigned char class;
  unsigned long tag;
  long ret;

  if (der == NULL || der_len == 0)
    return ASN1_VALUE_NOT_VALID;

  if (ETYPE_OK (etype) == 0 || ETYPE_IS_STRING(etype) == 0)
    return ASN1_VALUE_NOT_VALID;

  /* doesn't handle constructed classes */
  class = ETYPE_CLASS(etype);
  if (class != ASN1_CLASS_UNIVERSAL)
    return ASN1_VALUE_NOT_VALID;

  p = der;

  if (dflags & DECODE_FLAG_HAVE_TAG)
    {
      ret = asn1_get_tag_der (p, der_len, &class, &tag_len, &tag);
      if (ret != ASN1_SUCCESS)
        return ret;

      if (class != ETYPE_CLASS (etype) || tag != ETYPE_TAG (etype))
        {
          warn();
          return ASN1_DER_ERROR;
        }

      p += tag_len;
      der_len -= tag_len;
      if (der_len <= 0)
        return ASN1_DER_ERROR;
    }

  ret = asn1_get_length_der (p, der_len, &len_len);
  if (ret < 0)
    return ASN1_DER_ERROR;

  p += len_len;
  der_len -= len_len;
  if (der_len <= 0)
    return ASN1_DER_ERROR;

  *str_len = ret;
  *str = p;

  return ASN1_SUCCESS;
}

/**
 * asn1_decode_simple_der:
 * @etype: The type of the string to be encoded (ASN1_ETYPE_)
 * @der: the encoded string
 * @_der_len: the bytes of the encoded string
 * @str: a pointer to the data
 * @str_len: the length of the data
 *
 * Decodes a simple DER encoded type (e.g. a string, which is not constructed).
 * The output is a pointer inside the @der.
 *
 * Returns: %ASN1_SUCCESS if successful or an error value.
 **/
int
asn1_decode_simple_der (unsigned int etype, const unsigned char *der,
			unsigned int _der_len, const unsigned char **str,
			unsigned int *str_len)
{
  return _asn1_decode_simple_der(etype, der, _der_len, str, str_len, DECODE_FLAG_HAVE_TAG);
}

static int append(uint8_t **dst, unsigned *dst_size, const unsigned char *src, unsigned src_size)
{
  *dst = _asn1_realloc(*dst, *dst_size+src_size);
  if (*dst == NULL)
    return ASN1_MEM_ERROR;
  memcpy(*dst + *dst_size, src, src_size);
  *dst_size += src_size;
  return ASN1_SUCCESS;
}

/*-
 * _asn1_decode_simple_ber:
 * @etype: The type of the string to be encoded (ASN1_ETYPE_)
 * @der: the encoded string
 * @_der_len: the bytes of the encoded string
 * @str: a pointer to the data
 * @str_len: the length of the data
 * @ber_len: the total length occupied by BER (may be %NULL)
 * @have_tag: whether a DER tag is included
 *
 * Decodes a BER encoded type. The output is an allocated value 
 * of the data. This decodes BER STRINGS only. Other types are
 * decoded as DER.
 *
 * Returns: %ASN1_SUCCESS if successful or an error value.
 -*/
static int
_asn1_decode_simple_ber (unsigned int etype, const unsigned char *der,
			unsigned int _der_len, unsigned char **str,
			unsigned int *str_len, unsigned int *ber_len,
			unsigned dflags)
{
  int tag_len, len_len;
  const unsigned char *p;
  int der_len = _der_len;
  uint8_t *total = NULL;
  unsigned total_size = 0;
  unsigned char class;
  unsigned long tag;
  unsigned char *out = NULL;
  const unsigned char *cout = NULL;
  unsigned out_len;
  long result;

  if (ber_len) *ber_len = 0;

  if (der == NULL || der_len == 0)
    {
      warn();
      return ASN1_VALUE_NOT_VALID;
    }

  if (ETYPE_OK (etype) == 0)
    {
      warn();
      return ASN1_VALUE_NOT_VALID;
    }

  /* doesn't handle constructed + definite classes */
  class = ETYPE_CLASS (etype);
  if (class != ASN1_CLASS_UNIVERSAL)
    {
      warn();
      return ASN1_VALUE_NOT_VALID;
    }

  p = der;

  if (dflags & DECODE_FLAG_HAVE_TAG)
    {
      result = asn1_get_tag_der (p, der_len, &class, &tag_len, &tag);
        if (result != ASN1_SUCCESS)
          {
            warn();
            return result;
          }

        if (tag != ETYPE_TAG (etype))
          {
            warn();
            return ASN1_DER_ERROR;
          }

        p += tag_len;

        DECR_LEN(der_len, tag_len);

        if (ber_len) *ber_len += tag_len;
    }

  /* indefinite constructed */
  if ((((dflags & DECODE_FLAG_INDEFINITE) || class == ASN1_CLASS_STRUCTURED) && ETYPE_IS_STRING(etype)) &&
      !(dflags & DECODE_FLAG_LEVEL3))
    {
      len_len = 1;

      DECR_LEN(der_len, len_len);
      if (p[0] != 0x80)
        {
          warn();
          result = ASN1_DER_ERROR;
          goto cleanup;
        }

      p += len_len;

      if (ber_len) *ber_len += len_len;

      /* decode the available octet strings */
      do
        {
          unsigned tmp_len;
          unsigned flags = DECODE_FLAG_HAVE_TAG;

          if (dflags & DECODE_FLAG_LEVEL1)
                flags |= DECODE_FLAG_LEVEL2;
          else if (dflags & DECODE_FLAG_LEVEL2)
		flags |= DECODE_FLAG_LEVEL3;
	  else
		flags |= DECODE_FLAG_LEVEL1;

          result = _asn1_decode_simple_ber(etype, p, der_len, &out, &out_len, &tmp_len,
                                           flags);
          if (result != ASN1_SUCCESS)
            {
              warn();
              goto cleanup;
            }

          p += tmp_len;
          DECR_LEN(der_len, tmp_len);

          if (ber_len) *ber_len += tmp_len;

          DECR_LEN(der_len, 2); /* we need the EOC */

	  if (out_len > 0)
	    {
              result = append(&total, &total_size, out, out_len);
              if (result != ASN1_SUCCESS)
                {
                  warn();
                  goto cleanup;
                }
	    }

          free(out);
          out = NULL;

	  if (p[0] == 0 && p[1] == 0) /* EOC */
	    {
              if (ber_len) *ber_len += 2;
              break;
            }

          /* no EOC */
          der_len += 2;

          if (der_len == 2)
            {
              warn();
              result = ASN1_DER_ERROR;
              goto cleanup;
            }
        }
      while(1);
    }
  else if (class == ETYPE_CLASS(etype))
    {
      if (ber_len)
        {
          result = asn1_get_length_der (p, der_len, &len_len);
          if (result < 0)
            {
              warn();
              result = ASN1_DER_ERROR;
              goto cleanup;
            }
          *ber_len += result + len_len;
        }

      /* non-string values are decoded as DER */
      result = _asn1_decode_simple_der(etype, der, _der_len, &cout, &out_len, dflags);
      if (result != ASN1_SUCCESS)
        {
          warn();
          goto cleanup;
        }

      result = append(&total, &total_size, cout, out_len);
      if (result != ASN1_SUCCESS)
        {
          warn();
          goto cleanup;
        }
    }
  else
    {
      warn();
      result = ASN1_DER_ERROR;
      goto cleanup;
    }

  *str = total;
  *str_len = total_size;

  return ASN1_SUCCESS;
cleanup:
  free(out);
  free(total);
  return result;
}

/**
 * asn1_decode_simple_ber:
 * @etype: The type of the string to be encoded (ASN1_ETYPE_)
 * @der: the encoded string
 * @_der_len: the bytes of the encoded string
 * @str: a pointer to the data
 * @str_len: the length of the data
 * @ber_len: the total length occupied by BER (may be %NULL)
 *
 * Decodes a BER encoded type. The output is an allocated value 
 * of the data. This decodes BER STRINGS only. Other types are
 * decoded as DER.
 *
 * Returns: %ASN1_SUCCESS if successful or an error value.
 **/
int
asn1_decode_simple_ber (unsigned int etype, const unsigned char *der,
			unsigned int _der_len, unsigned char **str,
			unsigned int *str_len, unsigned int *ber_len)
{
  return _asn1_decode_simple_ber(etype, der, _der_len, str, str_len, ber_len, DECODE_FLAG_HAVE_TAG);
}
