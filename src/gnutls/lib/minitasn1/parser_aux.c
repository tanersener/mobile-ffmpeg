/*
 * Copyright (C) 2000-2016 Free Software Foundation, Inc.
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

#include <int.h>
#include <hash-pjw-bare.h>
#include "parser_aux.h"
#include "gstr.h"
#include "structure.h"
#include "element.h"

char _asn1_identifierMissing[ASN1_MAX_NAME_SIZE + 1];	/* identifier name not found */

/***********************************************/
/* Type: list_type                             */
/* Description: type used in the list during   */
/* the structure creation.                     */
/***********************************************/
typedef struct list_struct
{
  asn1_node node;
  struct list_struct *next;
} list_type;


/* Pointer to the first element of the list */
list_type *firstElement = NULL;

/******************************************************/
/* Function : _asn1_add_static_node                   */
/* Description: creates a new NODE_ASN element and    */
/* puts it in the list pointed by firstElement.       */
/* Parameters:                                        */
/*   type: type of the new element (see ASN1_ETYPE_   */
/*         and CONST_ constants).                     */
/* Return: pointer to the new element.                */
/******************************************************/
asn1_node
_asn1_add_static_node (unsigned int type)
{
  list_type *listElement;
  asn1_node punt;

  punt = calloc (1, sizeof (struct asn1_node_st));
  if (punt == NULL)
    return NULL;

  listElement = malloc (sizeof (list_type));
  if (listElement == NULL)
    {
      free (punt);
      return NULL;
    }

  listElement->node = punt;
  listElement->next = firstElement;
  firstElement = listElement;

  punt->type = type;

  return punt;
}

/**
 * asn1_find_node:
 * @pointer: NODE_ASN element pointer.
 * @name: null terminated string with the element's name to find.
 *
 * Searches for an element called @name starting from @pointer.  The
 * name is composed by different identifiers separated by dots.  When
 * *@pointer has a name, the first identifier must be the name of
 * *@pointer, otherwise it must be the name of one child of *@pointer.
 *
 * Returns: the search result, or %NULL if not found.
 **/
asn1_node
asn1_find_node (asn1_node pointer, const char *name)
{
  asn1_node p;
  char *n_end, n[ASN1_MAX_NAME_SIZE + 1];
  const char *n_start;
  unsigned int nsize;
  unsigned int nhash;

  if (pointer == NULL)
    return NULL;

  if (name == NULL)
    return NULL;

  p = pointer;
  n_start = name;

  if (name[0] == '?' && name[1] == 'C' && p->name[0] == '?')
    { /* ?CURRENT */
      n_start = strchr(n_start, '.');
      if (n_start)
        n_start++;
    }
  else if (p->name[0] != 0)
    {				/* has *pointer got a name ? */
      n_end = strchr (n_start, '.');	/* search the first dot */
      if (n_end)
	{
	  nsize = n_end - n_start;
	  if (nsize >= sizeof(n))
		return NULL;

	  memcpy (n, n_start, nsize);
	  n[nsize] = 0;
	  n_start = n_end;
	  n_start++;

	  nhash = hash_pjw_bare (n, nsize);
	}
      else
	{
	  nsize = _asn1_str_cpy (n, sizeof (n), n_start);
	  nhash = hash_pjw_bare (n, nsize);

	  n_start = NULL;
	}

      while (p)
	{
	  if (nhash == p->name_hash && (!strcmp (p->name, n)))
	    break;
	  else
	    p = p->right;
	}			/* while */

      if (p == NULL)
	return NULL;
    }
  else
    {				/* *pointer doesn't have a name */
      if (n_start[0] == 0)
	return p;
    }

  while (n_start)
    {				/* Has the end of NAME been reached? */
      n_end = strchr (n_start, '.');	/* search the next dot */
      if (n_end)
	{
	  nsize = n_end - n_start;
	  if (nsize >= sizeof(n))
		return NULL;

	  memcpy (n, n_start, nsize);
	  n[nsize] = 0;
	  n_start = n_end;
	  n_start++;

	  nhash = hash_pjw_bare (n, nsize);
	}
      else
	{
	  nsize = _asn1_str_cpy (n, sizeof (n), n_start);
	  nhash = hash_pjw_bare (n, nsize);
	  n_start = NULL;
	}

      if (p->down == NULL)
	return NULL;

      p = p->down;
      if (p == NULL)
        return NULL;

      /* The identifier "?LAST" indicates the last element
         in the right chain. */
      if (n[0] == '?' && n[1] == 'L') /* ?LAST */
	{
	  while (p->right)
	    p = p->right;
	}
      else
	{			/* no "?LAST" */
	  while (p)
	    {
	      if (p->name_hash == nhash && !strcmp (p->name, n))
		break;
	      else
		p = p->right;
	    }
	}
      if (p == NULL)
        return NULL;
    }				/* while */

  return p;
}


/******************************************************************/
/* Function : _asn1_set_value                                     */
/* Description: sets the field VALUE in a NODE_ASN element. The   */
/*              previous value (if exist) will be lost            */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   value: pointer to the value that you want to set.            */
/*   len: character number of value.                              */
/* Return: pointer to the NODE_ASN element.                       */
/******************************************************************/
asn1_node
_asn1_set_value (asn1_node node, const void *value, unsigned int len)
{
  if (node == NULL)
    return node;
  if (node->value)
    {
      if (node->value != node->small_value)
	free (node->value);
      node->value = NULL;
      node->value_len = 0;
    }

  if (!len)
    return node;

  if (len < sizeof (node->small_value))
    {
      node->value = node->small_value;
    }
  else
    {
      node->value = malloc (len);
      if (node->value == NULL)
	return NULL;
    }
  node->value_len = len;

  memcpy (node->value, value, len);
  return node;
}

/******************************************************************/
/* Function : _asn1_set_value_lv                                  */
/* Description: sets the field VALUE in a NODE_ASN element. The   */
/*              previous value (if exist) will be lost. The value */
/*		given is stored as an length-value format (LV     */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   value: pointer to the value that you want to set.            */
/*   len: character number of value.                              */
/* Return: pointer to the NODE_ASN element.                       */
/******************************************************************/
asn1_node
_asn1_set_value_lv (asn1_node node, const void *value, unsigned int len)
{
  int len2;
  void *temp;

  if (node == NULL)
    return node;

  asn1_length_der (len, NULL, &len2);
  temp = malloc (len + len2);
  if (temp == NULL)
    return NULL;

  asn1_octet_der (value, len, temp, &len2);
  return _asn1_set_value_m (node, temp, len2);
}

/* the same as _asn1_set_value except that it sets an already malloc'ed
 * value.
 */
asn1_node
_asn1_set_value_m (asn1_node node, void *value, unsigned int len)
{
  if (node == NULL)
    return node;

  if (node->value)
    {
      if (node->value != node->small_value)
	free (node->value);
      node->value = NULL;
      node->value_len = 0;
    }

  if (!len)
    return node;

  node->value = value;
  node->value_len = len;

  return node;
}

/******************************************************************/
/* Function : _asn1_append_value                                  */
/* Description: appends to the field VALUE in a NODE_ASN element. */
/*							          */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   value: pointer to the value that you want to be appended.    */
/*   len: character number of value.                              */
/* Return: pointer to the NODE_ASN element.                       */
/******************************************************************/
asn1_node
_asn1_append_value (asn1_node node, const void *value, unsigned int len)
{
  if (node == NULL)
    return node;

  if (node->value == NULL)
    return _asn1_set_value (node, value, len);

  if (len == 0)
    return node;

  if (node->value == node->small_value)
    {
      /* value is in node */
      int prev_len = node->value_len;
      node->value_len += len;
      node->value = malloc (node->value_len);
      if (node->value == NULL)
	{
	  node->value_len = 0;
	  return NULL;
	}

      if (prev_len > 0)
        memcpy (node->value, node->small_value, prev_len);

      memcpy (&node->value[prev_len], value, len);

      return node;
    }
  else /* if (node->value != NULL && node->value != node->small_value) */
    {
      /* value is allocated */
      int prev_len = node->value_len;
      node->value_len += len;

      node->value = _asn1_realloc (node->value, node->value_len);
      if (node->value == NULL)
	{
	  node->value_len = 0;
	  return NULL;
	}

      memcpy (&node->value[prev_len], value, len);

      return node;
    }
}

/******************************************************************/
/* Function : _asn1_set_name                                      */
/* Description: sets the field NAME in a NODE_ASN element. The    */
/*              previous value (if exist) will be lost            */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   name: a null terminated string with the name that you want   */
/*         to set.                                                */
/* Return: pointer to the NODE_ASN element.                       */
/******************************************************************/
asn1_node
_asn1_set_name (asn1_node node, const char *name)
{
  unsigned int nsize;

  if (node == NULL)
    return node;

  if (name == NULL)
    {
      node->name[0] = 0;
      node->name_hash = hash_pjw_bare (node->name, 0);
      return node;
    }

  nsize = _asn1_str_cpy (node->name, sizeof (node->name), name);
  node->name_hash = hash_pjw_bare (node->name, nsize);

  return node;
}

/******************************************************************/
/* Function : _asn1_cpy_name                                      */
/* Description: copies the field NAME in a NODE_ASN element.      */
/* Parameters:                                                    */
/*   dst: a dest element pointer.                                 */
/*   src: a source element pointer.                               */
/* Return: pointer to the NODE_ASN element.                       */
/******************************************************************/
asn1_node
_asn1_cpy_name (asn1_node dst, asn1_node src)
{
  if (dst == NULL)
    return dst;

  if (src == NULL)
    {
      dst->name[0] = 0;
      dst->name_hash = hash_pjw_bare (dst->name, 0);
      return dst;
    }

  _asn1_str_cpy (dst->name, sizeof (dst->name), src->name);
  dst->name_hash = src->name_hash;

  return dst;
}

/******************************************************************/
/* Function : _asn1_set_right                                     */
/* Description: sets the field RIGHT in a NODE_ASN element.       */
/* Parameters:                                                    */
/*   node: element pointer.                                       */
/*   right: pointer to a NODE_ASN element that you want be pointed*/
/*          by NODE.                                              */
/* Return: pointer to *NODE.                                      */
/******************************************************************/
asn1_node
_asn1_set_right (asn1_node node, asn1_node right)
{
  if (node == NULL)
    return node;
  node->right = right;
  if (right)
    right->left = node;
  return node;
}


/******************************************************************/
/* Function : _asn1_get_last_right                                */
/* Description: return the last element along the right chain.    */
/* Parameters:                                                    */
/*   node: starting element pointer.                              */
/* Return: pointer to the last element along the right chain.     */
/******************************************************************/
asn1_node
_asn1_get_last_right (asn1_node node)
{
  asn1_node p;

  if (node == NULL)
    return NULL;
  p = node;
  while (p->right)
    p = p->right;
  return p;
}

/******************************************************************/
/* Function : _asn1_remove_node                                   */
/* Description: gets free the memory allocated for an NODE_ASN    */
/*              element (not the elements pointed by it).         */
/* Parameters:                                                    */
/*   node: NODE_ASN element pointer.                              */
/*   flags: ASN1_DELETE_FLAG_*                                    */
/******************************************************************/
void
_asn1_remove_node (asn1_node node, unsigned int flags)
{
  if (node == NULL)
    return;

  if (node->value != NULL)
    {
      if (flags & ASN1_DELETE_FLAG_ZEROIZE)
        {
          safe_memset(node->value, 0, node->value_len);
        }

      if (node->value != node->small_value)
        free (node->value);
    }
  free (node);
}

/******************************************************************/
/* Function : _asn1_find_up                                       */
/* Description: return the father of the NODE_ASN element.        */
/* Parameters:                                                    */
/*   node: NODE_ASN element pointer.                              */
/* Return: Null if not found.                                     */
/******************************************************************/
asn1_node
_asn1_find_up (asn1_node node)
{
  asn1_node p;

  if (node == NULL)
    return NULL;

  p = node;

  while ((p->left != NULL) && (p->left->right == p))
    p = p->left;

  return p->left;
}

/******************************************************************/
/* Function : _asn1_delete_list                                   */
/* Description: deletes the list elements (not the elements       */
/*  pointed by them).                                             */
/******************************************************************/
void
_asn1_delete_list (void)
{
  list_type *listElement;

  while (firstElement)
    {
      listElement = firstElement;
      firstElement = firstElement->next;
      free (listElement);
    }
}

/******************************************************************/
/* Function : _asn1_delete_list_and nodes                         */
/* Description: deletes the list elements and the elements        */
/*  pointed by them.                                              */
/******************************************************************/
void
_asn1_delete_list_and_nodes (void)
{
  list_type *listElement;

  while (firstElement)
    {
      listElement = firstElement;
      firstElement = firstElement->next;
      _asn1_remove_node (listElement->node, 0);
      free (listElement);
    }
}


char *
_asn1_ltostr (int64_t v, char str[LTOSTR_MAX_SIZE])
{
  uint64_t d, r;
  char temp[LTOSTR_MAX_SIZE];
  int count, k, start;
  uint64_t val;

  if (v < 0)
    {
      str[0] = '-';
      start = 1;
      val = -((uint64_t)v);
    }
  else
    {
      val = v;
      start = 0;
    }

  count = 0;
  do
    {
      d = val / 10;
      r = val - d * 10;
      temp[start + count] = '0' + (char) r;
      count++;
      val = d;
    }
  while (val && ((start+count) < LTOSTR_MAX_SIZE-1));

  for (k = 0; k < count; k++)
    str[k + start] = temp[start + count - k - 1];
  str[count + start] = 0;
  return str;
}


/******************************************************************/
/* Function : _asn1_change_integer_value                          */
/* Description: converts into DER coding the value assign to an   */
/*   INTEGER constant.                                            */
/* Parameters:                                                    */
/*   node: root of an ASN1element.                                */
/* Return:                                                        */
/*   ASN1_ELEMENT_NOT_FOUND if NODE is NULL,                       */
/*   otherwise ASN1_SUCCESS                                             */
/******************************************************************/
int
_asn1_change_integer_value (asn1_node node)
{
  asn1_node p;
  unsigned char val[SIZEOF_UNSIGNED_LONG_INT];
  unsigned char val2[SIZEOF_UNSIGNED_LONG_INT + 1];
  int len;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node;
  while (p)
    {
      if ((type_field (p->type) == ASN1_ETYPE_INTEGER)
	  && (p->type & CONST_ASSIGN))
	{
	  if (p->value)
	    {
	      _asn1_convert_integer (p->value, val, sizeof (val), &len);
	      asn1_octet_der (val, len, val2, &len);
	      _asn1_set_value (p, val2, len);
	    }
	}

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
		  if (p && p->right)
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


/******************************************************************/
/* Function : _asn1_expand_object_id                              */
/* Description: expand the IDs of an OBJECT IDENTIFIER constant.  */
/* Parameters:                                                    */
/*   node: root of an ASN1 element.                               */
/* Return:                                                        */
/*   ASN1_ELEMENT_NOT_FOUND if NODE is NULL,                       */
/*   otherwise ASN1_SUCCESS                                             */
/******************************************************************/
int
_asn1_expand_object_id (asn1_node node)
{
  asn1_node p, p2, p3, p4, p5;
  char name_root[ASN1_MAX_NAME_SIZE], name2[2 * ASN1_MAX_NAME_SIZE + 1];
  int move, tlen;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  _asn1_str_cpy (name_root, sizeof (name_root), node->name);

  p = node;
  move = DOWN;

  while (!((p == node) && (move == UP)))
    {
      if (move != UP)
	{
	  if ((type_field (p->type) == ASN1_ETYPE_OBJECT_ID)
	      && (p->type & CONST_ASSIGN))
	    {
	      p2 = p->down;
	      if (p2 && (type_field (p2->type) == ASN1_ETYPE_CONSTANT))
		{
		  if (p2->value && !isdigit (p2->value[0]))
		    {
		      _asn1_str_cpy (name2, sizeof (name2), name_root);
		      _asn1_str_cat (name2, sizeof (name2), ".");
		      _asn1_str_cat (name2, sizeof (name2),
				     (char *) p2->value);
		      p3 = asn1_find_node (node, name2);
		      if (!p3
			  || (type_field (p3->type) != ASN1_ETYPE_OBJECT_ID)
			  || !(p3->type & CONST_ASSIGN))
			return ASN1_ELEMENT_NOT_FOUND;
		      _asn1_set_down (p, p2->right);
		      _asn1_remove_node (p2, 0);
		      p2 = p;
		      p4 = p3->down;
		      while (p4)
			{
			  if (type_field (p4->type) == ASN1_ETYPE_CONSTANT)
			    {
			      p5 =
				_asn1_add_single_node (ASN1_ETYPE_CONSTANT);
			      _asn1_set_name (p5, p4->name);
			      if (p4->value)
			        {
			          tlen = _asn1_strlen (p4->value);
			          if (tlen > 0)
			            _asn1_set_value (p5, p4->value, tlen + 1);
			        }
			      if (p2 == p)
				{
				  _asn1_set_right (p5, p->down);
				  _asn1_set_down (p, p5);
				}
			      else
				{
				  _asn1_set_right (p5, p2->right);
				  _asn1_set_right (p2, p5);
				}
			      p2 = p5;
			    }
			  p4 = p4->right;
			}
		      move = DOWN;
		      continue;
		    }
		}
	    }
	  move = DOWN;
	}
      else
	move = RIGHT;

      if (move == DOWN)
	{
	  if (p->down)
	    p = p->down;
	  else
	    move = RIGHT;
	}

      if (p == node)
	{
	  move = UP;
	  continue;
	}

      if (move == RIGHT)
	{
	  if (p && p->right)
	    p = p->right;
	  else
	    move = UP;
	}
      if (move == UP)
	p = _asn1_find_up (p);
    }


  /*******************************/
  /*       expand DEFAULT        */
  /*******************************/
  p = node;
  move = DOWN;

  while (!((p == node) && (move == UP)))
    {
      if (move != UP)
	{
	  if ((type_field (p->type) == ASN1_ETYPE_OBJECT_ID) &&
	      (p->type & CONST_DEFAULT))
	    {
	      p2 = p->down;
	      if (p2 && (type_field (p2->type) == ASN1_ETYPE_DEFAULT))
		{
		  _asn1_str_cpy (name2, sizeof (name2), name_root);
		  _asn1_str_cat (name2, sizeof (name2), ".");
		  _asn1_str_cat (name2, sizeof (name2), (char *) p2->value);
		  p3 = asn1_find_node (node, name2);
		  if (!p3 || (type_field (p3->type) != ASN1_ETYPE_OBJECT_ID)
		      || !(p3->type & CONST_ASSIGN))
		    return ASN1_ELEMENT_NOT_FOUND;
		  p4 = p3->down;
		  name2[0] = 0;
		  while (p4)
		    {
		      if (type_field (p4->type) == ASN1_ETYPE_CONSTANT)
			{
			  if (p4->value == NULL)
			    return ASN1_VALUE_NOT_FOUND;

			  if (name2[0])
			    _asn1_str_cat (name2, sizeof (name2), ".");
			  _asn1_str_cat (name2, sizeof (name2),
					 (char *) p4->value);
			}
		      p4 = p4->right;
		    }
		  tlen = strlen (name2);
		  if (tlen > 0)
		    _asn1_set_value (p2, name2, tlen + 1);
		}
	    }
	  move = DOWN;
	}
      else
	move = RIGHT;

      if (move == DOWN)
	{
	  if (p->down)
	    p = p->down;
	  else
	    move = RIGHT;
	}

      if (p == node)
	{
	  move = UP;
	  continue;
	}

      if (move == RIGHT)
	{
	  if (p && p->right)
	    p = p->right;
	  else
	    move = UP;
	}
      if (move == UP)
	p = _asn1_find_up (p);
    }

  return ASN1_SUCCESS;
}


/******************************************************************/
/* Function : _asn1_type_set_config                               */
/* Description: sets the CONST_SET and CONST_NOT_USED properties  */
/*   in the fields of the SET elements.                           */
/* Parameters:                                                    */
/*   node: root of an ASN1 element.                               */
/* Return:                                                        */
/*   ASN1_ELEMENT_NOT_FOUND if NODE is NULL,                       */
/*   otherwise ASN1_SUCCESS                                             */
/******************************************************************/
int
_asn1_type_set_config (asn1_node node)
{
  asn1_node p, p2;
  int move;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node;
  move = DOWN;

  while (!((p == node) && (move == UP)))
    {
      if (move != UP)
	{
	  if (type_field (p->type) == ASN1_ETYPE_SET)
	    {
	      p2 = p->down;
	      while (p2)
		{
		  if (type_field (p2->type) != ASN1_ETYPE_TAG)
		    p2->type |= CONST_SET | CONST_NOT_USED;
		  p2 = p2->right;
		}
	    }
	  move = DOWN;
	}
      else
	move = RIGHT;

      if (move == DOWN)
	{
	  if (p->down)
	    p = p->down;
	  else
	    move = RIGHT;
	}

      if (p == node)
	{
	  move = UP;
	  continue;
	}

      if (move == RIGHT)
	{
	  if (p && p->right)
	    p = p->right;
	  else
	    move = UP;
	}
      if (move == UP)
	p = _asn1_find_up (p);
    }

  return ASN1_SUCCESS;
}


/******************************************************************/
/* Function : _asn1_check_identifier                              */
/* Description: checks the definitions of all the identifiers     */
/*   and the first element of an OBJECT_ID (e.g. {pkix 0 4}).     */
/*   The _asn1_identifierMissing global variable is filled if     */
/*   necessary.                                                   */
/* Parameters:                                                    */
/*   node: root of an ASN1 element.                               */
/* Return:                                                        */
/*   ASN1_ELEMENT_NOT_FOUND      if NODE is NULL,                 */
/*   ASN1_IDENTIFIER_NOT_FOUND   if an identifier is not defined, */
/*   otherwise ASN1_SUCCESS                                       */
/******************************************************************/
int
_asn1_check_identifier (asn1_node node)
{
  asn1_node p, p2;
  char name2[ASN1_MAX_NAME_SIZE * 2 + 2];

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node;
  while (p)
    {
      if (p->value && type_field (p->type) == ASN1_ETYPE_IDENTIFIER)
	{
	  _asn1_str_cpy (name2, sizeof (name2), node->name);
	  _asn1_str_cat (name2, sizeof (name2), ".");
	  _asn1_str_cat (name2, sizeof (name2), (char *) p->value);
	  p2 = asn1_find_node (node, name2);
	  if (p2 == NULL)
	    {
	      if (p->value)
		_asn1_str_cpy (_asn1_identifierMissing, sizeof(_asn1_identifierMissing), (char*)p->value);
	      else
		_asn1_strcpy (_asn1_identifierMissing, "(null)");
	      return ASN1_IDENTIFIER_NOT_FOUND;
	    }
	}
      else if ((type_field (p->type) == ASN1_ETYPE_OBJECT_ID) &&
	       (p->type & CONST_DEFAULT))
	{
	  p2 = p->down;
	  if (p2 && (type_field (p2->type) == ASN1_ETYPE_DEFAULT))
	    {
	      _asn1_str_cpy (name2, sizeof (name2), node->name);
	      if (p2->value)
	        {
	          _asn1_str_cat (name2, sizeof (name2), ".");
	          _asn1_str_cat (name2, sizeof (name2), (char *) p2->value);
	          _asn1_str_cpy (_asn1_identifierMissing, sizeof(_asn1_identifierMissing), (char*)p2->value);
	        }
	      else
		_asn1_strcpy (_asn1_identifierMissing, "(null)");

	      p2 = asn1_find_node (node, name2);
	      if (!p2 || (type_field (p2->type) != ASN1_ETYPE_OBJECT_ID) ||
		  !(p2->type & CONST_ASSIGN))
		return ASN1_IDENTIFIER_NOT_FOUND;
	      else
		_asn1_identifierMissing[0] = 0;
	    }
	}
      else if ((type_field (p->type) == ASN1_ETYPE_OBJECT_ID) &&
	       (p->type & CONST_ASSIGN))
	{
	  p2 = p->down;
	  if (p2 && (type_field (p2->type) == ASN1_ETYPE_CONSTANT))
	    {
	      if (p2->value && !isdigit (p2->value[0]))
		{
		  _asn1_str_cpy (name2, sizeof (name2), node->name);
		  _asn1_str_cat (name2, sizeof (name2), ".");
		  _asn1_str_cat (name2, sizeof (name2), (char *) p2->value);
		  _asn1_str_cpy (_asn1_identifierMissing, sizeof(_asn1_identifierMissing), (char*)p2->value);

		  p2 = asn1_find_node (node, name2);
		  if (!p2 || (type_field (p2->type) != ASN1_ETYPE_OBJECT_ID)
		      || !(p2->type & CONST_ASSIGN))
		    return ASN1_IDENTIFIER_NOT_FOUND;
		  else
		    _asn1_identifierMissing[0] = 0;
		}
	    }
	}

      if (p->down)
	{
	  p = p->down;
	}
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
	      if (p && p->right)
		{
		  p = p->right;
		  break;
		}
	    }
	}
    }

  return ASN1_SUCCESS;
}


/******************************************************************/
/* Function : _asn1_set_default_tag                               */
/* Description: sets the default IMPLICIT or EXPLICIT property in */
/*   the tagged elements that don't have this declaration.        */
/* Parameters:                                                    */
/*   node: pointer to a DEFINITIONS element.                      */
/* Return:                                                        */
/*   ASN1_ELEMENT_NOT_FOUND if NODE is NULL or not a pointer to   */
/*     a DEFINITIONS element,                                     */
/*   otherwise ASN1_SUCCESS                                       */
/******************************************************************/
int
_asn1_set_default_tag (asn1_node node)
{
  asn1_node p;

  if ((node == NULL) || (type_field (node->type) != ASN1_ETYPE_DEFINITIONS))
    return ASN1_ELEMENT_NOT_FOUND;

  p = node;
  while (p)
    {
      if ((type_field (p->type) == ASN1_ETYPE_TAG) &&
	  !(p->type & CONST_EXPLICIT) && !(p->type & CONST_IMPLICIT))
	{
	  if (node->type & CONST_EXPLICIT)
	    p->type |= CONST_EXPLICIT;
	  else
	    p->type |= CONST_IMPLICIT;
	}

      if (p->down)
	{
	  p = p->down;
	}
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
	      if (p && p->right)
		{
		  p = p->right;
		  break;
		}
	    }
	}
    }

  return ASN1_SUCCESS;
}
