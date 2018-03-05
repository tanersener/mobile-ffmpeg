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

#ifndef __TLSPROXY_CRYPTO_GNUTLS_H
#define __TLSPROXY_CRYPTO_GNUTLS_H

int tlssession_init ();

typedef struct tlssession tlssession_t;
tlssession_t *tlssession_new (int isserver,
			      char *keyfile, char *certfile, char *cacertfile,
			      char *hostname, int insecure, int debug,
			      int (*quitfn) (void *opaque),
			      int (*erroutfn) (void *opaque,
					       const char *format,
					       va_list ap), void *opaque);
void tlssession_close (tlssession_t * s);
int tlssession_mainloop (int cryptfd, int plainfd, tlssession_t * session);

#endif
