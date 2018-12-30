/*====================================================================*
 -  Copyright (C) 2001 Leptonica.  All rights reserved.
 -
 -  Redistribution and use in source and binary forms, with or without
 -  modification, are permitted provided that the following conditions
 -  are met:
 -  1. Redistributions of source code must retain the above copyright
 -     notice, this list of conditions and the following disclaimer.
 -  2. Redistributions in binary form must reproduce the above
 -     copyright notice, this list of conditions and the following
 -     disclaimer in the documentation and/or other materials
 -     provided with the distribution.
 -
 -  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 -  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 -  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 -  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ANY
 -  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 -  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 -  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 -  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 -  OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 -  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 -  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *====================================================================*/

/*!
 * \file  heap.c
 * <pre>
 *
 *      Create/Destroy L_Heap
 *          L_HEAP         *lheapCreate()
 *          void           *lheapDestroy()
 *
 *      Operations to add/remove to/from the heap
 *          l_int32         lheapAdd()
 *          static l_int32  lheapExtendArray()
 *          void           *lheapRemove()
 *
 *      Heap operations
 *          l_int32         lheapSwapUp()
 *          l_int32         lheapSwapDown()
 *          l_int32         lheapSort()
 *          l_int32         lheapSortStrictOrder()
 *
 *      Accessors
 *          l_int32         lheapGetCount()
 *
 *      Debug output
 *          l_int32         lheapPrint()
 *
 *    The L_Heap is useful to implement a priority queue, that is sorted
 *    on a key in each element of the heap.  The heap is an array
 *    of nearly arbitrary structs, with a l_float32 the first field.
 *    This field is the key on which the heap is sorted.
 *
 *    Internally, we keep track of the heap size, n.  The item at the
 *    root of the heap is at the head of the array.  Items are removed
 *    from the head of the array and added to the end of the array.
 *    When an item is removed from the head, the item at the end
 *    of the array is moved to the head.  When items are either
 *    added or removed, it is usually necessary to swap array items
 *    to restore the heap order.  It is guaranteed that the number
 *    of swaps does not exceed log(n).
 *
 *    --------------------------  N.B.  ------------------------------
 *    The items on the heap (or, equivalently, in the array) are cast
 *    to void*.  Their key is a l_float32, and it is REQUIRED that the
 *    key be the first field in the struct.  That allows us to get the
 *    key by simply dereferencing the struct.  Alternatively, we could
 *    choose (but don't) to pass an application-specific comparison
 *    function into the heap operation functions.
 *    --------------------------  N.B.  ------------------------------
 * </pre>
 */

#include <string.h>
#include "allheaders.h"

static const l_int32  MIN_BUFFER_SIZE = 20;             /* n'importe quoi */
static const l_int32  INITIAL_BUFFER_ARRAYSIZE = 128;   /* n'importe quoi */

#define SWAP_ITEMS(i, j)       { void *tempitem = lh->array[(i)]; \
                                 lh->array[(i)] = lh->array[(j)]; \
                                 lh->array[(j)] = tempitem; }

    /* Static function */
static l_int32 lheapExtendArray(L_HEAP *lh);


/*--------------------------------------------------------------------------*
 *                          L_Heap create/destroy                           *
 *--------------------------------------------------------------------------*/
/*!
 * \brief   lheapCreate()
 *
 * \param[in]    nalloc size of ptr array to be alloc'd 0 for default
 * \param[in]    direction L_SORT_INCREASING, L_SORT_DECREASING
 * \return  lheap, or NULL on error
 */
L_HEAP *
lheapCreate(l_int32  nalloc,
            l_int32  direction)
{
L_HEAP  *lh;

    PROCNAME("lheapCreate");

    if (nalloc < MIN_BUFFER_SIZE)
        nalloc = MIN_BUFFER_SIZE;

        /* Allocate ptr array and initialize counters. */
    if ((lh = (L_HEAP *)LEPT_CALLOC(1, sizeof(L_HEAP))) == NULL)
        return (L_HEAP *)ERROR_PTR("lh not made", procName, NULL);
    if ((lh->array = (void **)LEPT_CALLOC(nalloc, sizeof(void *))) == NULL) {
        lheapDestroy(&lh, FALSE);
        return (L_HEAP *)ERROR_PTR("ptr array not made", procName, NULL);
    }
    lh->nalloc = nalloc;
    lh->n = 0;
    lh->direction = direction;
    return lh;
}


/*!
 * \brief   lheapDestroy()
 *
 * \param[in,out]   plh  to be nulled
 * \param[in]    freeflag TRUE to free each remaining struct in the array
 * \return  void
 *
 * <pre>
 * Notes:
 *      (1) Use freeflag == TRUE when the items in the array can be
 *          simply destroyed using free.  If those items require their
 *          own destroy function, they must be destroyed before
 *          calling this function, and then this function is called
 *          with freeflag == FALSE.
 *      (2) To destroy the lheap, we destroy the ptr array, then
 *          the lheap, and then null the contents of the input ptr.
 * </pre>
 */
void
lheapDestroy(L_HEAP  **plh,
             l_int32   freeflag)
{
l_int32  i;
L_HEAP  *lh;

    PROCNAME("lheapDestroy");

    if (plh == NULL) {
        L_WARNING("ptr address is NULL\n", procName);
        return;
    }
    if ((lh = *plh) == NULL)
        return;

    if (freeflag) {  /* free each struct in the array */
        for (i = 0; i < lh->n; i++)
            LEPT_FREE(lh->array[i]);
    } else if (lh->n > 0) {  /* freeflag == FALSE but elements exist on array */
        L_WARNING("memory leak of %d items in lheap!\n", procName, lh->n);
    }

    if (lh->array)
        LEPT_FREE(lh->array);
    LEPT_FREE(lh);
    *plh = NULL;

    return;
}

/*--------------------------------------------------------------------------*
 *                                  Accessors                               *
 *--------------------------------------------------------------------------*/
/*!
 * \brief   lheapAdd()
 *
 * \param[in]    lh heap
 * \param[in]    item to be added to the tail of the heap
 * \return  0 if OK, 1 on error
 */
l_ok
lheapAdd(L_HEAP  *lh,
         void    *item)
{
    PROCNAME("lheapAdd");

    if (!lh)
        return ERROR_INT("lh not defined", procName, 1);
    if (!item)
        return ERROR_INT("item not defined", procName, 1);

        /* If necessary, expand the allocated array by a factor of 2 */
    if (lh->n >= lh->nalloc)
        lheapExtendArray(lh);

        /* Add the item */
    lh->array[lh->n] = item;
    lh->n++;

        /* Restore the heap */
    lheapSwapUp(lh, lh->n - 1);
    return 0;
}


/*!
 * \brief   lheapExtendArray()
 *
 * \param[in]    lh heap
 * \return  0 if OK, 1 on error
 */
static l_int32
lheapExtendArray(L_HEAP  *lh)
{
    PROCNAME("lheapExtendArray");

    if (!lh)
        return ERROR_INT("lh not defined", procName, 1);

    if ((lh->array = (void **)reallocNew((void **)&lh->array,
                                sizeof(void *) * lh->nalloc,
                                2 * sizeof(void *) * lh->nalloc)) == NULL)
        return ERROR_INT("new ptr array not returned", procName, 1);

    lh->nalloc = 2 * lh->nalloc;
    return 0;
}


/*!
 * \brief   lheapRemove()
 *
 * \param[in]    lh heap
 * \return  ptr to item popped from the root of the heap,
 *              or NULL if the heap is empty or on error
 */
void *
lheapRemove(L_HEAP  *lh)
{
void   *item;

    PROCNAME("lheapRemove");

    if (!lh)
        return (void *)ERROR_PTR("lh not defined", procName, NULL);

    if (lh->n == 0)
        return NULL;

    item = lh->array[0];
    lh->array[0] = lh->array[lh->n - 1];  /* move last to the head */
    lh->array[lh->n - 1] = NULL;  /* set ptr to null */
    lh->n--;

    lheapSwapDown(lh);  /* restore the heap */
    return item;
}


/*!
 * \brief   lheapGetCount()
 *
 * \param[in]    lh heap
 * \return  count, or 0 on error
 */
l_int32
lheapGetCount(L_HEAP  *lh)
{
    PROCNAME("lheapGetCount");

    if (!lh)
        return ERROR_INT("lh not defined", procName, 0);

    return lh->n;
}



/*--------------------------------------------------------------------------*
 *                               Heap operations                            *
 *--------------------------------------------------------------------------*/
/*!
 * \brief   lheapSwapUp()
 *
 * \param[in]    lh heap
 * \param[in]    index of array corresponding to node to be swapped up
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is called after a new item is put on the heap, at the
 *          bottom of a complete tree.
 *      (2) To regain the heap order, we let it bubble up,
 *          iteratively swapping with its parent, until it either
 *          reaches the root of the heap or it finds a parent that
 *          is in the correct position already vis-a-vis the child.
 * </pre>
 */
l_ok
lheapSwapUp(L_HEAP  *lh,
            l_int32  index)
{
l_int32    ip;  /* index to heap for parent; 1 larger than array index */
l_int32    ic;  /* index into heap for child */
l_float32  valp, valc;

  PROCNAME("lheapSwapUp");

  if (!lh)
      return ERROR_INT("lh not defined", procName, 1);
  if (index < 0 || index >= lh->n)
      return ERROR_INT("invalid index", procName, 1);

  ic = index + 1;  /* index into heap: add 1 to array index */
  if (lh->direction == L_SORT_INCREASING) {
      while (1) {
          if (ic == 1)  /* root of heap */
              break;
          ip = ic / 2;
          valc = *(l_float32 *)(lh->array[ic - 1]);
          valp = *(l_float32 *)(lh->array[ip - 1]);
          if (valp <= valc)
             break;
          SWAP_ITEMS(ip - 1, ic - 1);
          ic = ip;
      }
  } else {  /* lh->direction == L_SORT_DECREASING */
      while (1) {
          if (ic == 1)  /* root of heap */
              break;
          ip = ic / 2;
          valc = *(l_float32 *)(lh->array[ic - 1]);
          valp = *(l_float32 *)(lh->array[ip - 1]);
          if (valp >= valc)
             break;
          SWAP_ITEMS(ip - 1, ic - 1);
          ic = ip;
      }
  }
  return 0;
}


/*!
 * \brief   lheapSwapDown()
 *
 * \param[in]    lh heap
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This is called after an item has been popped off the
 *          root of the heap, and the last item in the heap has
 *          been placed at the root.
 *      (2) To regain the heap order, we let it bubble down,
 *          iteratively swapping with one of its children.  For a
 *          decreasing sort, it swaps with the largest child; for
 *          an increasing sort, the smallest.  This continues until
 *          it either reaches the lowest level in the heap, or the
 *          parent finds that neither child should swap with it
 *          (e.g., for a decreasing heap, the parent is larger
 *          than or equal to both children).
 * </pre>
 */
l_ok
lheapSwapDown(L_HEAP  *lh)
{
l_int32    ip;  /* index to heap for parent; 1 larger than array index */
l_int32    icr, icl;  /* index into heap for left/right children */
l_float32  valp, valcl, valcr;

  PROCNAME("lheapSwapDown");

  if (!lh)
      return ERROR_INT("lh not defined", procName, 1);
  if (lheapGetCount(lh) < 1)
      return 0;

  ip = 1;  /* index into top of heap: corresponds to array[0] */
  if (lh->direction == L_SORT_INCREASING) {
      while (1) {
          icl = 2 * ip;
          if (icl > lh->n)
             break;
          valp = *(l_float32 *)(lh->array[ip - 1]);
          valcl = *(l_float32 *)(lh->array[icl - 1]);
          icr = icl + 1;
          if (icr > lh->n) {  /* only a left child; no iters below */
              if (valp > valcl)
                  SWAP_ITEMS(ip - 1, icl - 1);
              break;
          } else {  /* both children exist; swap with the smallest if bigger */
              valcr = *(l_float32 *)(lh->array[icr - 1]);
              if (valp <= valcl && valp <= valcr)  /* smaller than both */
                  break;
              if (valcl <= valcr) {  /* left smaller; swap */
                  SWAP_ITEMS(ip - 1, icl - 1);
                  ip = icl;
              } else { /* right smaller; swap */
                  SWAP_ITEMS(ip - 1, icr - 1);
                  ip = icr;
              }
          }
      }
  } else {  /* lh->direction == L_SORT_DECREASING */
      while (1) {
          icl = 2 * ip;
          if (icl > lh->n)
             break;
          valp = *(l_float32 *)(lh->array[ip - 1]);
          valcl = *(l_float32 *)(lh->array[icl - 1]);
          icr = icl + 1;
          if (icr > lh->n) {  /* only a left child; no iters below */
              if (valp < valcl)
                  SWAP_ITEMS(ip - 1, icl - 1);
              break;
          } else {  /* both children exist; swap with the biggest if smaller */
              valcr = *(l_float32 *)(lh->array[icr - 1]);
              if (valp >= valcl && valp >= valcr)  /* bigger than both */
                  break;
              if (valcl >= valcr) {  /* left bigger; swap */
                  SWAP_ITEMS(ip - 1, icl - 1);
                  ip = icl;
              } else {  /* right bigger; swap */
                  SWAP_ITEMS(ip - 1, icr - 1);
                  ip = icr;
              }
          }
      }
  }

  return 0;
}


/*!
 * \brief   lheapSort()
 *
 * \param[in]    lh heap, with internal array
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This sorts an array into heap order.  If the heap is already
 *          in heap order for the direction given, this has no effect.
 * </pre>
 */
l_ok
lheapSort(L_HEAP  *lh)
{
l_int32  i;

  PROCNAME("lheapSort");

  if (!lh)
      return ERROR_INT("lh not defined", procName, 1);

  for (i = 0; i < lh->n; i++)
      lheapSwapUp(lh, i);

  return 0;
}


/*!
 * \brief   lheapSortStrictOrder()
 *
 * \param[in]    lh heap, with internal array
 * \return  0 if OK, 1 on error
 *
 * <pre>
 * Notes:
 *      (1) This sorts a heap into strict order.
 *      (2) For each element, starting at the end of the array and
 *          working forward, the element is swapped with the head
 *          element and then allowed to swap down onto a heap of
 *          size reduced by one.  The result is that the heap is
 *          reversed but in strict order.  The array elements are
 *          then reversed to put it in the original order.
 * </pre>
 */
l_ok
lheapSortStrictOrder(L_HEAP  *lh)
{
l_int32  i, index, size;

  PROCNAME("lheapSortStrictOrder");

  if (!lh)
      return ERROR_INT("lh not defined", procName, 1);

  size = lh->n;  /* save the actual size */
  for (i = 0; i < size; i++) {
      index = size - i;
      SWAP_ITEMS(0, index - 1);
      lh->n--;  /* reduce the apparent heap size by 1 */
      lheapSwapDown(lh);
  }
  lh->n = size;  /* restore the size */

  for (i = 0; i < size / 2; i++)  /* reverse */
      SWAP_ITEMS(i, size - i - 1);

  return 0;
}



/*---------------------------------------------------------------------*
 *                            Debug output                             *
 *---------------------------------------------------------------------*/
/*!
 * \brief   lheapPrint()
 *
 * \param[in]    fp file stream
 * \param[in]    lh heap
 * \return  0 if OK; 1 on error
 */
l_ok
lheapPrint(FILE    *fp,
           L_HEAP  *lh)
{
l_int32  i;

    PROCNAME("lheapPrint");

    if (!fp)
        return ERROR_INT("stream not defined", procName, 1);
    if (!lh)
        return ERROR_INT("lh not defined", procName, 1);

    fprintf(fp, "\n L_Heap: nalloc = %d, n = %d, array = %p\n",
            lh->nalloc, lh->n, lh->array);
    for (i = 0; i < lh->n; i++)
        fprintf(fp,   "keyval[%d] = %f\n", i, *(l_float32 *)lh->array[i]);

    return 0;
}
