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

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <gnutls/gnutls.h>
#include <gnutls/crypto.h>
#include <gnutls/x509.h>
#include <gnutls/abstract.h>

#include "crypto-gnutls.h"
#include "buffer.h"

#define FALSE 0
#define TRUE 1

struct tlssession
{
  gnutls_certificate_credentials_t creds;
  gnutls_session_t session;
  char *hostname;
  int (*quitfn) (void *opaque);
  int (*erroutfn) (void *opaque, const char *format, va_list ap);
  int debug;
  void *opaque;
};

#define BUF_SIZE 65536
#define BUF_HWM ((BUF_SIZE*3)/4)

static int
falsequit (void *opaque)
{
  return FALSE;
}

static int
quit (tlssession_t * s)
{
  return s->quitfn (s->opaque);
}

#if defined __clang__ || __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
# pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
#endif

static int stderrout (void *opaque, const char *format, va_list ap)
{
  return vfprintf (stderr, format, ap);
}

static int
errout (tlssession_t * s, const char *format, ...)
{
  va_list ap;
  int ret;
  va_start (ap, format);
  ret = s->erroutfn (s->opaque, format, ap);
  va_end (ap);
  return ret;
}

static int
debugout (tlssession_t * s, const char *format, ...)
{
  va_list ap;
  int ret = 0;
  va_start (ap, format);
  if (s->debug)
    ret = s->erroutfn (s->opaque, format, ap);
  va_end (ap);
  return ret;
}

static int
socksetnonblock (int fd, int nb)
{
  int sf = fcntl (fd, F_GETFL, 0);
  if (sf == -1)
    return -1;
  return fcntl (fd, F_SETFL, nb ? (sf | O_NONBLOCK) : (sf & ~O_NONBLOCK));
}

/* From (public domain) example file in GNUTLS
 *
 * This function will try to verify the peer's certificate, and
 * also check if the hostname matches, and the activation, expiration dates.
 */
static int
verify_certificate_callback (gnutls_session_t session)
{
  unsigned int status;
  int ret;
  tlssession_t *s;

  /* read session pointer */
  s = (tlssession_t *) gnutls_session_get_ptr (session);

  if (gnutls_certificate_type_get (session) != GNUTLS_CRT_X509)
    return GNUTLS_E_CERTIFICATE_ERROR;

  /* This verification function uses the trusted CAs in the credentials
   * structure. So you must have installed one or more CA certificates.
   */
  if (s->hostname && *s->hostname)
    ret = gnutls_certificate_verify_peers3 (session, s->hostname, &status);
  else
    ret = gnutls_certificate_verify_peers2 (session, &status);

  if (ret < 0)
    {
      debugout (s, "Could not verfify peer certificate due to an error\n");
      return GNUTLS_E_CERTIFICATE_ERROR;
    }

  if (status)
    {
      gnutls_datum_t txt;
      ret = gnutls_certificate_verification_status_print(status, GNUTLS_CRT_X509,
							 &txt, 0);
      if (ret >= 0)
        {
          debugout (s, "verification error: %s\n", txt.data);
          gnutls_free(txt.data);
        }

      return GNUTLS_E_CERTIFICATE_ERROR;
    }

  debugout (s, "Peer passed certificate verification\n");

  /* notify gnutls to continue handshake normally */
  return 0;
}

tlssession_t *
tlssession_new (int isserver,
		char *keyfile, char *certfile, char *cacertfile,
		char *hostname, int insecure, int debug,
		int (*quitfn) (void *opaque),
		int (*erroutfn) (void *opaque, const char *format,
				 va_list ap), void *opaque)
{
  int ret;
  tlssession_t *s = calloc (1, sizeof (tlssession_t));
  if (!s)
    return NULL;

  if (quitfn)
    s->quitfn = quitfn;
  else
    s->quitfn = falsequit;

  if (erroutfn)
    s->erroutfn = erroutfn;
  else
    s->erroutfn = stderrout;

  if (hostname)
    s->hostname = strdup (hostname);

  s->debug = debug;

  if (gnutls_certificate_allocate_credentials (&s->creds) < 0)
    {
      errout (s, "Certificate allocation memory error\n");
      goto error;
    }

  if (cacertfile != NULL)
    {
      ret =
	gnutls_certificate_set_x509_trust_file (s->creds, cacertfile,
						GNUTLS_X509_FMT_PEM);
      if (ret < 0)
	{
	  errout (s, "Error setting the x509 trust file: %s\n",
		  gnutls_strerror (ret));
	  goto error;
	}

      if (!insecure)
	{
	  gnutls_certificate_set_verify_function (s->creds,
						  verify_certificate_callback);
	  gnutls_certificate_set_verify_flags (s->creds,
					       GNUTLS_VERIFY_ALLOW_X509_V1_CA_CRT);
	}
    }

  if (keyfile && !certfile)
    certfile = keyfile;

  if (certfile != NULL && keyfile != NULL)
    {
      ret =
	gnutls_certificate_set_x509_key_file (s->creds, certfile, keyfile,
					      GNUTLS_X509_FMT_PEM);

      if (ret < 0)
	{
	  errout (s,
		  "Error loading certificate or key file (%s, %s): %s\n",
		  certfile, keyfile, gnutls_strerror (ret));
	  goto error;
	}
    }

  if (isserver)
    ret = gnutls_init (&s->session, GNUTLS_SERVER);
  else
    ret = gnutls_init (&s->session, GNUTLS_CLIENT);

  if (ret < 0)
    {
      errout (s, "Cannot initialize GNUTLS session: %s\n",
	      gnutls_strerror (ret));
      goto error;
    }

  gnutls_session_set_ptr (s->session, (void *) s);

  if (!isserver && s->hostname && *s->hostname)
    {
      ret = gnutls_server_name_set (s->session, GNUTLS_NAME_DNS, s->hostname,
				    strlen (s->hostname));
      if (ret < 0)
        {
          errout (s, "Cannot set server name: %s\n",
	          gnutls_strerror (ret));
          goto error;
        }
    }

  ret = gnutls_set_default_priority (s->session);
  if (ret < 0)
    {
      errout (s, "Cannot set default GNUTLS session priority: %s\n",
	      gnutls_strerror (ret));
      goto error;
    }

  ret = gnutls_credentials_set (s->session, GNUTLS_CRD_CERTIFICATE, s->creds);
  if (ret < 0)
    {
      errout (s, "Cannot set session GNUTL credentials: %s\n",
	      gnutls_strerror (ret));
      goto error;
    }

  if (isserver)
    {
      /* requests but does not check a client certificate */
      gnutls_certificate_server_set_request (s->session, GNUTLS_CERT_REQUEST);
    }


  return s;

error:
  if (s->session)
    gnutls_deinit (s->session);
  free (s);
  return NULL;
}

void
tlssession_close (tlssession_t * s)
{
  if (s->session)
    gnutls_deinit (s->session);
  free (s->hostname);
  free (s);
}

int
tlssession_init (void)
{
  return gnutls_global_init ();
}


int
tlssession_mainloop (int cryptfd, int plainfd, tlssession_t * s)
{
  fd_set readfds;
  fd_set writefds;
  int maxfd;
  int tls_wr_interrupted = 0;
  int plainEOF = FALSE;
  int cryptEOF = FALSE;
  ssize_t ret;

  buffer_t *plainToCrypt = bufNew (BUF_SIZE, BUF_HWM);
  buffer_t *cryptToPlain = bufNew (BUF_SIZE, BUF_HWM);

  if (socksetnonblock (cryptfd, 0) < 0)
    {
      errout (s, "Could not turn on blocking: %m");
      goto error;
    }

  /* set it up to work with our FD */
  gnutls_transport_set_ptr (s->session,
			    (gnutls_transport_ptr_t) (intptr_t) cryptfd);


  /* Now do the handshake */
  ret = gnutls_handshake (s->session);
  if (ret < 0)
    {
      errout (s, "TLS handshake failed: %s\n", gnutls_strerror (ret));
      goto error;
    }

  if (socksetnonblock (cryptfd, 1) < 0)
    {
      errout (s, "Could not turn on non-blocking on crypt FD: %m");
      goto error;
    }

  if (socksetnonblock (plainfd, 1) < 0)
    {
      errout (s, "Could not turn on non-blocking on plain FD: %m");
      goto error;
    }

  maxfd = (plainfd > cryptfd) ? plainfd + 1 : cryptfd + 1;

  while ((!plainEOF || !cryptEOF) && !quit (s))
    {
      struct timeval timeout;
      int result;
      int selecterrno;
      int wait = TRUE;

      FD_ZERO (&readfds);
      FD_ZERO (&writefds);

      size_t buffered = gnutls_record_check_pending (s->session);
      if (buffered)
	wait = FALSE;		/* do not wait for select to return if we have buffered data */

      if (plainEOF)
	{
	  /* plain text end has closed, but me may still have
	   * data yet to write to the crypt end */
	  if (bufIsEmpty (plainToCrypt) && !tls_wr_interrupted)
	    {
	      cryptEOF = TRUE;
	      break;
	    }
	}
      else
	{
	  if (!bufIsEmpty (cryptToPlain))
	    FD_SET (plainfd, &writefds);
	  if (!bufIsOverHWM (plainToCrypt))
	    FD_SET (plainfd, &readfds);
	}

      if (cryptEOF)
	{
	  /* crypt end has closed, but me way still have data to
	   * write from the crypt buffer */
	  if (bufIsEmpty (cryptToPlain) && !buffered)
	    {
	      plainEOF = TRUE;
	      break;
	    }
	}
      else
	{
	  if (!bufIsEmpty (plainToCrypt) || tls_wr_interrupted)
	    FD_SET (cryptfd, &writefds);
	  if (!bufIsOverHWM (cryptToPlain))
	    FD_SET (cryptfd, &readfds);
	}

      /* Repeat select whilst EINTR happens */
      do
	{
	  timeout.tv_sec = wait ? 1 : 0;
	  timeout.tv_usec = 0;
	  result = select (maxfd, &readfds, &writefds, NULL, &timeout);

	  selecterrno = errno;
	}
      while ((result == -1) && (selecterrno == EINTR) && !quit (s));
      if (quit (s))
	break;

      if (FD_ISSET (plainfd, &readfds))
	{
	  /* we can read at least one byte */
	  void *addr = NULL;
	  /* get a span of characters to write to the
	   * buffer. As the empty portion may wrap the end of the
	   * circular buffer this might not be all we could read.
	   */
	  ssize_t len = bufGetWriteSpan (plainToCrypt, &addr);
	  if (len > 0)
	    {
	      do
		{
		  ret = read (plainfd, addr, (size_t) len);
		}
	      while ((ret < 0) && (errno == EINTR) && !quit (s));
	      if (quit (s))
		break;
	      if (ret < 0)
		{
		  errout (s, "Error on read from plain socket: %m\n");
		  goto error;
		}
	      if (ret == 0)
		{
		  plainEOF = TRUE;
		}
	      else
		{
		  bufDoneWrite (plainToCrypt, ret);	/* mark ret bytes as written to the buffer */
		}
	    }
	}

      if (FD_ISSET (plainfd, &writefds))
	{
	  /* we can write at least one byte */
	  void *addr = NULL;
	  /* get a span of characters to read from the buffer
	   * as the full portion may wrap the end of the circular buffer
	   * this might not be all we have to write.
	   */
	  ssize_t len = bufGetReadSpan (cryptToPlain, &addr);
	  if (len > 0)
	    {
	      do
		{
		  ret = write (plainfd, addr, (size_t) len);
		}
	      while ((ret < 0) && (errno == EINTR) && !quit (s));
	      if (quit (s))
		break;
	      if (ret < 0)
		{
		  errout (s, "Error on write to plain socket: %m\n");
		  goto error;
		}
	      bufDoneRead (cryptToPlain, ret);	/* mark ret bytes as read from the buffer */
	    }
	}

      if (FD_ISSET (cryptfd, &readfds) || buffered)
	{
	  /* we can read at least one byte */
	  void *addr = NULL;
	  /* get a span of characters to write to the
	   * buffer. As the empty portion may wrap the end of the
	   * circular buffer this might not be all we could read.
	   */
	  ssize_t len = bufGetWriteSpan (cryptToPlain, &addr);
	  if (len > 0)
	    {
	      do
		{
		  ret = gnutls_record_recv (s->session, addr, (size_t) len);
		}
	      while (ret == GNUTLS_E_INTERRUPTED && !quit (s));
	      /* do not loop on GNUTLS_E_AGAIN - this means we'd block so we'd loop for
	       * ever
	       */
	      if (quit (s))
		break;
	      if (ret < 0 && ret != GNUTLS_E_AGAIN)
		{
		  errout (s, "Error on read from crypt socket: %s\n",
			  gnutls_strerror (ret));
		  goto error;
		}
	      if (ret == 0)
		{
		  cryptEOF = TRUE;
		}
	      else
		{
		  bufDoneWrite (cryptToPlain, ret);	/* mark ret bytes as written to the buffer */
		}
	    }
	}

      if (FD_ISSET (cryptfd, &writefds))
	{
	  /* we can write at least one byte */
	  void *addr = NULL;
	  /* get a span of characters to read from the buffer
	   * as the full portion may wrap the end of the circular buffer
	   * this might not be all we have to write.
	   */
	  ssize_t len = bufGetReadSpan (plainToCrypt, &addr);
	  if (len > 0)
	    {
	      do
		{
		  if (tls_wr_interrupted)
		    {
		      ret = gnutls_record_send (s->session, NULL, 0);
		    }
		  else
		    {
		      ret = gnutls_record_send (s->session, addr, len);
		    }
		}
	      while (ret == GNUTLS_E_INTERRUPTED && !quit (s));
	      if (quit (s))
		break;
	      if (ret == GNUTLS_E_AGAIN)
		{
		  /* we need to call this again with NULL parameters
		   * as it blocked
		   */
		  tls_wr_interrupted = TRUE;
		}
	      else if (ret < 0)
		{
		  errout (s, "Error on write to crypto socket: %s\n",
			  gnutls_strerror (ret));
		  goto error;
		}
	      else
		{
		  bufDoneRead (plainToCrypt, ret);	/* mark ret bytes as read from the buffer */
		}
	    }
	}
    }

  ret = 0;
  goto freereturn;

error:
  ret = -1;

freereturn:
  gnutls_bye (s->session, GNUTLS_SHUT_RDWR);
  shutdown (plainfd, SHUT_RDWR);
  bufFree (plainToCrypt);
  bufFree (cryptToPlain);
  return ret;
}
