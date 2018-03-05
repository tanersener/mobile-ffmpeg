/*

The MIT License (MIT)

Copyright (c) 2016 Wrymouth Innovation Ltd

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

*/

#include <sys/types.h>

#include "buffer.h"

typedef struct buffer
{
  char *buf;
  ssize_t size;
  ssize_t hwm;
  ssize_t ridx;
  ssize_t widx;
  int empty;
} buffer_t;

/* the buffer is organised internally as follows:
 *
 * * There are b->size bytes in the buffer.
 *
 * * Bytes are at offsets 0 to b->size-1
 * 
 * * b->ridx points to the first readable byte
 *
 * * b->widx points to the first empty space
 *
 * * b->ridx < b->widx indicates a non-wrapped buffer:
 *
 *     0       ridx     widx            size
 *     |       |        |               |
 *     V       V        V               V
 *     ........XXXXXXXXX................
 *
 * * b->ridx > b->widx indicates a wrapped buffer:
 *
 *     0       widx     ridx            size
 *     |       |        |               |
 *     V       V        V               V
 *     XXXXXXXX.........XXXXXXXXXXXXXXXX
 *
 * * b->ridx == b->widx indicates a FULL buffer:
 *
 * * b->ridx == b->widx indicates a wrapped buffer:
 *
 *     0       widx == ridx            size
 *     |       |                       |
 *     V       V                       V
 *     XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
 *
 * An empty buffer is indicated by empty=1
 *
 */

buffer_t *
bufNew (ssize_t size, ssize_t hwm)
{
  buffer_t *b = calloc (1, sizeof (buffer_t));
  b->buf = calloc (1, size);
  b->size = size;
  b->hwm = hwm;
  b->empty = 1;
  return b;
}


void
bufFree (buffer_t * b)
{
  free (b->buf);
  free (b);
}

/* get a maximal span to read. Returns 0 if buffer
 * is empty
 */
ssize_t
bufGetReadSpan (buffer_t * b, void **addr)
{
  if (b->empty)
    {
      *addr = NULL;
      return 0;
    }
  *addr = &(b->buf[b->ridx]);
  ssize_t len = b->widx - b->ridx;
  if (len <= 0)
    len = b->size - b->ridx;
  return len;
}

/* get a maximal span to write. Returns 0 id buffer is full
 */
ssize_t
bufGetWriteSpan (buffer_t * b, void **addr)
{
  if (b->empty)
    {
      *addr = b->buf;
      b->ridx = 0;
      b->widx = 0;
      return b->size;
    }
  if (b->ridx == b->widx)
    {
      *addr = NULL;
      return 0;
    }
  *addr = &(b->buf[b->widx]);
  ssize_t len = b->ridx - b->widx;
  if (len <= 0)
    len = b->size - b->widx;
  return len;
}

/* mark size bytes as read */
void
bufDoneRead (buffer_t * b, ssize_t size)
{
  while (!b->empty && (size > 0))
    {
      /* empty can't occur here, so equal pointers means full */
      ssize_t len = b->widx - b->ridx;
      if (len <= 0)
	len = b->size - b->ridx;

      /* len is the number of bytes in one read span */
      if (len > size)
	len = size;

      b->ridx += len;
      if (b->ridx >= b->size)
	b->ridx = 0;

      if (b->ridx == b->widx)
	{
	  b->ridx = 0;
	  b->widx = 0;
	  b->empty = 1;
	}

      size -= len;
    }
}

/* mark size bytes as written */
void
bufDoneWrite (buffer_t * b, ssize_t size)
{
  while ((b->empty || (b->ridx != b->widx)) && (size > 0))
    {
      /* full can't occur here, so equal pointers means empty */
      ssize_t len = b->ridx - b->widx;
      if (len <= 0)
	len = b->size - b->widx;

      /* len is the number of bytes in one write span */
      if (len > size)
	len = size;

      b->widx += len;
      if (b->widx >= b->size)
	b->widx = 0;

      /* it can't be empty as we've written at least one byte */
      b->empty = 0;

      size -= len;
    }
}

int
bufIsEmpty (buffer_t * b)
{
  return b->empty;
}

int
bufIsFull (buffer_t * b)
{
  return !b->empty && (b->ridx == b->widx);
}

int
bufIsOverHWM (buffer_t * b)
{
  return bufGetCount (b) > b->hwm;
}

ssize_t
bufGetFree (buffer_t * b)
{
  return b->size - bufGetCount (b);
}

ssize_t
bufGetCount (buffer_t * b)
{
  if (b->empty)
    return 0;
  return b->widx - b->ridx + ((b->ridx < b->widx) ? 0 : b->size);
}
