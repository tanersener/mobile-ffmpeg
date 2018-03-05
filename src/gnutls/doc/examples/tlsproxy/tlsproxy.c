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

#define _GNU_SOURCE
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "crypto-gnutls.h"

static char *connectaddr = NULL;
static char *listenaddr = NULL;
static char *keyfile = NULL;
static char *certfile = NULL;
static char *cacertfile = NULL;
static char *hostname = NULL;
static int debug = 0;
static int insecure = 0;
static int nofork = 0;
static int server = 0;

static char *defaultport = "12345";

static volatile sig_atomic_t rxsigquit = 0;

static int
bindtoaddress (char *addrport)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int fd, s;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_flags = AI_PASSIVE;	/* For wildcard IP address */
  hints.ai_family = AF_UNSPEC;	/* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;	/* Stream socket */
  hints.ai_protocol = 0;	/* any protocol */

  char *addr = strdupa (addrport);
  char *colon = strrchr (addr, ':');
  char *port = defaultport;
  if (colon)
    {
      *colon = 0;
      port = colon + 1;
    }

  s = getaddrinfo (addr, port, &hints, &result);
  if (s != 0)
    {
      fprintf (stderr, "Error in address %s: %s\n", addr, gai_strerror (s));
      return -1;
    }

  /* attempt to bind to each address */

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      fd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);

      if (fd >= 0)
	{
	  int one = 1;
	  if (setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof (one)) <
	      0)
	    {
	      close (fd);
	      continue;
	    }
	  if (bind (fd, rp->ai_addr, rp->ai_addrlen) == 0)
	    break;
	  close (fd);
	}
    }

  if (!rp)
    {
      fprintf (stderr, "Error binding to %s:%s: %m\n", addr, port);
      return -1;
    }

  freeaddrinfo (result);	/* No longer needed */

  if (listen (fd, 5) < 0)
    {
      close (fd);
      return -1;
    }

  return fd;
}

static int
connecttoaddress (char *addrport)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;
  int fd, s;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_flags = AI_PASSIVE;	/* For wildcard IP address */
  hints.ai_family = AF_UNSPEC;	/* Allow IPv4 or IPv6 */
  hints.ai_socktype = SOCK_STREAM;	/* Stream socket */
  hints.ai_protocol = 0;	/* any protocol */

  char *addr = strdupa (addrport);
  char *colon = strrchr (addr, ':');
  char *port = defaultport;
  if (colon)
    {
      *colon = 0;
      port = colon + 1;
    }

  if (!hostname && !server)
    hostname = strdup (addr);

  s = getaddrinfo (addr, port, &hints, &result);
  if (s != 0)
    {
      fprintf (stderr, "Error in address %s: %s\n", addr, gai_strerror (s));
      return -1;
    }

  /* attempt to connect to each address */
  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      fd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (fd >= 0)
	{
	  if (connect (fd, rp->ai_addr, rp->ai_addrlen) == 0)
	    break;
	  close (fd);
	}
    }

  if (!rp)
    {
      fprintf (stderr, "Error connecting to %s:%s: %m\n", addr, port);
      return -1;
    }

  freeaddrinfo (result);	/* No longer needed */

  return fd;
}

static int
quitfn (void *opaque)
{
  return rxsigquit;
}

static int
runproxy (int acceptfd)
{
  int connectfd;
  if ((connectfd = connecttoaddress (connectaddr)) < 0)
    {
      fprintf (stderr, "Could not connect\n");
      close (acceptfd);
      return -1;
    }

  tlssession_t *session =
    tlssession_new (server, keyfile, certfile, cacertfile, hostname, insecure,
		    debug, quitfn, NULL, NULL);
  if (!session)
    {
      fprintf (stderr, "Could create TLS session\n");
      close (connectfd);
      close (acceptfd);
      return -1;
    }

  int ret;
  if (server)
    ret = tlssession_mainloop (acceptfd, connectfd, session);
  else
    ret = tlssession_mainloop (connectfd, acceptfd, session);

  tlssession_close (session);
  close (connectfd);
  close (acceptfd);

  if (ret < 0)
    {
      fprintf (stderr, "TLS proxy exited with an error\n");
      return -1;
    }
  return 0;
}

static int
runlistener ()
{
  int listenfd;
  if ((listenfd = bindtoaddress (listenaddr)) < 0)
    {
      fprintf (stderr, "Could not bind listener\n");
      return -1;
    }

  /*
     if (!nofork)
     daemon (FALSE, FALSE);
   */

  int fd;
  while (!rxsigquit)
    {
      do
	{
	  if ((fd = accept (listenfd, NULL, NULL)) < 0)
	    {
	      if (errno != EINTR)
		{
		  fprintf (stderr, "Accept failed\n");
		  return -1;
		}
	    }
	}
      while (fd < 0 && !rxsigquit);
      if (rxsigquit)
	break;
      if (nofork < 2)
	{
	  int ret = runproxy (fd);
	  if (ret < 0)
	    return -1;
	}
      else
	{
	  int cpid = fork ();
	  if (cpid == 0)
	    {
	      /* we're the child */
	      runproxy (fd);
	      exit (0);
	    }
	  else
	    close (fd);
	}
    }
  return 0;
}

static void
usage ()
{
  fprintf (stderr, "tlsproxy\n\n\
Usage:\n\
     tlsproxy [OPTIONS]\n\
\n\
A TLS client or server proxy\n\
\n\
Options:\n\
     -c, --connect ADDRRESS    Connect to ADDRESS\n\
     -l, --listen ADDRESS      Listen on ADDRESS\n\
     -K, --key FILE            Use FILE as private key\n\
     -C, --cert FILE           Use FILE as public key\n\
     -A, --cacert FILE         Use FILE as public CA cert file\n\
     -H, --hostname HOSTNAME   Use HOSTNAME to validate the CN of the peer\n\
                               rather than hostname extracted from -C option\n\
     -s, --server              Run the listen port encrypted rather than the\n\
                               connect port\n\
     -i, --insecure            Do not validate certificates\n\
     -n, --nofork              Do not fork off (aids debugging); specify twice\n\
                               to stop forking on accept as well\n\
     -d, --debug               Turn on debugging\n\
     -h, --help                Show this usage message\n\
\n\
\n");
}

static void
processoptions (int argc, char **argv)
{
  while (1)
    {
      static struct option longopts[] = {
	{"connect", required_argument, 0, 'c'},
	{"listen", required_argument, 0, 'l'},
	{"key", required_argument, 0, 'K'},
	{"cert", required_argument, 0, 'C'},
	{"cacert", required_argument, 0, 'A'},
	{"hostname", required_argument, 0, 'H'},
	{"server", no_argument, 0, 's'},
	{"insecure", no_argument, 0, 'i'},
	{"nofork", no_argument, 0, 'n'},
	{"debug", no_argument, 0, 'd'},
	{"help", no_argument, 0, 'h'},
	{0, 0, 0, 0}
      };

      int optind = 0;

      int c =
	getopt_long (argc, argv, "c:l:K:C:A:H:sindh", longopts, &optind);
      if (c == -1)
	break;

      switch (c)
	{
	case 0:		/* set a flag, nothing else to do */
	  break;

	case 'c':
	  connectaddr = strdup (optarg);
	  break;

	case 'l':
	  listenaddr = strdup (optarg);
	  break;

	case 'K':
	  keyfile = strdup (optarg);
	  break;

	case 'C':
	  certfile = strdup (optarg);
	  break;

	case 'A':
	  cacertfile = strdup (optarg);
	  break;

	case 'H':
	  hostname = strdup (optarg);
	  break;

	case 's':
	  server = 1;
	  break;

	case 'i':
	  insecure = 1;
	  break;

	case 'n':
	  nofork++;
	  break;

	case 'd':
	  debug++;
	  break;

	case 'h':
	  usage ();
	  exit (0);
	  break;

	default:
	  usage ();
	  exit (1);
	}
    }

  if (optind != argc || !connectaddr || !listenaddr)
    {
      usage ();
      exit (1);
    }

  if (!certfile && keyfile)
    certfile = strdup (keyfile);
}

static void
handlesignal (int sig)
{
  switch (sig)
    {
    case SIGINT:
    case SIGTERM:
      rxsigquit++;
      break;
    default:
      break;
    }
}

static void
setsignalmasks ()
{
  struct sigaction sa;
  /* Set up the structure to specify the new action. */
  memset (&sa, 0, sizeof (struct sigaction));
  sa.sa_handler = handlesignal;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);

  memset (&sa, 0, sizeof (struct sigaction));
  sa.sa_handler = SIG_IGN;
  sa.sa_flags = SA_RESTART;
  sigaction (SIGPIPE, &sa, NULL);
}

int
main (int argc, char **argv)
{
  processoptions (argc, argv);

  setsignalmasks ();

  if (tlssession_init ())
    exit (1);

  runlistener ();

  free (connectaddr);
  free (listenaddr);
  free (keyfile);
  free (certfile);
  free (cacertfile);
  free (hostname);

  exit (0);
}
