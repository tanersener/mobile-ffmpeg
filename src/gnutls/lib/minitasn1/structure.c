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


/*****************************************************/
/* File: structure.c                                 */
/* Description: Functions to create and delete an    */
/*  ASN1 tree.                                       */
/*****************************************************/


#include <int.h>
#include <structure.h>
#include "parser_aux.h"
#include <gstr.h>


extern char _asn1_identifierMissing[];


/******************************************************/
/* Function : _asn1_add_single_node                     */
/* Description: creates a new NODE_ASN element.       */
/* Parameters:                                        */
/*   type: type of the new element (see ASN1_ETYPE_         */
/*         and CONST_ constants).                     */
/* Return: pointer to the new element.                */
/******************************************************/
asn1_node
_asn1_add_single_node (unsigned int type)
{
  asn1_node punt;

  punt = calloc (1, sizeof (struct asn1_node_st));
  if (punt == NULL)
    return NULL;

  punt->type = type;

  return punt;
}


/******************************************************************/
/* Function : _asn1_find_left                                     */
/* Description: returns the NODE_ASN element with RIGHT field that*/
/*              points the element NODE.                          */
/* Parameters:                                                    */
/*   node: NODE_ASN element pointer.                              */
/* Return: NULL if not found.                                     */
/******************************************************************/
asn1_node
_asn1_find_left (asn1_node node)
{
  if ((node == NULL) || (node->left == NULL) || (node->left->down == node))
    return NULL;

  return node->left;
}


int
_asn1_create_static_structure (asn1_node pointer, char *output_file_name,
			       char *vector_name)
{
  FILE *file;
  asn1_node p;
  unsigned long t;

  file = fopen (output_file_name, "w");

  if (file == NULL)
    return ASN1_FILE_NOT_FOUND;

  fprintf (file, "#if HAVE_CONFIG_H\n");
  fprintf (file, "# include \"config.h\"\n");
  fprintf (file, "#endif\n\n");

  fprintf (file, "#include <libtasn1.h>\n\n");

  fprintf (file, "const asn1_static_node %s[] = {\n", vector_name);

  p = pointer;

  while (p)
    {
      fprintf (file, "  { ");

      if (p->name[0] != 0)
	fprintf (file, "\"%s\", ", p->name);
      else
	fprintf (file, "NULL, ");

      t = p->type;
      if (p->down)
	t |= CONST_DOWN;
      if (p->right)
	t |= CONST_RIGHT;

      fprintf (file, "%lu, ", t);

      if (p->value)
	fprintf (file, "\"%s\"},\n", p->value);
      else
	fprintf (file, "NULL },\n");

      if (p->down)
	{
	  p = p->down;
	}
      else if (p->right)
	{
	  p = p->right;
	}
      else
	{
	  while (1)
	    {
	      p = _asn1_find_up (p);
	      if (p == pointer)
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

  fprintf (file, "  { NULL, 0, NULL }\n};\n");

  fclose (file);

  return ASN1_SUCCESS;
}


/**
 * asn1_array2tree:
 * @array: specify the array that contains ASN.1 declarations
 * @definitions: return the pointer to the structure created by
 *   *ARRAY ASN.1 declarations
 * @errorDescription: return the error description.
 *
 * Creates the structures needed to manage the ASN.1 definitions.
 * @array is a vector created by asn1_parser2array().
 *
 * Returns: %ASN1_SUCCESS if structure was created correctly,
 *   %ASN1_ELEMENT_NOT_EMPTY if *@definitions not NULL,
 *   %ASN1_IDENTIFIER_NOT_FOUND if in the file there is an identifier
 *   that is not defined (see @errorDescription for more information),
 *   %ASN1_ARRAY_ERROR if the array pointed by @array is wrong.
 **/
int
asn1_array2tree (const asn1_static_node * array, asn1_node * definitions,
		 char *errorDescription)
{
  asn1_node p, p_last = NULL;
  unsigned long k;
  int move;
  int result;
  unsigned int type;

  if (errorDescription)
    errorDescription[0] = 0;

  if (*definitions != NULL)
    return ASN1_ELEMENT_NOT_EMPTY;

  move = UP;

  k = 0;
  while (array[k].value || array[k].type || array[k].name)
    {
      type = convert_old_type (array[k].type);

      p = _asn1_add_static_node (type & (~CONST_DOWN));
      if (array[k].name)
	_asn1_set_name (p, array[k].name);
      if (array[k].value)
	_asn1_set_value (p, array[k].value, strlen (array[k].value) + 1);

      if (*definitions == NULL)
	*definitions = p;

      if (move == DOWN)
	_asn1_set_down (p_last, p);
      else if (move == RIGHT)
	_asn1_set_right (p_last, p);

      p_last = p;

      if (type & CONST_DOWN)
	move = DOWN;
      else if (type & CONST_RIGHT)
	move = RIGHT;
      else
	{
	  while (1)
	    {
	      if (p_last == *definitions)
		break;

	      p_last = _asn1_find_up (p_last);

	      if (p_last == NULL)
		break;

	      if (p_last->type & CONST_RIGHT)
		{
		  p_last->type &= ~CONST_RIGHT;
		  move = RIGHT;
		  break;
		}
	    }			/* while */
	}
      k++;
    }				/* while */

  if (p_last == *definitions)
    {
      result = _asn1_check_identifier (*definitions);
      if (result == ASN1_SUCCESS)
	{
	  _asn1_change_integer_value (*definitions);
	  _asn1_expand_object_id (*definitions);
	}
    }
  else
    {
      result = ASN1_ARRAY_ERROR;
    }

  if (errorDescription != NULL)
    {
      if (result == ASN1_IDENTIFIER_NOT_FOUND)
	{
	  Estrcpy (errorDescription, ":: identifier '");
	  Estrcat (errorDescription, _asn1_identifierMissing);
	  Estrcat (errorDescription, "' not found");
	}
      else
	errorDescription[0] = 0;
    }

  if (result != ASN1_SUCCESS)
    {
      _asn1_delete_list_and_nodes ();
      *definitions = NULL;
    }
  else
    _asn1_delete_list ();

  return result;
}

/**
 * asn1_delete_structure:
 * @structure: pointer to the structure that you want to delete.
 *
 * Deletes the structure *@structure.  At the end, *@structure is set
 * to NULL.
 *
 * Returns: %ASN1_SUCCESS if successful, %ASN1_ELEMENT_NOT_FOUND if
 *   *@structure was NULL.
 **/
int
asn1_delete_structure (asn1_node * structure)
{
  return asn1_delete_structure2(structure, 0);
}

/**
 * asn1_delete_structure2:
 * @structure: pointer to the structure that you want to delete.
 * @flags: additional flags (see %ASN1_DELETE_FLAG)
 *
 * Deletes the structure *@structure.  At the end, *@structure is set
 * to NULL.
 *
 * Returns: %ASN1_SUCCESS if successful, %ASN1_ELEMENT_NOT_FOUND if
 *   *@structure was NULL.
 **/
int
asn1_delete_structure2 (asn1_node * structure, unsigned int flags)
{
  asn1_node p, p2, p3;

  if (*structure == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = *structure;
  while (p)
    {
      if (p->down)
	{
	  p = p->down;
	}
      else
	{			/* no down */
	  p2 = p->right;
	  if (p != *structure)
	    {
	      p3 = _asn1_find_up (p);
	      _asn1_set_down (p3, p2);
	      _asn1_remove_node (p, flags);
	      p = p3;
	    }
	  else
	    {			/* p==root */
	      p3 = _asn1_find_left (p);
	      if (!p3)
		{
		  p3 = _asn1_find_up (p);
		  if (p3)
		    _asn1_set_down (p3, p2);
		  else
		    {
		      if (p->right)
			p->right->left = NULL;
		    }
		}
	      else
		_asn1_set_right (p3, p2);
	      _asn1_remove_node (p, flags);
	      p = NULL;
	    }
	}
    }

  *structure = NULL;
  return ASN1_SUCCESS;
}



/**
 * asn1_delete_element:
 * @structure: pointer to the structure that contains the element you
 *   want to delete.
 * @element_name: element's name you want to delete.
 *
 * Deletes the element named *@element_name inside *@structure.
 *
 * Returns: %ASN1_SUCCESS if successful, %ASN1_ELEMENT_NOT_FOUND if
 *   the @element_name was not found.
 **/
int
asn1_delete_element (asn1_node structure, const char *element_name)
{
  asn1_node p2, p3, source_node;

  source_node = asn1_find_node (structure, element_name);

  if (source_node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p2 = source_node->right;
  p3 = _asn1_find_left (source_node);
  if (!p3)
    {
      p3 = _asn1_find_up (source_node);
      if (p3)
	_asn1_set_down (p3, p2);
      else if (source_node->right)
	source_node->right->left = NULL;
    }
  else
    _asn1_set_right (p3, p2);

  return asn1_delete_structure (&source_node);
}

asn1_node
_asn1_copy_structure3 (asn1_node source_node)
{
  asn1_node dest_node, p_s, p_d, p_d_prev;
  int move;

  if (source_node == NULL)
    return NULL;

  dest_node = _asn1_add_single_node (source_node->type);

  p_s = source_node;
  p_d = dest_node;

  move = DOWN;

  do
    {
      if (move != UP)
	{
	  if (p_s->name[0] != 0)
	    _asn1_cpy_name (p_d, p_s);
	  if (p_s->value)
	    _asn1_set_value (p_d, p_s->value, p_s->value_len);
	  if (p_s->down)
	    {
	      p_s = p_s->down;
	      p_d_prev = p_d;
	      p_d = _asn1_add_single_node (p_s->type);
	      _asn1_set_down (p_d_prev, p_d);
	      continue;
	    }
	  p_d->start = p_s->start;
	  p_d->end = p_s->end;
	}

      if (p_s == source_node)
	break;

      if (p_s->right)
	{
	  move = RIGHT;
	  p_s = p_s->right;
	  p_d_prev = p_d;
	  p_d = _asn1_add_single_node (p_s->type);
	  _asn1_set_right (p_d_prev, p_d);
	}
      else
	{
	  move = UP;
	  p_s = _asn1_find_up (p_s);
	  p_d = _asn1_find_up (p_d);
	}
    }
  while (p_s != source_node);

  return dest_node;
}


static asn1_node
_asn1_copy_structure2 (asn1_node root, const char *source_name)
{
  asn1_node source_node;

  source_node = asn1_find_node (root, source_name);

  return _asn1_copy_structure3 (source_node);

}


static int
_asn1_type_choice_config (asn1_node node)
{
  asn1_node p, p2, p3, p4;
  int move, tlen;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node;
  move = DOWN;

  while (!((p == node) && (move == UP)))
    {
      if (move != UP)
	{
	  if ((type_field (p->type) == ASN1_ETYPE_CHOICE)
	      && (p->type & CONST_TAG))
	    {
	      p2 = p->down;
	      while (p2)
		{
		  if (type_field (p2->type) != ASN1_ETYPE_TAG)
		    {
		      p2->type |= CONST_TAG;
		      p3 = _asn1_find_left (p2);
		      while (p3)
			{
			  if (type_field (p3->type) == ASN1_ETYPE_TAG)
			    {
			      p4 = _asn1_add_single_node (p3->type);
			      tlen = _asn1_strlen (p3->value);
			      if (tlen > 0)
				_asn1_set_value (p4, p3->value, tlen + 1);
			      _asn1_set_right (p4, p2->down);
			      _asn1_set_down (p2, p4);
			    }
			  p3 = _asn1_find_left (p3);
			}
		    }
		  p2 = p2->right;
		}
	      p->type &= ~(CONST_TAG);
	      p2 = p->down;
	      while (p2)
		{
		  p3 = p2->right;
		  if (type_field (p2->type) == ASN1_ETYPE_TAG)
		    asn1_delete_structure (&p2);
		  p2 = p3;
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
	  if (p->right)
	    p = p->right;
	  else
	    move = UP;
	}
      if (move == UP)
	p = _asn1_find_up (p);
    }

  return ASN1_SUCCESS;
}


static int
_asn1_expand_identifier (asn1_node * node, asn1_node root)
{
  asn1_node p, p2, p3;
  char name2[ASN1_MAX_NAME_SIZE + 2];
  int move;

  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = *node;
  move = DOWN;

  while (!((p == *node) && (move == UP)))
    {
      if (move != UP)
	{
	  if (type_field (p->type) == ASN1_ETYPE_IDENTIFIER)
	    {
	      snprintf (name2, sizeof (name2), "%s.%s", root->name, p->value);
	      p2 = _asn1_copy_structure2 (root, name2);
	      if (p2 == NULL)
		{
		  return ASN1_IDENTIFIER_NOT_FOUND;
		}
	      _asn1_cpy_name (p2, p);
	      p2->right = p->right;
	      p2->left = p->left;
	      if (p->right)
		p->right->left = p2;
	      p3 = p->down;
	      if (p3)
		{
		  while (p3->right)
		    p3 = p3->right;
		  _asn1_set_right (p3, p2->down);
		  _asn1_set_down (p2, p->down);
		}

	      p3 = _asn1_find_left (p);
	      if (p3)
		_asn1_set_right (p3, p2);
	      else
		{
		  p3 = _asn1_find_up (p);
		  if (p3)
		    _asn1_set_down (p3, p2);
		  else
		    {
		      p2->left = NULL;
		    }
		}

	      if (p->type & CONST_SIZE)
		p2->type |= CONST_SIZE;
	      if (p->type & CONST_TAG)
		p2->type |= CONST_TAG;
	      if (p->type & CONST_OPTION)
		p2->type |= CONST_OPTION;
	      if (p->type & CONST_DEFAULT)
		p2->type |= CONST_DEFAULT;
	      if (p->type & CONST_SET)
		p2->type |= CONST_SET;
	      if (p->type & CONST_NOT_USED)
		p2->type |= CONST_NOT_USED;

	      if (p == *node)
		*node = p2;
	      _asn1_remove_node (p, 0);
	      p = p2;
	      move = DOWN;
	      continue;
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

      if (p == *node)
	{
	  move = UP;
	  continue;
	}

      if (move == RIGHT)
	{
	  if (p->right)
	    p = p->right;
	  else
	    move = UP;
	}
      if (move == UP)
	p = _asn1_find_up (p);
    }

  return ASN1_SUCCESS;
}


/**
 * asn1_create_element:
 * @definitions: pointer to the structure returned by "parser_asn1" function
 * @source_name: the name of the type of the new structure (must be
 *   inside p_structure).
 * @element: pointer to the structure created.
 *
 * Creates a structure of type @source_name.  Example using
 *  "pkix.asn":
 *
 * rc = asn1_create_element(cert_def, "PKIX1.Certificate", certptr);
 *
 * Returns: %ASN1_SUCCESS if creation OK, %ASN1_ELEMENT_NOT_FOUND if
 *   @source_name is not known.
 **/
int
asn1_create_element (asn1_node definitions, const char *source_name,
		     asn1_node * element)
{
  asn1_node dest_node;
  int res;

  dest_node = _asn1_copy_structure2 (definitions, source_name);

  if (dest_node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  _asn1_set_name (dest_node, "");

  res = _asn1_expand_identifier (&dest_node, definitions);
  _asn1_type_choice_config (dest_node);

  *element = dest_node;

  return res;
}


/**
 * asn1_print_structure:
 * @out: pointer to the output file (e.g. stdout).
 * @structure: pointer to the structure that you want to visit.
 * @name: an element of the structure
 * @mode: specify how much of the structure to print, can be
 *   %ASN1_PRINT_NAME, %ASN1_PRINT_NAME_TYPE,
 *   %ASN1_PRINT_NAME_TYPE_VALUE, or %ASN1_PRINT_ALL.
 *
 * Prints on the @out file descriptor the structure's tree starting
 * from the @name element inside the structure @structure.
 **/
void
asn1_print_structure (FILE * out, asn1_node structure, const char *name,
		      int mode)
{
  asn1_node p, root;
  int k, indent = 0, len, len2, len3;

  if (out == NULL)
    return;

  root = asn1_find_node (structure, name);

  if (root == NULL)
    return;

  p = root;
  while (p)
    {
      if (mode == ASN1_PRINT_ALL)
	{
	  for (k = 0; k < indent; k++)
	    fprintf (out, " ");
	  fprintf (out, "name:");
	  if (p->name[0] != 0)
	    fprintf (out, "%s  ", p->name);
	  else
	    fprintf (out, "NULL  ");
	}
      else
	{
	  switch (type_field (p->type))
	    {
	    case ASN1_ETYPE_CONSTANT:
	    case ASN1_ETYPE_TAG:
	    case ASN1_ETYPE_SIZE:
	      break;
	    default:
	      for (k = 0; k < indent; k++)
		fprintf (out, " ");
	      fprintf (out, "name:");
	      if (p->name[0] != 0)
		fprintf (out, "%s  ", p->name);
	      else
		fprintf (out, "NULL  ");
	    }
	}

      if (mode != ASN1_PRINT_NAME)
	{
	  unsigned type = type_field (p->type);
	  switch (type)
	    {
	    case ASN1_ETYPE_CONSTANT:
	      if (mode == ASN1_PRINT_ALL)
		fprintf (out, "type:CONST");
	      break;
	    case ASN1_ETYPE_TAG:
	      if (mode == ASN1_PRINT_ALL)
		fprintf (out, "type:TAG");
	      break;
	    case ASN1_ETYPE_SIZE:
	      if (mode == ASN1_PRINT_ALL)
		fprintf (out, "type:SIZE");
	      break;
	    case ASN1_ETYPE_DEFAULT:
	      fprintf (out, "type:DEFAULT");
	      break;
	    case ASN1_ETYPE_IDENTIFIER:
	      fprintf (out, "type:IDENTIFIER");
	      break;
	    case ASN1_ETYPE_ANY:
	      fprintf (out, "type:ANY");
	      break;
	    case ASN1_ETYPE_CHOICE:
	      fprintf (out, "type:CHOICE");
	      break;
	    case ASN1_ETYPE_DEFINITIONS:
	      fprintf (out, "type:DEFINITIONS");
	      break;
	    CASE_HANDLED_ETYPES:
	      fprintf (out, "%s", _asn1_tags[type].desc);
	      break;
	    default:
	      break;
	    }
	}

      if ((mode == ASN1_PRINT_NAME_TYPE_VALUE) || (mode == ASN1_PRINT_ALL))
	{
	  switch (type_field (p->type))
	    {
	    case ASN1_ETYPE_CONSTANT:
	      if (mode == ASN1_PRINT_ALL)
		if (p->value)
		  fprintf (out, "  value:%s", p->value);
	      break;
	    case ASN1_ETYPE_TAG:
	      if (mode == ASN1_PRINT_ALL)
		if (p->value)
		  fprintf (out, "  value:%s", p->value);
	      break;
	    case ASN1_ETYPE_SIZE:
	      if (mode == ASN1_PRINT_ALL)
		if (p->value)
		  fprintf (out, "  value:%s", p->value);
	      break;
	    case ASN1_ETYPE_DEFAULT:
	      if (p->value)
		fprintf (out, "  value:%s", p->value);
	      else if (p->type & CONST_TRUE)
		fprintf (out, "  value:TRUE");
	      else if (p->type & CONST_FALSE)
		fprintf (out, "  value:FALSE");
	      break;
	    case ASN1_ETYPE_IDENTIFIER:
	      if (p->value)
		fprintf (out, "  value:%s", p->value);
	      break;
	    case ASN1_ETYPE_INTEGER:
	      if (p->value)
		{
		  len2 = -1;
		  len = asn1_get_length_der (p->value, p->value_len, &len2);
		  fprintf (out, "  value:0x");
		  if (len > 0)
		    for (k = 0; k < len; k++)
		      fprintf (out, "%02x", (unsigned) (p->value)[k + len2]);
		}
	      break;
	    case ASN1_ETYPE_ENUMERATED:
	      if (p->value)
		{
		  len2 = -1;
		  len = asn1_get_length_der (p->value, p->value_len, &len2);
		  fprintf (out, "  value:0x");
		  if (len > 0)
		    for (k = 0; k < len; k++)
		      fprintf (out, "%02x", (unsigned) (p->value)[k + len2]);
		}
	      break;
	    case ASN1_ETYPE_BOOLEAN:
	      if (p->value)
		{
		  if (p->value[0] == 'T')
		    fprintf (out, "  value:TRUE");
		  else if (p->value[0] == 'F')
		    fprintf (out, "  value:FALSE");
		}
	      break;
	    case ASN1_ETYPE_BIT_STRING:
	      if (p->value)
		{
		  len2 = -1;
		  len = asn1_get_length_der (p->value, p->value_len, &len2);
		  if (len > 0)
		    {
		      fprintf (out, "  value(%i):",
			       (len - 1) * 8 - (p->value[len2]));
		      for (k = 1; k < len; k++)
			fprintf (out, "%02x", (unsigned) (p->value)[k + len2]);
		    }
		}
	      break;
	    case ASN1_ETYPE_GENERALIZED_TIME:
	    case ASN1_ETYPE_UTC_TIME:
	      if (p->value)
		{
		  fprintf (out, "  value:");
		  for (k = 0; k < p->value_len; k++)
		    fprintf (out, "%c", (p->value)[k]);
		}
	      break;
	    case ASN1_ETYPE_GENERALSTRING:
	    case ASN1_ETYPE_NUMERIC_STRING:
	    case ASN1_ETYPE_IA5_STRING:
	    case ASN1_ETYPE_TELETEX_STRING:
	    case ASN1_ETYPE_PRINTABLE_STRING:
	    case ASN1_ETYPE_UNIVERSAL_STRING:
	    case ASN1_ETYPE_UTF8_STRING:
	    case ASN1_ETYPE_VISIBLE_STRING:
	      if (p->value)
		{
		  len2 = -1;
		  len = asn1_get_length_der (p->value, p->value_len, &len2);
		  fprintf (out, "  value:");
		  if (len > 0)
		    for (k = 0; k < len; k++)
		      fprintf (out, "%c", (p->value)[k + len2]);
		}
	      break;
	    case ASN1_ETYPE_BMP_STRING:
	    case ASN1_ETYPE_OCTET_STRING:
	      if (p->value)
		{
		  len2 = -1;
		  len = asn1_get_length_der (p->value, p->value_len, &len2);
		  fprintf (out, "  value:");
		  if (len > 0)
		    for (k = 0; k < len; k++)
		      fprintf (out, "%02x", (unsigned) (p->value)[k + len2]);
		}
	      break;
	    case ASN1_ETYPE_OBJECT_ID:
	      if (p->value)
		fprintf (out, "  value:%s", p->value);
	      break;
	    case ASN1_ETYPE_ANY:
	      if (p->value)
		{
		  len3 = -1;
		  len2 = asn1_get_length_der (p->value, p->value_len, &len3);
		  fprintf (out, "  value:");
		  if (len2 > 0)
		    for (k = 0; k < len2; k++)
		      fprintf (out, "%02x", (unsigned) (p->value)[k + len3]);
		}
	      break;
	    case ASN1_ETYPE_SET:
	    case ASN1_ETYPE_SET_OF:
	    case ASN1_ETYPE_CHOICE:
	    case ASN1_ETYPE_DEFINITIONS:
	    case ASN1_ETYPE_SEQUENCE_OF:
	    case ASN1_ETYPE_SEQUENCE:
	    case ASN1_ETYPE_NULL:
	      break;
	    default:
	      break;
	    }
	}

      if (mode == ASN1_PRINT_ALL)
	{
	  if (p->type & 0x1FFFFF00)
	    {
	      fprintf (out, "  attr:");
	      if (p->type & CONST_UNIVERSAL)
		fprintf (out, "UNIVERSAL,");
	      if (p->type & CONST_PRIVATE)
		fprintf (out, "PRIVATE,");
	      if (p->type & CONST_APPLICATION)
		fprintf (out, "APPLICATION,");
	      if (p->type & CONST_EXPLICIT)
		fprintf (out, "EXPLICIT,");
	      if (p->type & CONST_IMPLICIT)
		fprintf (out, "IMPLICIT,");
	      if (p->type & CONST_TAG)
		fprintf (out, "TAG,");
	      if (p->type & CONST_DEFAULT)
		fprintf (out, "DEFAULT,");
	      if (p->type & CONST_TRUE)
		fprintf (out, "TRUE,");
	      if (p->type & CONST_FALSE)
		fprintf (out, "FALSE,");
	      if (p->type & CONST_LIST)
		fprintf (out, "LIST,");
	      if (p->type & CONST_MIN_MAX)
		fprintf (out, "MIN_MAX,");
	      if (p->type & CONST_OPTION)
		fprintf (out, "OPTION,");
	      if (p->type & CONST_1_PARAM)
		fprintf (out, "1_PARAM,");
	      if (p->type & CONST_SIZE)
		fprintf (out, "SIZE,");
	      if (p->type & CONST_DEFINED_BY)
		fprintf (out, "DEF_BY,");
	      if (p->type & CONST_GENERALIZED)
		fprintf (out, "GENERALIZED,");
	      if (p->type & CONST_UTC)
		fprintf (out, "UTC,");
	      if (p->type & CONST_SET)
		fprintf (out, "SET,");
	      if (p->type & CONST_NOT_USED)
		fprintf (out, "NOT_USED,");
	      if (p->type & CONST_ASSIGN)
		fprintf (out, "ASSIGNMENT,");
	    }
	}

      if (mode == ASN1_PRINT_ALL)
	{
	  fprintf (out, "\n");
	}
      else
	{
	  switch (type_field (p->type))
	    {
	    case ASN1_ETYPE_CONSTANT:
	    case ASN1_ETYPE_TAG:
	    case ASN1_ETYPE_SIZE:
	      break;
	    default:
	      fprintf (out, "\n");
	    }
	}

      if (p->down)
	{
	  p = p->down;
	  indent += 2;
	}
      else if (p == root)
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
	      if (p == root)
		{
		  p = NULL;
		  break;
		}
	      indent -= 2;
	      if (p->right)
		{
		  p = p->right;
		  break;
		}
	    }
	}
    }
}



/**
 * asn1_number_of_elements:
 * @element: pointer to the root of an ASN1 structure.
 * @name: the name of a sub-structure of ROOT.
 * @num: pointer to an integer where the result will be stored
 *
 * Counts the number of elements of a sub-structure called NAME with
 * names equal to "?1","?2", ...
 *
 * Returns: %ASN1_SUCCESS if successful, %ASN1_ELEMENT_NOT_FOUND if
 *   @name is not known, %ASN1_GENERIC_ERROR if pointer @num is %NULL.
 **/
int
asn1_number_of_elements (asn1_node element, const char *name, int *num)
{
  asn1_node node, p;

  if (num == NULL)
    return ASN1_GENERIC_ERROR;

  *num = 0;

  node = asn1_find_node (element, name);
  if (node == NULL)
    return ASN1_ELEMENT_NOT_FOUND;

  p = node->down;

  while (p)
    {
      if (p->name[0] == '?')
	(*num)++;
      p = p->right;
    }

  return ASN1_SUCCESS;
}


/**
 * asn1_find_structure_from_oid:
 * @definitions: ASN1 definitions
 * @oidValue: value of the OID to search (e.g. "1.2.3.4").
 *
 * Search the structure that is defined just after an OID definition.
 *
 * Returns: %NULL when @oidValue not found, otherwise the pointer to a
 *   constant string that contains the element name defined just after
 *   the OID.
 **/
const char *
asn1_find_structure_from_oid (asn1_node definitions, const char *oidValue)
{
  char name[2 * ASN1_MAX_NAME_SIZE + 1];
  char value[ASN1_MAX_NAME_SIZE];
  asn1_node p;
  int len;
  int result;
  const char *definitionsName;

  if ((definitions == NULL) || (oidValue == NULL))
    return NULL;		/* ASN1_ELEMENT_NOT_FOUND; */

  definitionsName = definitions->name;

  /* search the OBJECT_ID into definitions */
  p = definitions->down;
  while (p)
    {
      if ((type_field (p->type) == ASN1_ETYPE_OBJECT_ID) &&
	  (p->type & CONST_ASSIGN))
	{
          snprintf(name, sizeof(name), "%s.%s", definitionsName, p->name);

	  len = ASN1_MAX_NAME_SIZE;
	  result = asn1_read_value (definitions, name, value, &len);

	  if ((result == ASN1_SUCCESS) && (!strcmp (oidValue, value)))
	    {
	      p = p->right;
	      if (p == NULL)	/* reach the end of ASN1 definitions */
		return NULL;	/* ASN1_ELEMENT_NOT_FOUND; */

	      return p->name;
	    }
	}
      p = p->right;
    }

  return NULL;			/* ASN1_ELEMENT_NOT_FOUND; */
}

/**
 * asn1_copy_node:
 * @dst: Destination asn1 node.
 * @dst_name: Field name in destination node.
 * @src: Source asn1 node.
 * @src_name: Field name in source node.
 *
 * Create a deep copy of a asn1_node variable. That
 * function requires @dst to be expanded using asn1_create_element().
 *
 * Returns: Return %ASN1_SUCCESS on success.
 **/
int
asn1_copy_node (asn1_node dst, const char *dst_name,
		asn1_node src, const char *src_name)
{
  int result;
  asn1_node dst_node;
  void *data = NULL;
  int size = 0;

  result = asn1_der_coding (src, src_name, NULL, &size, NULL);
  if (result != ASN1_MEM_ERROR)
    return result;

  data = malloc (size);
  if (data == NULL)
    return ASN1_MEM_ERROR;

  result = asn1_der_coding (src, src_name, data, &size, NULL);
  if (result != ASN1_SUCCESS)
    {
      free (data);
      return result;
    }

  dst_node = asn1_find_node (dst, dst_name);
  if (dst_node == NULL)
    {
      free (data);
      return ASN1_ELEMENT_NOT_FOUND;
    }

  result = asn1_der_decoding (&dst_node, data, size, NULL);

  free (data);

  return result;
}

/**
 * asn1_dup_node:
 * @src: Source asn1 node.
 * @src_name: Field name in source node.
 *
 * Create a deep copy of a asn1_node variable. This function
 * will return an exact copy of the provided structure.
 *
 * Returns: Return %NULL on failure.
 **/
asn1_node
asn1_dup_node (asn1_node src, const char *src_name)
{
  return _asn1_copy_structure2(src, src_name);
}
