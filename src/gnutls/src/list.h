/*
 * Copyright (C) 2001,2002 Paul Sheer
 *
 * This file is part of GnuTLS.
 *
 * GnuTLS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * GnuTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef GNUTLS_SRC_LIST_H
#define GNUTLS_SRC_LIST_H

/*
    SOAP:
    
    Academics always want to implement hash tables (i.e. dictionaries),
    singly or doubly linked lists, and queues, because ...  well ... they
    know how.
    
    These datatypes are nonsense for the following reasons:
	hash tables: Hash tables are a mapping of some
	    string to some data, where that data is going to
	    be accessed COMPLETELY RANDOMLY. This is what it
	    is for. However it is extremely rare to have a
	    large number of data elements which really are
	    being accessed in a completely random way.

	lists: appending and searching through lists is always
	    slow because these operations search all the way
	    through the list.

	queues: whats the difference between a queue and a list?
	    very little really.

    The system implemented here is a doubly linked list with previous
    search index that can be appended or prepended with no overhead,
    implemented entirely in macros. It is hence as versatile as a
    doubly/singly linked list or queue and blazingly fast. Hence doing
    sequential searches where the next search result is likely to be
    closely indexed to the previous (usual case), is efficient.

    Of course this doesn't mean you should use this as a hash table
    where you REALLY need a hash table.

*/

/********** example usage **********/
/*

#include "list.h"

extern void free (void *x);
extern char *strdup (char *s);

// consider a list of elements containing an `int' and a `char *'
LIST_TYPE_DECLARE (names, char *s; int i;);

// for sorting, to compare elements
static int cm (names **a, names **b)
{
    return strcmp ((*a)->s, (*b)->s);
}

// to free the contents of an element
static void free_item (names *a)
{
    free (a->s);
    a->s = 0;	// say
    a->i = 0;	// say
}

int main (int argc, char **argv)
{
// you can separate these into LIST_TYPE_DECLARE(), LIST_DECLARE() and linit() if needed.
    LIST_DECLARE_INIT (l, names, free_item);
    names *j;

    lappend (l);
    l.tail->s = strdup ("hello");
    l.tail->i = 1;
    lappend (l);
    l.tail->s = strdup ("there");
    l.tail->i = 2;
    lappend (l);
    l.tail->s = strdup ("my");
    l.tail->i = 3;
    lappend (l);
    l.tail->s = strdup ("name");
    l.tail->i = 4;
    lappend (l);
    l.tail->s = strdup ("is");
    l.tail->i = 5;
    lappend (l);
    l.tail->s = strdup ("fred");
    l.tail->i = 6;

    printf ("%ld\n\n", lcount (l));
    lloopforward (l, j, printf ("%d %s\n", j->i, j->s));
    printf ("\n");

    lsort (l, cm);
    lloopforward (l, j, printf ("%d %s\n", j->i, j->s));

    lloopreverse (l, j, if (j->i <= 3) ldeleteinc (l, j););

    printf ("\n");
    lloopforward (l, j, printf ("%d %s\n", j->i, j->s));

    ldeleteall (l);

    printf ("\n");
    lloopforward (l, j, printf ("%d %s\n", j->i, j->s));
    return 0;
}

*/

/* the `search' member points to the last found.
   this speeds up repeated searches on the same list-item,
   the consecutive list-item, or the pre-consecutive list-item.
   this obviates the need for a hash table for 99% of
   cercumstances the time */
struct list {
	long length;
	long item_size;
	struct list_item {
		struct list_item *next;
		struct list_item *prev;
		char data[1];
	} *head, *tail, *search;
	void (*free_func) (struct list_item *);
};

/* declare a list of type `x', also called `x' having members `typelist' */

#define LIST_TYPE_DECLARE(type,typelist)						\
    typedef struct type {								\
	struct type *next;								\
	struct type *prev;								\
	typelist									\
    } type

#define LIST_DECLARE(l,type)								\
    struct {										\
	long length;									\
	long item_size;									\
	type *head, *tail, *search;							\
	void (*free_func) (type *);							\
    } l

#define LIST_DECLARE_INIT(l,type,free)							\
    struct {										\
	long length;									\
	long item_size;									\
	type *head, *tail, *search;							\
	void (*free_func) (type *);							\
    } l = {0, sizeof (type), 0, 0, 0, (void (*) (type *)) free}

#define linit(l,type,free)								\
    {											\
	memset (&(l), 0, sizeof (l));							\
	(l).item_size = sizeof (type);							\
	(l).free_func = free;								\
    }

/* returns a pointer to the data of an item */
#define ldata(i)	((void *) &((i)->data[0]))

/* returns a pointer to the list head */
#define lhead(l)	((l).head)

/* returns a pointer to the list tail */
#define ltail(l)	((l).tail)

#define lnewsearch(l)									\
	(l).search = 0;

/* adds to the beginning of the list */
#define lprepend(l)									\
    {											\
	struct list_item *__t;								\
	__t = (struct list_item *) malloc ((l).item_size);				\
	memset (__t, 0, (l).item_size);							\
	__t->next = (l).head;								\
	if (__t->next)									\
	    __t->next->prev = __t;							\
	__t->prev = 0;									\
	if (!(l).tail)									\
	    (l).tail = __t;								\
	(l).head = __t;									\
	length++;									\
    }

/* adds to the end of the list */
#define lappend(l)									\
    {											\
	struct list_item *__t;								\
	__t = (struct list_item *) malloc ((l).item_size);				\
	memset (__t, 0, (l).item_size);							\
	__t->prev = (struct list_item *) (l).tail;					\
	if (__t->prev)									\
	    __t->prev->next = __t;							\
	__t->next = 0;									\
	if (!(l).head)									\
	    (l).head = (void *) __t;							\
	(l).tail = (void *) __t;							\
	(l).length++;									\
    }

/* you should free these manually */
#define lunlink(l,e)									\
    {											\
	struct list_item *__t;								\
	(l).search = 0;									\
	__t = (void *) e;								\
	if ((void *) (l).head == (void *) __t)						\
	    (l).head = (l).head->next;							\
	if ((void *) (l).tail == (void *) __t)						\
	    (l).tail = (l).tail->prev;							\
	if (__t->next)									\
	    __t->next->prev = __t->prev;						\
	if (__t->prev)									\
	    __t->prev->next = __t->next;						\
	(l).length--;									\
    }

/* deletes list entry at point e, and increments e to the following list entry */
#define ldeleteinc(l,e)									\
    {											\
	struct list_item *__t;								\
	(l).search = 0;									\
	__t = (void *) e;								\
	if ((void *) (l).head == (void *) __t)						\
	    (l).head = (l).head->next;							\
	if ((void *) (l).tail == (void *) __t)						\
	    (l).tail = (l).tail->prev;							\
	if (__t->next)									\
	    __t->next->prev = __t->prev;						\
	if (__t->prev)									\
	    __t->prev->next = __t->next;						\
	__t = __t->next;								\
	if ((l).free_func)								\
	    (*(l).free_func) ((void *) e);						\
	free (e);									\
	e = (void *) __t;								\
	(l).length--;									\
    }

/* deletes list entry at point e, and deccrements e to the preceding list emtry */
#define ldeletedec(l,e)									\
    {											\
	struct list_item *__t;								\
	(l).search = 0;									\
	__t = (void *) e;								\
	if ((void *) (l).head == (void *) __t)						\
	    (l).head = (l).head->next;							\
	if ((void *) (l).tail == (void *) __t)						\
	    (l).tail = (l).tail->prev;							\
	if (__t->next)									\
	    __t->next->prev = __t->prev;						\
	if (__t->prev)									\
	    __t->prev->next = __t->next;						\
	__t = __t->prev;								\
	if ((l).free_func)								\
	    (*(l).free_func) ((void *) e);						\
	free (e);									\
	e = (void *) __t;								\
	(l).length--;									\
    }

/* p and q must be consecutive and neither must be zero */
#define linsert(l,p,q)									\
    {											\
	struct list_item *__t;								\
	__t = (struct list_item *) malloc ((l).item_size);				\
	memset (__t, 0, (l).item_size);							\
	__t->prev = (void *) p;								\
	__t->next = (void *) q;								\
	q->prev = (void *) __t;								\
	p->next = (void *) __t;								\
	(l).length++;									\
    }

/* p and q must be consecutive and neither must be zero */
#define ldeleteall(l)									\
    {											\
	(l).search = 0;									\
	while ((l).head) {								\
	    struct list_item *__p;							\
	    __p = (struct list_item *) (l).head;					\
	    lunlink(l, __p);								\
	    if ((l).free_func)								\
		(*(l).free_func) ((void *) __p);					\
	    free (__p);									\
	}										\
    }

#define lloopstart(l,i)									\
	for (i = (void *) (l).head; i;) {						\
	    struct list_item *__tl;							\
	    __tl = (void *) i->next;							\

#define lloopend(l,i)									\
	    i = (void *) __tl;								\
	}										\

#define lloopforward(l,i,op)								\
    {											\
	for (i = (void *) (l).head; i;) {						\
	    struct list_item *__t;							\
	    __t = (void *) i->next;							\
	    op;										\
	    i = (void *) __t;								\
	}										\
    }

#define lloopreverse(l,i,op)								\
    {											\
	for (i = (void *) (l).tail; i;) {						\
	    struct list_item *__t;							\
	    __t = (void *) i->prev;							\
	    op;										\
	    i = (void *) __t;								\
	}										\
    }

#define lindex(l,i,n)									\
    {											\
	int __k;									\
	for (__k = 0, i = (void *) (l).head; i && __k != n; i = i->next, __k++);	\
    }

#define lsearchforward(l,i,op)								\
    {											\
	int __found = 0;								\
	if (!__found)									\
	    if ((i = (void *) (l).search)) {						\
		if (op) {								\
		    __found = 1;							\
		    (l).search = (void *) i;						\
		}									\
		if (!__found)								\
		    if ((i = (void *) (l).search->next))				\
			if (op) {							\
			    __found = 1;						\
			    (l).search = (void *) i;					\
			}								\
		if (!__found)								\
		    if ((i = (void *) (l).search->prev))				\
			if (op) {							\
			    __found = 1;						\
			    (l).search = (void *) i;					\
			}								\
	    }										\
	if (!__found)									\
	    for (i = (void *) (l).head; i; i = i->next)					\
		if (op) {								\
		    __found = 1;							\
		    (l).search = (void *) i;						\
		    break;								\
		}									\
    }

#define lsearchreverse(l,i,op)								\
    {											\
	int __found = 0;								\
	if (!__found)									\
	    if ((i = (void *) (l).search)) {						\
		if (op) {								\
		    __found = 1;							\
		    (l).search = (void *) i;						\
		}									\
		if (!__found)								\
		    if ((i = (void *) (l).search->prev))				\
			if (op) {							\
			    __found = 1;						\
			    (l).search = (void *) i;					\
			}								\
		if (!__found)								\
		    if ((i = (void *) (l).search->next))				\
			if (op) {							\
			    __found = 1;						\
			    (l).search = (void *) i;					\
			}								\
	    }										\
	if (!__found)									\
	    for (i = (void *) (l).tail; i; i = i->prev)					\
		if (op) {								\
		    __found = 1;							\
		    (l).search = (void *) i;						\
		    break;								\
		}									\
    }

#define lcount(l)	((l).length)

/* sort with comparison function see qsort(3) */
#define larray(l,a)									\
    {											\
	long __i;									\
	struct list_item *__p;								\
	a = (void *) malloc (((l).length + 1) * sizeof (void *));			\
	for (__i = 0, __p = (void *) (l).head; __p; __p = __p->next, __i++)		\
	    a[__i] = (void *) __p;							\
	a[__i] = 0;									\
    }											\

/* sort with comparison function see qsort(3) */
#define llist(l,a)									\
    {											\
	struct list_item *__p;								\
	(l).head = (void *) a[0];							\
	(l).search = 0;									\
	__p = (void *) a[0];								\
	__p->prev = 0;									\
	for (__j = 1; a[__j]; __j++, __p = __p->next) {					\
	    __p->next = (void *) a[__j];						\
	    __p->next->prev = __p;							\
	}										\
	(l).tail = (void *) __p;							\
	__p->next = 0;									\
    }											\

/* sort with comparison function see qsort(3) */
#define lsort(l,compare)								\
    {											\
	void **__t;									\
	long __j;									\
	larray (l,__t);									\
	qsort (__t, (l).length, sizeof (void *), (int (*) (const void *, const void *)) compare);	\
	llist (l,__t);									\
	free (__t);									\
    }											\

#endif /* GNUTLS_SRC_LIST_H */
