/* recvfrom.c --- wrappers for Windows recvfrom function

   Copyright (C) 2008-2016 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Paolo Bonzini */

#include <config.h>

#define WIN32_LEAN_AND_MEAN
/* Get winsock2.h. */
#include <sys/socket.h>

/* Get set_winsock_errno, FD_TO_SOCKET etc. */
#include "w32sock.h"

#undef recvfrom

ssize_t
rpl_recvfrom (int fd, void *buf, size_t len, int flags, struct sockaddr *from,
              socklen_t *fromlen)
{
  SOCKET sock = FD_TO_SOCKET (fd);

  if (sock == INVALID_SOCKET)
    {
      errno = EBADF;
      return -1;
    }
  else
    {
      int frombufsize = (from != NULL ? *fromlen : 0);
      int r = recvfrom (sock, buf, len, flags, from, fromlen);

      if (r < 0)
        set_winsock_errno ();

      /* Winsock recvfrom() only returns a valid 'from' when the socket is
         connectionless.  POSIX gives a valid 'from' for all types of
         sockets.  */
      else if (from != NULL && *fromlen == frombufsize)
        rpl_getpeername (fd, from, fromlen);

      return r;
    }
}
