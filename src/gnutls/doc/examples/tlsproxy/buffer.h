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

#ifndef __TLSPROXY_BUFFERS_H
#define __TLSPROXY_BUFFERS_H

#include <stdlib.h>
#include <sys/types.h>

typedef struct buffer buffer_t;

buffer_t *bufNew (ssize_t size, ssize_t hwm);
void bufFree (buffer_t * b);
ssize_t bufGetReadSpan (buffer_t * b, void **addr);
ssize_t bufGetWriteSpan (buffer_t * b, void **addr);
void bufDoneRead (buffer_t * b, ssize_t size);
void bufDoneWrite (buffer_t * b, ssize_t size);
int bufIsEmpty (buffer_t * b);
int bufIsFull (buffer_t * b);
int bufIsOverHWM (buffer_t * b);
ssize_t bufGetFree (buffer_t * b);
ssize_t bufGetCount (buffer_t * b);

#endif
