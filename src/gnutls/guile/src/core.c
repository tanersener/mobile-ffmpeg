/* GnuTLS --- Guile bindings for GnuTLS.
   Copyright (C) 2007-2014, 2016 Free Software Foundation, Inc.

   GnuTLS is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   GnuTLS is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with GnuTLS; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA  */

/* Written by Ludovic Courtès <ludo@gnu.org>.  */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <gnutls/gnutls.h>
#include <gnutls/openpgp.h>
#include <libguile.h>

#include <alloca.h>

#include "enums.h"
#include "smobs.h"
#include "errors.h"
#include "utils.h"



/* SMOB and enums type definitions.  */
#include "enum-map.i.c"
#include "smob-types.i.c"

const char scm_gnutls_array_error_message[] =
  "cannot handle non-contiguous array: ~A";


/* Data that are attached to `gnutls_session_t' objects.

   We need to keep several pieces of information along with each session:

     - A boolean indicating whether its underlying transport is a file
       descriptor or Scheme port.  This is used to decide whether to leave
       "Guile mode" when invoking `gnutls_record_recv ()'.

     - The record port attached to the session (returned by
       `session-record-port').  This is so that several calls to
       `session-record-port' return the same port.

   Currently, this information is maintained into a pair.  The whole pair is
   marked by the session mark procedure.  */

#define SCM_GNUTLS_MAKE_SESSION_DATA()		\
  scm_cons (SCM_BOOL_F, SCM_BOOL_F)
#define SCM_GNUTLS_SET_SESSION_DATA(c_session, data)			\
  gnutls_session_set_ptr (c_session, (void *) SCM_UNPACK (data))
#define SCM_GNUTLS_SESSION_DATA(c_session)			\
  SCM_PACK ((scm_t_bits) gnutls_session_get_ptr (c_session))

#define SCM_GNUTLS_SET_SESSION_TRANSPORT_IS_FD(c_session, c_is_fd)	\
  SCM_SETCAR (SCM_GNUTLS_SESSION_DATA (c_session),			\
	      scm_from_bool (c_is_fd))
#define SCM_GNUTLS_SET_SESSION_RECORD_PORT(c_session, port)	\
  SCM_SETCDR (SCM_GNUTLS_SESSION_DATA (c_session), port)

#define SCM_GNUTLS_SESSION_TRANSPORT_IS_FD(c_session)		\
  scm_to_bool (SCM_CAR (SCM_GNUTLS_SESSION_DATA (c_session)))
#define SCM_GNUTLS_SESSION_RECORD_PORT(c_session)	\
  SCM_CDR (SCM_GNUTLS_SESSION_DATA (c_session))


/* Weak-key hash table.  */
static SCM weak_refs;

/* Register a weak reference from @FROM to @TO, such that the lifetime of TO is
   greater than or equal to that of FROM.  */
static void
register_weak_reference (SCM from, SCM to)
{
  scm_hashq_set_x (weak_refs, from, to);
}




/* Bindings.  */

/* Mark the data associated with SESSION.  */
SCM_SMOB_MARK (scm_tc16_gnutls_session, mark_session, session)
{
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, "mark_session");

  return (SCM_GNUTLS_SESSION_DATA (c_session));
}

SCM_DEFINE (scm_gnutls_version, "gnutls-version", 0, 0, 0,
            (void),
            "Return a string denoting the version number of the underlying "
            "GnuTLS library, e.g., @code{\"1.7.2\"}.")
#define FUNC_NAME s_scm_gnutls_version
{
  return (scm_from_locale_string (gnutls_check_version (NULL)));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_make_session, "make-session", 1, 0, 0,
            (SCM end),
            "Return a new session for connection end @var{end}, either "
            "@code{connection-end/server} or @code{connection-end/client}.")
#define FUNC_NAME s_scm_gnutls_make_session
{
  int err;
  gnutls_session_t c_session;
  gnutls_connection_end_t c_end;
  SCM session_data;

  c_end = scm_to_gnutls_connection_end (end, 1, FUNC_NAME);

  session_data = SCM_GNUTLS_MAKE_SESSION_DATA ();
  err = gnutls_init (&c_session, c_end);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  SCM_GNUTLS_SET_SESSION_DATA (c_session, session_data);

  return (scm_from_gnutls_session (c_session));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_bye, "bye", 2, 0, 0,
            (SCM session, SCM how),
            "Close @var{session} according to @var{how}.")
#define FUNC_NAME s_scm_gnutls_bye
{
  int err;
  gnutls_session_t c_session;
  gnutls_close_request_t c_how;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  c_how = scm_to_gnutls_close_request (how, 2, FUNC_NAME);

  err = gnutls_bye (c_session, c_how);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_handshake, "handshake", 1, 0, 0,
            (SCM session), "Perform a handshake for @var{session}.")
#define FUNC_NAME s_scm_gnutls_handshake
{
  int err;
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  err = gnutls_handshake (c_session);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_rehandshake, "rehandshake", 1, 0, 0,
            (SCM session), "Perform a re-handshaking for @var{session}.")
#define FUNC_NAME s_scm_gnutls_rehandshake
{
  int err;
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  err = gnutls_rehandshake (c_session);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_alert_get, "alert-get", 1, 0, 0,
            (SCM session), "Get an aleter from @var{session}.")
#define FUNC_NAME s_scm_gnutls_alert_get
{
  gnutls_session_t c_session;
  gnutls_alert_description_t c_alert;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_alert = gnutls_alert_get (c_session);

  return (scm_from_gnutls_alert_description (c_alert));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_alert_send, "alert-send", 3, 0, 0,
            (SCM session, SCM level, SCM alert),
            "Send @var{alert} via @var{session}.")
#define FUNC_NAME s_scm_gnutls_alert_send
{
  int err;
  gnutls_session_t c_session;
  gnutls_alert_level_t c_level;
  gnutls_alert_description_t c_alert;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  c_level = scm_to_gnutls_alert_level (level, 2, FUNC_NAME);
  c_alert = scm_to_gnutls_alert_description (alert, 3, FUNC_NAME);

  err = gnutls_alert_send (c_session, c_level, c_alert);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

/* FIXME: Omitting `alert-send-appropriate'.  */


/* Session accessors.  */

SCM_DEFINE (scm_gnutls_session_cipher, "session-cipher", 1, 0, 0,
            (SCM session), "Return @var{session}'s cipher.")
#define FUNC_NAME s_scm_gnutls_session_cipher
{
  gnutls_session_t c_session;
  gnutls_cipher_algorithm_t c_cipher;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_cipher = gnutls_cipher_get (c_session);

  return (scm_from_gnutls_cipher (c_cipher));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_kx, "session-kx", 1, 0, 0,
            (SCM session), "Return @var{session}'s kx.")
#define FUNC_NAME s_scm_gnutls_session_kx
{
  gnutls_session_t c_session;
  gnutls_kx_algorithm_t c_kx;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_kx = gnutls_kx_get (c_session);

  return (scm_from_gnutls_kx (c_kx));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_mac, "session-mac", 1, 0, 0,
            (SCM session), "Return @var{session}'s MAC.")
#define FUNC_NAME s_scm_gnutls_session_mac
{
  gnutls_session_t c_session;
  gnutls_mac_algorithm_t c_mac;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_mac = gnutls_mac_get (c_session);

  return (scm_from_gnutls_mac (c_mac));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_compression_method,
            "session-compression-method", 1, 0, 0,
            (SCM session), "Return @var{session}'s compression method.")
#define FUNC_NAME s_scm_gnutls_session_compression_method
{
  gnutls_session_t c_session;
  gnutls_compression_method_t c_comp;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_comp = gnutls_compression_get (c_session);

  return (scm_from_gnutls_compression_method (c_comp));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_certificate_type,
            "session-certificate-type", 1, 0, 0,
            (SCM session), "Return @var{session}'s certificate type.")
#define FUNC_NAME s_scm_gnutls_session_certificate_type
{
  gnutls_session_t c_session;
  gnutls_certificate_type_t c_cert;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_cert = gnutls_certificate_type_get (c_session);

  return (scm_from_gnutls_certificate_type (c_cert));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_protocol, "session-protocol", 1, 0, 0,
            (SCM session), "Return the protocol used by @var{session}.")
#define FUNC_NAME s_scm_gnutls_session_protocol
{
  gnutls_session_t c_session;
  gnutls_protocol_t c_protocol;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_protocol = gnutls_protocol_get_version (c_session);

  return (scm_from_gnutls_protocol (c_protocol));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_authentication_type,
            "session-authentication-type",
            1, 0, 0,
            (SCM session),
            "Return the authentication type (a @code{credential-type} value) "
            "used by @var{session}.")
#define FUNC_NAME s_scm_gnutls_session_authentication_type
{
  gnutls_session_t c_session;
  gnutls_credentials_type_t c_auth;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_auth = gnutls_auth_get_type (c_session);

  return (scm_from_gnutls_credentials (c_auth));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_server_authentication_type,
            "session-server-authentication-type",
            1, 0, 0,
            (SCM session),
            "Return the server authentication type (a "
            "@code{credential-type} value) used in @var{session}.")
#define FUNC_NAME s_scm_gnutls_session_server_authentication_type
{
  gnutls_session_t c_session;
  gnutls_credentials_type_t c_auth;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_auth = gnutls_auth_server_get_type (c_session);

  return (scm_from_gnutls_credentials (c_auth));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_client_authentication_type,
            "session-client-authentication-type",
            1, 0, 0,
            (SCM session),
            "Return the client authentication type (a "
            "@code{credential-type} value) used in @var{session}.")
#define FUNC_NAME s_scm_gnutls_session_client_authentication_type
{
  gnutls_session_t c_session;
  gnutls_credentials_type_t c_auth;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_auth = gnutls_auth_client_get_type (c_session);

  return (scm_from_gnutls_credentials (c_auth));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_peer_certificate_chain,
            "session-peer-certificate-chain",
            1, 0, 0,
            (SCM session),
            "Return the a list of certificates in raw format (u8vectors) "
            "where the first one is the peer's certificate.  In the case "
            "of OpenPGP, there is always exactly one certificate.  In the "
            "case of X.509, subsequent certificates indicate form a "
            "certificate chain.  Return the empty list if no certificate "
            "was sent.")
#define FUNC_NAME s_scm_gnutls_session_peer_certificate_chain
{
  SCM result;
  gnutls_session_t c_session;
  const gnutls_datum_t *c_cert;
  unsigned int c_list_size;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  c_cert = gnutls_certificate_get_peers (c_session, &c_list_size);

  if (EXPECT_FALSE (c_cert == NULL))
    result = SCM_EOL;
  else
    {
      SCM pair;
      unsigned int i;

      result = scm_make_list (scm_from_uint (c_list_size), SCM_UNSPECIFIED);

      for (i = 0, pair = result; i < c_list_size; i++, pair = SCM_CDR (pair))
        {
          unsigned char *c_cert_copy;

          c_cert_copy = (unsigned char *) malloc (c_cert[i].size);
          if (EXPECT_FALSE (c_cert_copy == NULL))
            scm_gnutls_error (GNUTLS_E_MEMORY_ERROR, FUNC_NAME);

          memcpy (c_cert_copy, c_cert[i].data, c_cert[i].size);

          SCM_SETCAR (pair, scm_take_u8vector (c_cert_copy, c_cert[i].size));
        }
    }

  return result;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_session_our_certificate_chain,
            "session-our-certificate-chain",
            1, 0, 0,
            (SCM session),
            "Return our certificate chain for @var{session} (as sent to "
            "the peer) in raw format (a u8vector).  In the case of OpenPGP "
            "there is exactly one certificate.  Return the empty list "
            "if no certificate was used.")
#define FUNC_NAME s_scm_gnutls_session_our_certificate_chain
{
  SCM result;
  gnutls_session_t c_session;
  const gnutls_datum_t *c_cert;
  unsigned char *c_cert_copy;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  /* XXX: Currently, the C function actually returns only one certificate.
     Future versions of the API may provide the full certificate chain, as
     for `gnutls_certificate_get_peers ()'.  */
  c_cert = gnutls_certificate_get_ours (c_session);

  if (EXPECT_FALSE (c_cert == NULL))
    result = SCM_EOL;
  else
    {
      c_cert_copy = (unsigned char *) malloc (c_cert->size);
      if (EXPECT_FALSE (c_cert_copy == NULL))
        scm_gnutls_error (GNUTLS_E_MEMORY_ERROR, FUNC_NAME);

      memcpy (c_cert_copy, c_cert->data, c_cert->size);

      result = scm_list_1 (scm_take_u8vector (c_cert_copy, c_cert->size));
    }

  return result;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_server_session_certificate_request_x,
            "set-server-session-certificate-request!",
            2, 0, 0,
            (SCM session, SCM request),
            "Tell how @var{session}, a server-side session, should deal "
            "with certificate requests.  @var{request} should be either "
            "@code{certificate-request/request} or "
            "@code{certificate-request/require}.")
#define FUNC_NAME s_scm_gnutls_set_server_session_certificate_request_x
{
  gnutls_session_t c_session;
  gnutls_certificate_status_t c_request;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  c_request = scm_to_gnutls_certificate_request (request, 2, FUNC_NAME);

  gnutls_certificate_server_set_request (c_session, c_request);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME


/* Choice of a protocol and cipher suite.  */

SCM_DEFINE (scm_gnutls_set_default_priority_x,
            "set-session-default-priority!", 1, 0, 0,
            (SCM session), "Have @var{session} use the default priorities.")
#define FUNC_NAME s_scm_gnutls_set_default_priority_x
{
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  gnutls_set_default_priority (c_session);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_session_priorities_x,
	    "set-session-priorities!", 2, 0, 0,
	    (SCM session, SCM priorities),
	    "Have @var{session} use the given @var{priorities} for "
	    "the ciphers, key exchange methods, MACs and compression "
	    "methods.  @var{priorities} must be a string (@pxref{"
	    "Priority Strings,,, gnutls, GnuTLS@comma{} Transport Layer "
	    "Security Library for the GNU system}).  When @var{priorities} "
	    "cannot be parsed, an @code{error/invalid-request} error "
	    "is raised, with an extra argument indication the position "
	    "of the error.\n")
#define FUNC_NAME s_scm_gnutls_set_session_priorities_x
{
  int err;
  char *c_priorities;
  const char *err_pos;
  gnutls_session_t c_session;
  size_t pos;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  c_priorities = scm_to_locale_string (priorities); /* XXX: to_latin1_string */

  err = gnutls_priority_set_direct (c_session, c_priorities, &err_pos);
  if (err == GNUTLS_E_INVALID_REQUEST)
    pos = err_pos - c_priorities;

  free (c_priorities);

  switch (err)
    {
    case GNUTLS_E_SUCCESS:
      break;
    case GNUTLS_E_INVALID_REQUEST:
      {
	scm_gnutls_error_with_args (err, FUNC_NAME,
				    scm_list_1 (scm_from_size_t (pos)));
	break;
      }
    default:
      scm_gnutls_error (err, FUNC_NAME);
    }

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_cipher_suite_to_string, "cipher-suite->string",
            3, 0, 0,
            (SCM kx, SCM cipher, SCM mac),
            "Return the name of the given cipher suite.")
#define FUNC_NAME s_scm_gnutls_cipher_suite_to_string
{
  gnutls_kx_algorithm_t c_kx;
  gnutls_cipher_algorithm_t c_cipher;
  gnutls_mac_algorithm_t c_mac;
  const char *c_name;

  c_kx = scm_to_gnutls_kx (kx, 1, FUNC_NAME);
  c_cipher = scm_to_gnutls_cipher (cipher, 2, FUNC_NAME);
  c_mac = scm_to_gnutls_mac (mac, 3, FUNC_NAME);

  c_name = gnutls_cipher_suite_get_name (c_kx, c_cipher, c_mac);

  return (scm_from_locale_string (c_name));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_session_credentials_x, "set-session-credentials!",
            2, 0, 0,
            (SCM session, SCM cred),
            "Use @var{cred} as @var{session}'s credentials.")
#define FUNC_NAME s_scm_gnutls_set_session_credentials_x
{
  int err = 0;
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  if (SCM_SMOB_PREDICATE (scm_tc16_gnutls_certificate_credentials, cred))
    {
      gnutls_certificate_credentials_t c_cred;

      c_cred = scm_to_gnutls_certificate_credentials (cred, 2, FUNC_NAME);
      err =
        gnutls_credentials_set (c_session, GNUTLS_CRD_CERTIFICATE, c_cred);
    }
  else
    if (SCM_SMOB_PREDICATE
        (scm_tc16_gnutls_anonymous_client_credentials, cred))
    {
      gnutls_anon_client_credentials_t c_cred;

      c_cred = scm_to_gnutls_anonymous_client_credentials (cred, 2,
                                                           FUNC_NAME);
      err = gnutls_credentials_set (c_session, GNUTLS_CRD_ANON, c_cred);
    }
  else if (SCM_SMOB_PREDICATE (scm_tc16_gnutls_anonymous_server_credentials,
                               cred))
    {
      gnutls_anon_server_credentials_t c_cred;

      c_cred = scm_to_gnutls_anonymous_server_credentials (cred, 2,
                                                           FUNC_NAME);
      err = gnutls_credentials_set (c_session, GNUTLS_CRD_ANON, c_cred);
    }
#ifdef ENABLE_SRP
  else if (SCM_SMOB_PREDICATE (scm_tc16_gnutls_srp_client_credentials, cred))
    {
      gnutls_srp_client_credentials_t c_cred;

      c_cred = scm_to_gnutls_srp_client_credentials (cred, 2, FUNC_NAME);
      err = gnutls_credentials_set (c_session, GNUTLS_CRD_SRP, c_cred);
    }
  else if (SCM_SMOB_PREDICATE (scm_tc16_gnutls_srp_server_credentials, cred))
    {
      gnutls_srp_server_credentials_t c_cred;

      c_cred = scm_to_gnutls_srp_server_credentials (cred, 2, FUNC_NAME);
      err = gnutls_credentials_set (c_session, GNUTLS_CRD_SRP, c_cred);
    }
#endif
  else if (SCM_SMOB_PREDICATE (scm_tc16_gnutls_psk_client_credentials, cred))
    {
      gnutls_psk_client_credentials_t c_cred;

      c_cred = scm_to_gnutls_psk_client_credentials (cred, 2, FUNC_NAME);
      err = gnutls_credentials_set (c_session, GNUTLS_CRD_PSK, c_cred);
    }
  else if (SCM_SMOB_PREDICATE (scm_tc16_gnutls_psk_server_credentials, cred))
    {
      gnutls_psk_server_credentials_t c_cred;

      c_cred = scm_to_gnutls_psk_server_credentials (cred, 2, FUNC_NAME);
      err = gnutls_credentials_set (c_session, GNUTLS_CRD_PSK, c_cred);
    }
  else
    scm_wrong_type_arg (FUNC_NAME, 2, cred);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);
  else
    register_weak_reference (session, cred);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_session_server_name_x, "set-session-server-name!",
	    3, 0, 0,
	    (SCM session, SCM type, SCM name),
	    "For a client, this procedure provides a way to inform "
	    "the server that it is known under @var{name}, @i{via} the "
	    "@code{SERVER NAME} TLS extension.  @var{type} must be "
	    "a @code{server-name-type} value, @var{server-name-type/dns} "
	    "for DNS names.")
#define FUNC_NAME s_scm_gnutls_set_session_server_name_x
{
  int err;
  gnutls_session_t c_session;
  gnutls_server_name_type_t c_type;
  char *c_name;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  c_type = scm_to_gnutls_server_name_type (type, 2, FUNC_NAME);
  SCM_VALIDATE_STRING (3, name);

  c_name = scm_to_locale_string (name);

  err = gnutls_server_name_set (c_session, c_type, c_name,
				strlen (c_name));
  free (c_name);

  if (EXPECT_FALSE (err != GNUTLS_E_SUCCESS))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}
#undef FUNC_NAME


/* Record layer.  */

SCM_DEFINE (scm_gnutls_record_send, "record-send", 2, 0, 0,
            (SCM session, SCM array),
            "Send the record constituted by @var{array} through "
            "@var{session}.")
#define FUNC_NAME s_scm_gnutls_record_send
{
  SCM result;
  ssize_t c_result;
  gnutls_session_t c_session;
  scm_t_array_handle c_handle;
  const char *c_array;
  size_t c_len;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  SCM_VALIDATE_ARRAY (2, array);

  c_array = scm_gnutls_get_array (array, &c_handle, &c_len, FUNC_NAME);

  c_result = gnutls_record_send (c_session, c_array, c_len);

  scm_gnutls_release_array (&c_handle);

  if (EXPECT_TRUE (c_result >= 0))
    result = scm_from_ssize_t (c_result);
  else
    scm_gnutls_error (c_result, FUNC_NAME);

  return (result);
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_record_receive_x, "record-receive!", 2, 0, 0,
            (SCM session, SCM array),
            "Receive data from @var{session} into @var{array}, a uniform "
            "homogeneous array.  Return the number of bytes actually "
            "received.")
#define FUNC_NAME s_scm_gnutls_record_receive_x
{
  SCM result;
  ssize_t c_result;
  gnutls_session_t c_session;
  scm_t_array_handle c_handle;
  char *c_array;
  size_t c_len;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  SCM_VALIDATE_ARRAY (2, array);

  c_array = scm_gnutls_get_writable_array (array, &c_handle, &c_len,
                                           FUNC_NAME);

  c_result = gnutls_record_recv (c_session, c_array, c_len);

  scm_gnutls_release_array (&c_handle);

  if (EXPECT_TRUE (c_result >= 0))
    result = scm_from_ssize_t (c_result);
  else
    scm_gnutls_error (c_result, FUNC_NAME);

  return (result);
}

#undef FUNC_NAME


/* Whether we're using Guile < 2.2.  */
#define USING_GUILE_BEFORE_2_2					\
  (SCM_MAJOR_VERSION < 2					\
   || (SCM_MAJOR_VERSION == 2 && SCM_MINOR_VERSION == 0))

/* The session record port type.  Guile 2.1.4 introduced a brand new port API,
   so we have a separate implementation for these newer versions.  */
#if USING_GUILE_BEFORE_2_2
static scm_t_bits session_record_port_type;

/* Hint for the `scm_gc_' functions.  */
static const char session_record_port_gc_hint[] =
  "gnutls-session-record-port";
#else
static scm_t_port_type *session_record_port_type;
#endif

/* Return the session associated with PORT.  */
#define SCM_GNUTLS_SESSION_RECORD_PORT_SESSION(_port) \
  (SCM_PACK (SCM_STREAM (_port)))

/* Size of a session port's input buffer.  */
#define SCM_GNUTLS_SESSION_RECORD_PORT_BUFFER_SIZE 4096


#if SCM_MAJOR_VERSION == 1 && SCM_MINOR_VERSION <= 8

/* Mark the session associated with PORT.  */
static SCM
mark_session_record_port (SCM port)
{
  return (SCM_GNUTLS_SESSION_RECORD_PORT_SESSION (port));
}

static size_t
free_session_record_port (SCM port)
#define FUNC_NAME "free_session_record_port"
{
  SCM session;
  scm_t_port *c_port;

  session = SCM_GNUTLS_SESSION_RECORD_PORT_SESSION (port);

  /* SESSION _can_ be invalid at this point: it can be freed in the same GC
     cycle as PORT, just before PORT.  Thus, we need to check whether SESSION
     still points to a session SMOB.  */
  if (SCM_SMOB_PREDICATE (scm_tc16_gnutls_session, session))
    {
      /* SESSION is still valid.  Disassociate PORT from SESSION.  */
      gnutls_session_t c_session;

      c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
      SCM_GNUTLS_SET_SESSION_RECORD_PORT (c_session, SCM_BOOL_F);
    }

  /* Free the input buffer of PORT.  */
  c_port = SCM_PTAB_ENTRY (port);
  scm_gc_free (c_port->read_buf, c_port->read_buf_size,
               session_record_port_gc_hint);

  return 0;
}

#undef FUNC_NAME

#endif /* SCM_MAJOR_VERSION == 1 && SCM_MINOR_VERSION <= 8 */


#if USING_GUILE_BEFORE_2_2

/* Data passed to `do_fill_port ()'.  */
typedef struct
{
  scm_t_port *c_port;
  gnutls_session_t c_session;
} fill_port_data_t;

/* Actually fill a session record port (see below).  */
static void *
do_fill_port (void *data)
{
  int chr;
  ssize_t result;
  scm_t_port *c_port;
  const fill_port_data_t *args = (fill_port_data_t *) data;

  c_port = args->c_port;
  result = gnutls_record_recv (args->c_session,
                               c_port->read_buf, c_port->read_buf_size);
  if (EXPECT_TRUE (result > 0))
    {
      c_port->read_pos = c_port->read_buf;
      c_port->read_end = c_port->read_buf + result;
      chr = (int) *c_port->read_buf;
    }
  else if (result == 0)
    chr = EOF;
  else
    scm_gnutls_error (result, "fill_session_record_port_input");

  return ((void *) (uintptr_t) chr);
}

/* Fill in the input buffer of PORT.  */
static int
fill_session_record_port_input (SCM port)
#define FUNC_NAME "fill_session_record_port_input"
{
  int chr;
  scm_t_port *c_port = SCM_PTAB_ENTRY (port);

  if (c_port->read_pos >= c_port->read_end)
    {
      SCM session;
      fill_port_data_t c_args;
      gnutls_session_t c_session;

      session = SCM_GNUTLS_SESSION_RECORD_PORT_SESSION (port);
      c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

      c_args.c_session = c_session;
      c_args.c_port = c_port;

      if (SCM_GNUTLS_SESSION_TRANSPORT_IS_FD (c_session))
        /* SESSION's underlying transport is a raw file descriptor, so we
           must leave "Guile mode" to allow the GC to run.  */
        chr = (intptr_t) scm_without_guile (do_fill_port, &c_args);
      else
        /* SESSION's underlying transport is a port, so don't leave "Guile
           mode".  */
        chr = (intptr_t) do_fill_port (&c_args);
    }
  else
    chr = (int) *c_port->read_pos;

  return chr;
}

#undef FUNC_NAME

/* Write SIZE octets from DATA to PORT.  */
static void
write_to_session_record_port (SCM port, const void *data, size_t size)
#define FUNC_NAME "write_to_session_record_port"
{
  SCM session;
  gnutls_session_t c_session;
  ssize_t c_result;
  size_t c_sent = 0;

  session = SCM_GNUTLS_SESSION_RECORD_PORT_SESSION (port);
  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  while (c_sent < size)
    {
      c_result = gnutls_record_send (c_session, (char *) data + c_sent,
                                     size - c_sent);
      if (EXPECT_FALSE (c_result < 0))
        scm_gnutls_error (c_result, FUNC_NAME);
      else
        c_sent += c_result;
    }
}

#undef FUNC_NAME

/* Return a new session port for SESSION.  */
static SCM
make_session_record_port (SCM session)
{
  SCM port;
  scm_t_port *c_port;
  unsigned char *c_port_buf;
  const unsigned long mode_bits = SCM_OPN | SCM_RDNG | SCM_WRTNG;

  c_port_buf = (unsigned char *)
#ifdef HAVE_SCM_GC_MALLOC_POINTERLESS
    scm_gc_malloc_pointerless
#else
    scm_gc_malloc
#endif
    (SCM_GNUTLS_SESSION_RECORD_PORT_BUFFER_SIZE, session_record_port_gc_hint);

  /* Create a new port.  */
  port = scm_new_port_table_entry (session_record_port_type);
  c_port = SCM_PTAB_ENTRY (port);

  /* Mark PORT as open, readable and writable (hmm, how elegant...).  */
  SCM_SET_CELL_TYPE (port, session_record_port_type | mode_bits);

  /* Associate it with SESSION.  */
  SCM_SETSTREAM (port, SCM_UNPACK (session));

  c_port->read_pos = c_port->read_end = c_port->read_buf = c_port_buf;
  c_port->read_buf_size = SCM_GNUTLS_SESSION_RECORD_PORT_BUFFER_SIZE;

  c_port->write_buf = c_port->write_pos = &c_port->shortbuf;
  c_port->write_buf_size = 1;

  return (port);
}

#else  /* !USING_GUILE_BEFORE_2_2 */

static size_t
read_from_session_record_port (SCM port, SCM dst, size_t start, size_t count)
#define FUNC_NAME "read_from_session_record_port"
{
  SCM session;
  gnutls_session_t c_session;
  char *read_buf;
  ssize_t result;

  session = SCM_GNUTLS_SESSION_RECORD_PORT_SESSION (port);
  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  read_buf = (char *) SCM_BYTEVECTOR_CONTENTS (dst) + start;

  /* XXX: Leave guile mode when SCM_GNUTLS_SESSION_TRANSPORT_IS_FD is
     true?  */
  result = gnutls_record_recv (c_session, read_buf, count);
  if (EXPECT_FALSE (result < 0))
    /* FIXME: Silently swallowed! */
    scm_gnutls_error (result, FUNC_NAME);

  return result;
}
#undef FUNC_NAME

static size_t
write_to_session_record_port (SCM port, SCM src, size_t start, size_t count)
#define FUNC_NAME "write_to_session_record_port"
{
  SCM session;
  gnutls_session_t c_session;
  char *data;
  ssize_t result;

  session = SCM_GNUTLS_SESSION_RECORD_PORT_SESSION (port);
  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  data = (char *) SCM_BYTEVECTOR_CONTENTS (src) + start;

  result = gnutls_record_send (c_session, data, count);

  if (EXPECT_FALSE (result < 0))
    scm_gnutls_error (result, FUNC_NAME);

  return result;
}
#undef FUNC_NAME

/* Return a new session port for SESSION.  */
static SCM
make_session_record_port (SCM session)
{
  return scm_c_make_port (session_record_port_type,
			  SCM_OPN | SCM_RDNG | SCM_WRTNG | SCM_BUF0,
			  SCM_UNPACK (session));
}

#endif	/* !USING_GUILE_BEFORE_2_2 */


SCM_DEFINE (scm_gnutls_session_record_port, "session-record-port", 1, 0, 0,
            (SCM session),
            "Return a read-write port that may be used to communicate over "
            "@var{session}.  All invocations of @code{session-port} on a "
            "given session return the same object (in the sense of "
            "@code{eq?}).")
#define FUNC_NAME s_scm_gnutls_session_record_port
{
  SCM port;
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  port = SCM_GNUTLS_SESSION_RECORD_PORT (c_session);

  if (!SCM_PORTP (port))
    {
      /* Lazily create a new session port.  */
      port = make_session_record_port (session);
      SCM_GNUTLS_SET_SESSION_RECORD_PORT (c_session, port);
    }

  return (port);
}

#undef FUNC_NAME

/* Create the session port type.  */
static void
scm_init_gnutls_session_record_port_type (void)
{
  session_record_port_type =
    scm_make_port_type ("gnutls-session-port",
#if USING_GUILE_BEFORE_2_2
                        fill_session_record_port_input,
#else
                        read_from_session_record_port,
#endif
                        write_to_session_record_port);

  /* Guile >= 1.9.3 doesn't need a custom mark procedure, and doesn't need a
     finalizer (since memory associated with the port is automatically
     reclaimed.)  */
#if SCM_MAJOR_VERSION == 1 && SCM_MINOR_VERSION <= 8
  scm_set_port_mark (session_record_port_type, mark_session_record_port);
  scm_set_port_free (session_record_port_type, free_session_record_port);
#endif
}


/* Transport.  */

SCM_DEFINE (scm_gnutls_set_session_transport_fd_x,
            "set-session-transport-fd!", 2, 0, 0, (SCM session, SCM fd),
            "Use file descriptor @var{fd} as the underlying transport for "
            "@var{session}.")
#define FUNC_NAME s_scm_gnutls_set_session_transport_fd_x
{
  gnutls_session_t c_session;
  int c_fd;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  c_fd = (int) scm_to_uint (fd);

  gnutls_transport_set_ptr (c_session,
                            (gnutls_transport_ptr_t) (intptr_t) c_fd);

  SCM_GNUTLS_SET_SESSION_TRANSPORT_IS_FD (c_session, 1);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

/* Pull SIZE octets from TRANSPORT (a Scheme port) into DATA.  */
static ssize_t
pull_from_port (gnutls_transport_ptr_t transport, void *data, size_t size)
{
  SCM port;
  ssize_t result;

  port = SCM_PACK ((scm_t_bits) transport);

  result = scm_c_read (port, data, size);

  return ((ssize_t) result);
}

/* Write SIZE octets from DATA to TRANSPORT (a Scheme port).  */
static ssize_t
push_to_port (gnutls_transport_ptr_t transport, const void *data, size_t size)
{
  SCM port;

  port = SCM_PACK ((scm_t_bits) transport);

  scm_c_write (port, data, size);

  /* All we can do is assume that all SIZE octets were written.  */
  return (size);
}

SCM_DEFINE (scm_gnutls_set_session_transport_port_x,
            "set-session-transport-port!",
            2, 0, 0,
            (SCM session, SCM port),
            "Use @var{port} as the input/output port for @var{session}.")
#define FUNC_NAME s_scm_gnutls_set_session_transport_port_x
{
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  SCM_VALIDATE_PORT (2, port);

  /* Note: We do not attempt to optimize the case where PORT is a file port
     (i.e., over a file descriptor), because of port buffering issues.  Users
     are expected to explicitly use `set-session-transport-fd!' and `fileno'
     when they wish to do it.  */

  gnutls_transport_set_ptr (c_session,
                            (gnutls_transport_ptr_t) SCM_UNPACK (port));
  gnutls_transport_set_push_function (c_session, push_to_port);
  gnutls_transport_set_pull_function (c_session, pull_from_port);

  SCM_GNUTLS_SET_SESSION_TRANSPORT_IS_FD (c_session, 0);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME


/* Diffie-Hellman.  */

typedef int (*pkcs_export_function_t) (void *, gnutls_x509_crt_fmt_t,
                                       unsigned char *, size_t *);

/* Hint for the `scm_gc' functions.  */
static const char pkcs_export_gc_hint[] = "gnutls-pkcs-export";


/* Export DH/RSA parameters PARAMS through EXPORT, using format FORMAT.
   Return a `u8vector'.  */
static inline SCM
pkcs_export_parameters (pkcs_export_function_t export,
                        void *params, gnutls_x509_crt_fmt_t format,
                        const char *func_name)
#define FUNC_NAME func_name
{
  int err;
  unsigned char *output;
  size_t output_len, output_total_len = 4096;

  output = (unsigned char *) scm_gc_malloc (output_total_len,
                                            pkcs_export_gc_hint);
  do
    {
      output_len = output_total_len;
      err = export (params, format, output, &output_len);

      if (err == GNUTLS_E_SHORT_MEMORY_BUFFER)
        {
          output = scm_gc_realloc (output, output_total_len,
                                   output_total_len * 2, pkcs_export_gc_hint);
          output_total_len *= 2;
        }
    }
  while (err == GNUTLS_E_SHORT_MEMORY_BUFFER);

  if (EXPECT_FALSE (err))
    {
      scm_gc_free (output, output_total_len, pkcs_export_gc_hint);
      scm_gnutls_error (err, FUNC_NAME);
    }

  if (output_len != output_total_len)
    /* Shrink the output buffer.  */
    output = scm_gc_realloc (output, output_total_len,
                             output_len, pkcs_export_gc_hint);

  return (scm_take_u8vector (output, output_len));
}

#undef FUNC_NAME


SCM_DEFINE (scm_gnutls_make_dh_parameters, "make-dh-parameters", 1, 0, 0,
            (SCM bits), "Return new Diffie-Hellman parameters.")
#define FUNC_NAME s_scm_gnutls_make_dh_parameters
{
  int err;
  unsigned c_bits;
  gnutls_dh_params_t c_dh_params;

  c_bits = scm_to_uint (bits);

  err = gnutls_dh_params_init (&c_dh_params);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  err = gnutls_dh_params_generate2 (c_dh_params, c_bits);
  if (EXPECT_FALSE (err))
    {
      gnutls_dh_params_deinit (c_dh_params);
      scm_gnutls_error (err, FUNC_NAME);
    }

  return (scm_from_gnutls_dh_parameters (c_dh_params));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_pkcs3_import_dh_parameters,
            "pkcs3-import-dh-parameters",
            2, 0, 0,
            (SCM array, SCM format),
            "Import Diffie-Hellman parameters in PKCS3 format (further "
            "specified by @var{format}, an @code{x509-certificate-format} "
            "value) from @var{array} (a homogeneous array) and return a "
            "new @code{dh-params} object.")
#define FUNC_NAME s_scm_gnutls_pkcs3_import_dh_parameters
{
  int err;
  gnutls_x509_crt_fmt_t c_format;
  gnutls_dh_params_t c_dh_params;
  scm_t_array_handle c_handle;
  const char *c_array;
  size_t c_len;
  gnutls_datum_t c_datum;

  c_format = scm_to_gnutls_x509_certificate_format (format, 2, FUNC_NAME);

  c_array = scm_gnutls_get_array (array, &c_handle, &c_len, FUNC_NAME);
  c_datum.data = (unsigned char *) c_array;
  c_datum.size = c_len;

  err = gnutls_dh_params_init (&c_dh_params);
  if (EXPECT_FALSE (err))
    {
      scm_gnutls_release_array (&c_handle);
      scm_gnutls_error (err, FUNC_NAME);
    }

  err = gnutls_dh_params_import_pkcs3 (c_dh_params, &c_datum, c_format);
  scm_gnutls_release_array (&c_handle);

  if (EXPECT_FALSE (err))
    {
      gnutls_dh_params_deinit (c_dh_params);
      scm_gnutls_error (err, FUNC_NAME);
    }

  return (scm_from_gnutls_dh_parameters (c_dh_params));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_pkcs3_export_dh_parameters,
            "pkcs3-export-dh-parameters",
            2, 0, 0,
            (SCM dh_params, SCM format),
            "Export Diffie-Hellman parameters @var{dh_params} in PKCS3 "
            "format according for @var{format} (an "
            "@code{x509-certificate-format} value).  Return a "
            "@code{u8vector} containing the result.")
#define FUNC_NAME s_scm_gnutls_pkcs3_export_dh_parameters
{
  SCM result;
  gnutls_dh_params_t c_dh_params;
  gnutls_x509_crt_fmt_t c_format;

  c_dh_params = scm_to_gnutls_dh_parameters (dh_params, 1, FUNC_NAME);
  c_format = scm_to_gnutls_x509_certificate_format (format, 2, FUNC_NAME);

  result = pkcs_export_parameters ((pkcs_export_function_t)
                                   gnutls_dh_params_export_pkcs3,
                                   (void *) c_dh_params, c_format, FUNC_NAME);

  return (result);
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_session_dh_prime_bits_x,
            "set-session-dh-prime-bits!", 2, 0, 0,
            (SCM session, SCM bits),
            "Use @var{bits} DH prime bits for @var{session}.")
#define FUNC_NAME s_scm_gnutls_set_session_dh_prime_bits_x
{
  unsigned int c_bits;
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  c_bits = scm_to_uint (bits);

  gnutls_dh_set_prime_bits (c_session, c_bits);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME


/* Anonymous credentials.  */

SCM_DEFINE (scm_gnutls_make_anon_server_credentials,
            "make-anonymous-server-credentials",
            0, 0, 0, (void), "Return anonymous server credentials.")
#define FUNC_NAME s_scm_gnutls_make_anon_server_credentials
{
  int err;
  gnutls_anon_server_credentials_t c_cred;

  err = gnutls_anon_allocate_server_credentials (&c_cred);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_from_gnutls_anonymous_server_credentials (c_cred));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_make_anon_client_credentials,
            "make-anonymous-client-credentials",
            0, 0, 0, (void), "Return anonymous client credentials.")
#define FUNC_NAME s_scm_gnutls_make_anon_client_credentials
{
  int err;
  gnutls_anon_client_credentials_t c_cred;

  err = gnutls_anon_allocate_client_credentials (&c_cred);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_from_gnutls_anonymous_client_credentials (c_cred));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_anonymous_server_dh_parameters_x,
            "set-anonymous-server-dh-parameters!", 2, 0, 0,
            (SCM cred, SCM dh_params),
            "Set the Diffie-Hellman parameters of anonymous server "
            "credentials @var{cred}.")
#define FUNC_NAME s_scm_gnutls_set_anonymous_server_dh_parameters_x
{
  gnutls_dh_params_t c_dh_params;
  gnutls_anon_server_credentials_t c_cred;

  c_cred = scm_to_gnutls_anonymous_server_credentials (cred, 1, FUNC_NAME);
  c_dh_params = scm_to_gnutls_dh_parameters (dh_params, 2, FUNC_NAME);

  gnutls_anon_set_server_dh_params (c_cred, c_dh_params);
  register_weak_reference (cred, dh_params);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME



/* Certificate credentials.  */

typedef
  int (*certificate_set_file_function_t) (gnutls_certificate_credentials_t,
                                          const char *,
                                          gnutls_x509_crt_fmt_t);

typedef
  int (*certificate_set_data_function_t) (gnutls_certificate_credentials_t,
                                          const gnutls_datum_t *,
                                          gnutls_x509_crt_fmt_t);

/* Helper function to implement the `set-file!' functions.  */
static unsigned int
set_certificate_file (certificate_set_file_function_t set_file,
                      SCM cred, SCM file, SCM format, const char *func_name)
#define FUNC_NAME func_name
{
  int err;
  char *c_file;
  size_t c_file_len;

  gnutls_certificate_credentials_t c_cred;
  gnutls_x509_crt_fmt_t c_format;

  c_cred = scm_to_gnutls_certificate_credentials (cred, 1, FUNC_NAME);
  SCM_VALIDATE_STRING (2, file);
  c_format = scm_to_gnutls_x509_certificate_format (format, 3, FUNC_NAME);

  c_file_len = scm_c_string_length (file);
  c_file = alloca (c_file_len + 1);

  (void) scm_to_locale_stringbuf (file, c_file, c_file_len + 1);
  c_file[c_file_len] = '\0';

  err = set_file (c_cred, c_file, c_format);
  if (EXPECT_FALSE (err < 0))
    scm_gnutls_error (err, FUNC_NAME);

  /* Return the number of certificates processed.  */
  return ((unsigned int) err);
}

#undef FUNC_NAME

/* Helper function implementing the `set-data!' functions.  */
static inline unsigned int
set_certificate_data (certificate_set_data_function_t set_data,
                      SCM cred, SCM data, SCM format, const char *func_name)
#define FUNC_NAME func_name
{
  int err;
  gnutls_certificate_credentials_t c_cred;
  gnutls_x509_crt_fmt_t c_format;
  gnutls_datum_t c_datum;
  scm_t_array_handle c_handle;
  const char *c_data;
  size_t c_len;

  c_cred = scm_to_gnutls_certificate_credentials (cred, 1, FUNC_NAME);
  SCM_VALIDATE_ARRAY (2, data);
  c_format = scm_to_gnutls_x509_certificate_format (format, 3, FUNC_NAME);

  c_data = scm_gnutls_get_array (data, &c_handle, &c_len, FUNC_NAME);
  c_datum.data = (unsigned char *) c_data;
  c_datum.size = c_len;

  err = set_data (c_cred, &c_datum, c_format);
  scm_gnutls_release_array (&c_handle);

  if (EXPECT_FALSE (err < 0))
    scm_gnutls_error (err, FUNC_NAME);

  /* Return the number of certificates processed.  */
  return ((unsigned int) err);
}

#undef FUNC_NAME


SCM_DEFINE (scm_gnutls_make_certificate_credentials,
            "make-certificate-credentials",
            0, 0, 0,
            (void),
            "Return new certificate credentials (i.e., for use with "
            "either X.509 or OpenPGP certificates.")
#define FUNC_NAME s_scm_gnutls_make_certificate_credentials
{
  int err;
  gnutls_certificate_credentials_t c_cred;

  err = gnutls_certificate_allocate_credentials (&c_cred);
  if (err)
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_from_gnutls_certificate_credentials (c_cred));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_dh_params_x,
            "set-certificate-credentials-dh-parameters!",
            2, 0, 0,
            (SCM cred, SCM dh_params),
            "Use Diffie-Hellman parameters @var{dh_params} for "
            "certificate credentials @var{cred}.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_dh_params_x
{
  gnutls_dh_params_t c_dh_params;
  gnutls_certificate_credentials_t c_cred;

  c_cred = scm_to_gnutls_certificate_credentials (cred, 1, FUNC_NAME);
  c_dh_params = scm_to_gnutls_dh_parameters (dh_params, 2, FUNC_NAME);

  gnutls_certificate_set_dh_params (c_cred, c_dh_params);
  register_weak_reference (cred, dh_params);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_x509_key_files_x,
            "set-certificate-credentials-x509-key-files!",
            4, 0, 0,
            (SCM cred, SCM cert_file, SCM key_file, SCM format),
            "Use @var{file} as the password file for PSK server "
            "credentials @var{cred}.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_x509_key_files_x
{
  int err;
  gnutls_certificate_credentials_t c_cred;
  gnutls_x509_crt_fmt_t c_format;
  char *c_cert_file, *c_key_file;
  size_t c_cert_file_len, c_key_file_len;

  c_cred = scm_to_gnutls_certificate_credentials (cred, 1, FUNC_NAME);
  SCM_VALIDATE_STRING (2, cert_file);
  SCM_VALIDATE_STRING (3, key_file);
  c_format = scm_to_gnutls_x509_certificate_format (format, 2, FUNC_NAME);

  c_cert_file_len = scm_c_string_length (cert_file);
  c_cert_file = alloca (c_cert_file_len + 1);

  c_key_file_len = scm_c_string_length (key_file);
  c_key_file = alloca (c_key_file_len + 1);

  (void) scm_to_locale_stringbuf (cert_file, c_cert_file,
                                  c_cert_file_len + 1);
  c_cert_file[c_cert_file_len] = '\0';
  (void) scm_to_locale_stringbuf (key_file, c_key_file, c_key_file_len + 1);
  c_key_file[c_key_file_len] = '\0';

  err = gnutls_certificate_set_x509_key_file (c_cred, c_cert_file, c_key_file,
                                              c_format);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_x509_trust_file_x,
            "set-certificate-credentials-x509-trust-file!",
            3, 0, 0,
            (SCM cred, SCM file, SCM format),
            "Use @var{file} as the X.509 trust file for certificate "
            "credentials @var{cred}.  On success, return the number of "
            "certificates processed.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_x509_trust_file_x
{
  unsigned int count;

  count = set_certificate_file (gnutls_certificate_set_x509_trust_file,
                                cred, file, format, FUNC_NAME);

  return scm_from_uint (count);
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_x509_crl_file_x,
            "set-certificate-credentials-x509-crl-file!",
            3, 0, 0,
            (SCM cred, SCM file, SCM format),
            "Use @var{file} as the X.509 CRL (certificate revocation list) "
            "file for certificate credentials @var{cred}.  On success, "
            "return the number of CRLs processed.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_x509_crl_file_x
{
  unsigned int count;

  count = set_certificate_file (gnutls_certificate_set_x509_crl_file,
                                cred, file, format, FUNC_NAME);

  return scm_from_uint (count);
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_x509_trust_data_x,
            "set-certificate-credentials-x509-trust-data!",
            3, 0, 0,
            (SCM cred, SCM data, SCM format),
            "Use @var{data} (a uniform array) as the X.509 trust "
            "database for @var{cred}.  On success, return the number "
            "of certificates processed.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_x509_trust_data_x
{
  unsigned int count;

  count = set_certificate_data (gnutls_certificate_set_x509_trust_mem,
                                cred, data, format, FUNC_NAME);

  return scm_from_uint (count);
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_x509_crl_data_x,
            "set-certificate-credentials-x509-crl-data!",
            3, 0, 0,
            (SCM cred, SCM data, SCM format),
            "Use @var{data} (a uniform array) as the X.509 CRL "
            "(certificate revocation list) database for @var{cred}.  "
            "On success, return the number of CRLs processed.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_x509_crl_data_x
{
  unsigned int count;

  count = set_certificate_data (gnutls_certificate_set_x509_crl_mem,
                                cred, data, format, FUNC_NAME);

  return scm_from_uint (count);
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_x509_key_data_x,
            "set-certificate-credentials-x509-key-data!",
            4, 0, 0,
            (SCM cred, SCM cert, SCM key, SCM format),
            "Use X.509 certificate @var{cert} and private key @var{key}, "
            "both uniform arrays containing the X.509 certificate and key "
            "in format @var{format}, for certificate credentials "
            "@var{cred}.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_x509_key_data_x
{
  int err;
  gnutls_x509_crt_fmt_t c_format;
  gnutls_certificate_credentials_t c_cred;
  gnutls_datum_t c_cert_d, c_key_d;
  scm_t_array_handle c_cert_handle, c_key_handle;
  const char *c_cert, *c_key;
  size_t c_cert_len, c_key_len;

  c_cred = scm_to_gnutls_certificate_credentials (cred, 1, FUNC_NAME);
  c_format = scm_to_gnutls_x509_certificate_format (format, 4, FUNC_NAME);
  SCM_VALIDATE_ARRAY (2, cert);
  SCM_VALIDATE_ARRAY (3, key);

  /* FIXME: If the second call fails, an exception is raised and
     C_CERT_HANDLE is not released.  */
  c_cert = scm_gnutls_get_array (cert, &c_cert_handle, &c_cert_len,
                                 FUNC_NAME);
  c_key = scm_gnutls_get_array (key, &c_key_handle, &c_key_len, FUNC_NAME);

  c_cert_d.data = (unsigned char *) c_cert;
  c_cert_d.size = c_cert_len;
  c_key_d.data = (unsigned char *) c_key;
  c_key_d.size = c_key_len;

  err = gnutls_certificate_set_x509_key_mem (c_cred, &c_cert_d, &c_key_d,
                                             c_format);
  scm_gnutls_release_array (&c_cert_handle);
  scm_gnutls_release_array (&c_key_handle);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_x509_keys_x,
            "set-certificate-credentials-x509-keys!",
            3, 0, 0,
            (SCM cred, SCM certs, SCM privkey),
            "Have certificate credentials @var{cred} use the X.509 "
            "certificates listed in @var{certs} and X.509 private key "
            "@var{privkey}.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_x509_keys_x
{
  int err;
  gnutls_x509_crt_t *c_certs;
  gnutls_x509_privkey_t c_key;
  gnutls_certificate_credentials_t c_cred;
  long int c_cert_count, i;

  c_cred = scm_to_gnutls_certificate_credentials (cred, 1, FUNC_NAME);
  SCM_VALIDATE_LIST_COPYLEN (2, certs, c_cert_count);
  c_key = scm_to_gnutls_x509_private_key (privkey, 3, FUNC_NAME);

  c_certs = alloca (c_cert_count * sizeof (*c_certs));
  for (i = 0; scm_is_pair (certs); certs = SCM_CDR (certs), i++)
    {
      c_certs[i] = scm_to_gnutls_x509_certificate (SCM_CAR (certs),
                                                   2, FUNC_NAME);
    }

  err = gnutls_certificate_set_x509_key (c_cred, c_certs, c_cert_count,
                                         c_key);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);
  else
    {
      register_weak_reference (cred, privkey);
      register_weak_reference (cred, scm_list_copy (certs));
    }

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_verify_limits_x,
            "set-certificate-credentials-verify-limits!",
            3, 0, 0,
            (SCM cred, SCM max_bits, SCM max_depth),
            "Set the verification limits of @code{peer-certificate-status} "
            "for certificate credentials @var{cred} to @var{max_bits} "
            "bits for an acceptable certificate and @var{max_depth} "
            "as the maximum depth of a certificate chain.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_verify_limits_x
{
  gnutls_certificate_credentials_t c_cred;
  unsigned int c_max_bits, c_max_depth;

  c_cred = scm_to_gnutls_certificate_credentials (cred, 1, FUNC_NAME);
  c_max_bits = scm_to_uint (max_bits);
  c_max_depth = scm_to_uint (max_depth);

  gnutls_certificate_set_verify_limits (c_cred, c_max_bits, c_max_depth);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_certificate_credentials_verify_flags_x,
            "set-certificate-credentials-verify-flags!",
            1, 0, 1,
            (SCM cred, SCM flags),
            "Set the certificate verification flags to @var{flags}, a "
            "series of @code{certificate-verify} values.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_verify_flags_x
{
  unsigned int c_flags, c_pos;
  gnutls_certificate_credentials_t c_cred;

  c_cred = scm_to_gnutls_certificate_credentials (cred, 1, FUNC_NAME);

  for (c_flags = 0, c_pos = 2;
       !scm_is_null (flags); flags = SCM_CDR (flags), c_pos++)
    {
      c_flags |= (unsigned int)
        scm_to_gnutls_certificate_verify (SCM_CAR (flags), c_pos, FUNC_NAME);
    }

  gnutls_certificate_set_verify_flags (c_cred, c_flags);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_peer_certificate_status, "peer-certificate-status",
            1, 0, 0,
            (SCM session),
            "Verify the peer certificate for @var{session} and return "
            "a list of @code{certificate-status} values (such as "
            "@code{certificate-status/revoked}), or the empty list if "
            "the certificate is valid.")
#define FUNC_NAME s_scm_gnutls_peer_certificate_status
{
  int err;
  unsigned int c_status;
  gnutls_session_t c_session;
  SCM result = SCM_EOL;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);

  err = gnutls_certificate_verify_peers2 (c_session, &c_status);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

#define MATCH_STATUS(_value)						\
  if (c_status & (_value))						\
    {									\
      result = scm_cons (scm_from_gnutls_certificate_status (_value),	\
			 result);					\
      c_status &= ~(_value);						\
    }

  MATCH_STATUS (GNUTLS_CERT_INVALID);
  MATCH_STATUS (GNUTLS_CERT_REVOKED);
  MATCH_STATUS (GNUTLS_CERT_SIGNER_NOT_FOUND);
  MATCH_STATUS (GNUTLS_CERT_SIGNER_NOT_CA);
  MATCH_STATUS (GNUTLS_CERT_INSECURE_ALGORITHM);

  if (EXPECT_FALSE (c_status != 0))
    /* XXX: We failed to interpret one of the status flags.  */
    scm_gnutls_error (GNUTLS_E_UNIMPLEMENTED_FEATURE, FUNC_NAME);

#undef MATCH_STATUS

  return (result);
}

#undef FUNC_NAME


/* SRP credentials.  */

#ifdef ENABLE_SRP
SCM_DEFINE (scm_gnutls_make_srp_server_credentials,
            "make-srp-server-credentials",
            0, 0, 0, (void), "Return new SRP server credentials.")
#define FUNC_NAME s_scm_gnutls_make_srp_server_credentials
{
  int err;
  gnutls_srp_server_credentials_t c_cred;

  err = gnutls_srp_allocate_server_credentials (&c_cred);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_from_gnutls_srp_server_credentials (c_cred));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_srp_server_credentials_files_x,
            "set-srp-server-credentials-files!",
            3, 0, 0,
            (SCM cred, SCM password_file, SCM password_conf_file),
            "Set the credentials files for @var{cred}, an SRP server "
            "credentials object.")
#define FUNC_NAME s_scm_gnutls_set_srp_server_credentials_files_x
{
  int err;
  gnutls_srp_server_credentials_t c_cred;
  char *c_password_file, *c_password_conf_file;
  size_t c_password_file_len, c_password_conf_file_len;

  c_cred = scm_to_gnutls_srp_server_credentials (cred, 1, FUNC_NAME);
  SCM_VALIDATE_STRING (2, password_file);
  SCM_VALIDATE_STRING (3, password_conf_file);

  c_password_file_len = scm_c_string_length (password_file);
  c_password_conf_file_len = scm_c_string_length (password_conf_file);

  c_password_file = alloca (c_password_file_len + 1);
  c_password_conf_file = alloca (c_password_conf_file_len + 1);

  (void) scm_to_locale_stringbuf (password_file, c_password_file,
                                  c_password_file_len + 1);
  c_password_file[c_password_file_len] = '\0';
  (void) scm_to_locale_stringbuf (password_conf_file, c_password_conf_file,
                                  c_password_conf_file_len + 1);
  c_password_conf_file[c_password_conf_file_len] = '\0';

  err = gnutls_srp_set_server_credentials_file (c_cred, c_password_file,
                                                c_password_conf_file);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_make_srp_client_credentials,
            "make-srp-client-credentials",
            0, 0, 0, (void), "Return new SRP client credentials.")
#define FUNC_NAME s_scm_gnutls_make_srp_client_credentials
{
  int err;
  gnutls_srp_client_credentials_t c_cred;

  err = gnutls_srp_allocate_client_credentials (&c_cred);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_from_gnutls_srp_client_credentials (c_cred));
}

#undef FUNC_NAME


SCM_DEFINE (scm_gnutls_set_srp_client_credentials_x,
            "set-srp-client-credentials!",
            3, 0, 0,
            (SCM cred, SCM username, SCM password),
            "Use @var{username} and @var{password} as the credentials "
            "for @var{cred}, a client-side SRP credentials object.")
#define FUNC_NAME s_scm_gnutls_make_srp_client_credentials
{
  int err;
  gnutls_srp_client_credentials_t c_cred;
  char *c_username, *c_password;
  size_t c_username_len, c_password_len;

  c_cred = scm_to_gnutls_srp_client_credentials (cred, 1, FUNC_NAME);
  SCM_VALIDATE_STRING (2, username);
  SCM_VALIDATE_STRING (3, password);

  c_username_len = scm_c_string_length (username);
  c_password_len = scm_c_string_length (password);

  c_username = alloca (c_username_len + 1);
  c_password = alloca (c_password_len + 1);

  (void) scm_to_locale_stringbuf (username, c_username, c_username_len + 1);
  c_username[c_username_len] = '\0';
  (void) scm_to_locale_stringbuf (password, c_password, c_password_len + 1);
  c_password[c_password_len] = '\0';

  err = gnutls_srp_set_client_credentials (c_cred, c_username, c_password);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_server_session_srp_username,
            "server-session-srp-username",
            1, 0, 0,
            (SCM session),
            "Return the SRP username used in @var{session} (a server-side "
            "session).")
#define FUNC_NAME s_scm_gnutls_server_session_srp_username
{
  SCM result;
  const char *c_username;
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  c_username = gnutls_srp_server_get_username (c_session);

  if (EXPECT_FALSE (c_username == NULL))
    result = SCM_BOOL_F;
  else
    result = scm_from_locale_string (c_username);

  return (result);
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_srp_base64_encode, "srp-base64-encode",
            1, 0, 0,
            (SCM str),
            "Encode @var{str} using SRP's base64 algorithm.  Return "
            "the encoded string.")
#define FUNC_NAME s_scm_gnutls_srp_base64_encode
{
  int err;
  char *c_str, *c_result;
  size_t c_str_len, c_result_len, c_result_actual_len;
  gnutls_datum_t c_str_d;

  SCM_VALIDATE_STRING (1, str);

  c_str_len = scm_c_string_length (str);
  c_str = alloca (c_str_len + 1);
  (void) scm_to_locale_stringbuf (str, c_str, c_str_len + 1);
  c_str[c_str_len] = '\0';

  /* Typical size ratio is 4/3 so 3/2 is an upper bound.  */
  c_result_len = (c_str_len * 3) / 2;
  c_result = (char *) scm_malloc (c_result_len);
  if (EXPECT_FALSE (c_result == NULL))
    scm_gnutls_error (GNUTLS_E_MEMORY_ERROR, FUNC_NAME);

  c_str_d.data = (unsigned char *) c_str;
  c_str_d.size = c_str_len;

  do
    {
      c_result_actual_len = c_result_len;
      err = gnutls_srp_base64_encode (&c_str_d, c_result,
                                      &c_result_actual_len);
      if (err == GNUTLS_E_SHORT_MEMORY_BUFFER)
        {
          char *c_new_buf;

          c_new_buf = scm_realloc (c_result, c_result_len * 2);
          if (EXPECT_FALSE (c_new_buf == NULL))
            {
              free (c_result);
              scm_gnutls_error (GNUTLS_E_MEMORY_ERROR, FUNC_NAME);
            }
          else
            c_result = c_new_buf, c_result_len *= 2;
        }
    }
  while (EXPECT_FALSE (err == GNUTLS_E_SHORT_MEMORY_BUFFER));

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  if (c_result_actual_len + 1 < c_result_len)
    /* Shrink the buffer.  */
    c_result = scm_realloc (c_result, c_result_actual_len + 1);

  c_result[c_result_actual_len] = '\0';

  return (scm_take_locale_string (c_result));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_srp_base64_decode, "srp-base64-decode",
            1, 0, 0,
            (SCM str),
            "Decode @var{str}, an SRP-base64 encoded string, and return "
            "the decoded string.")
#define FUNC_NAME s_scm_gnutls_srp_base64_decode
{
  int err;
  char *c_str, *c_result;
  size_t c_str_len, c_result_len, c_result_actual_len;
  gnutls_datum_t c_str_d;

  SCM_VALIDATE_STRING (1, str);

  c_str_len = scm_c_string_length (str);
  c_str = alloca (c_str_len + 1);
  (void) scm_to_locale_stringbuf (str, c_str, c_str_len + 1);
  c_str[c_str_len] = '\0';

  /* We assume that the decoded string is smaller than the encoded
     string.  */
  c_result_len = c_str_len;
  c_result = alloca (c_result_len + 1);

  c_str_d.data = (unsigned char *) c_str;
  c_str_d.size = c_str_len;

  c_result_actual_len = c_result_len;
  err = gnutls_srp_base64_decode (&c_str_d, c_result, &c_result_actual_len);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  c_result[c_result_actual_len] = '\0';

  return (scm_from_locale_string (c_result));
}

#undef FUNC_NAME
#endif /* ENABLE_SRP */


/* PSK credentials.  */

SCM_DEFINE (scm_gnutls_make_psk_server_credentials,
            "make-psk-server-credentials",
            0, 0, 0, (void), "Return new PSK server credentials.")
#define FUNC_NAME s_scm_gnutls_make_psk_server_credentials
{
  int err;
  gnutls_psk_server_credentials_t c_cred;

  err = gnutls_psk_allocate_server_credentials (&c_cred);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_from_gnutls_psk_server_credentials (c_cred));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_psk_server_credentials_file_x,
            "set-psk-server-credentials-file!",
            2, 0, 0,
            (SCM cred, SCM file),
            "Use @var{file} as the password file for PSK server "
            "credentials @var{cred}.")
#define FUNC_NAME s_scm_gnutls_set_psk_server_credentials_file_x
{
  int err;
  gnutls_psk_server_credentials_t c_cred;
  char *c_file;
  size_t c_file_len;

  c_cred = scm_to_gnutls_psk_server_credentials (cred, 1, FUNC_NAME);
  SCM_VALIDATE_STRING (2, file);

  c_file_len = scm_c_string_length (file);
  c_file = alloca (c_file_len + 1);

  (void) scm_to_locale_stringbuf (file, c_file, c_file_len + 1);
  c_file[c_file_len] = '\0';

  err = gnutls_psk_set_server_credentials_file (c_cred, c_file);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_make_psk_client_credentials,
            "make-psk-client-credentials",
            0, 0, 0, (void), "Return a new PSK client credentials object.")
#define FUNC_NAME s_scm_gnutls_make_psk_client_credentials
{
  int err;
  gnutls_psk_client_credentials_t c_cred;

  err = gnutls_psk_allocate_client_credentials (&c_cred);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_from_gnutls_psk_client_credentials (c_cred));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_psk_client_credentials_x,
            "set-psk-client-credentials!",
            4, 0, 0,
            (SCM cred, SCM username, SCM key, SCM key_format),
            "Set the client credentials for @var{cred}, a PSK client "
            "credentials object.")
#define FUNC_NAME s_scm_gnutls_set_psk_client_credentials_x
{
  int err;
  gnutls_psk_client_credentials_t c_cred;
  gnutls_psk_key_flags c_key_format;
  scm_t_array_handle c_handle;
  const char *c_key;
  char *c_username;
  size_t c_username_len, c_key_len;
  gnutls_datum_t c_datum;

  c_cred = scm_to_gnutls_psk_client_credentials (cred, 1, FUNC_NAME);
  SCM_VALIDATE_STRING (2, username);
  SCM_VALIDATE_ARRAY (3, key);
  c_key_format = scm_to_gnutls_psk_key_format (key_format, 4, FUNC_NAME);

  c_username_len = scm_c_string_length (username);
  c_username = alloca (c_username_len + 1);

  (void) scm_to_locale_stringbuf (username, c_username, c_username_len + 1);
  c_username[c_username_len] = '\0';

  c_key = scm_gnutls_get_array (key, &c_handle, &c_key_len, FUNC_NAME);
  c_datum.data = (unsigned char *) c_key;
  c_datum.size = c_key_len;

  err = gnutls_psk_set_client_credentials (c_cred, c_username,
                                           &c_datum, c_key_format);
  scm_gnutls_release_array (&c_handle);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_server_session_psk_username,
            "server-session-psk-username",
            1, 0, 0,
            (SCM session),
            "Return the username associated with PSK server session "
            "@var{session}.")
#define FUNC_NAME s_scm_gnutls_server_session_psk_username
{
  SCM result;
  const char *c_username;
  gnutls_session_t c_session;

  c_session = scm_to_gnutls_session (session, 1, FUNC_NAME);
  c_username = gnutls_srp_server_get_username (c_session);

  if (EXPECT_FALSE (c_username == NULL))
    result = SCM_BOOL_F;
  else
    result = scm_from_locale_string (c_username);

  return (result);
}

#undef FUNC_NAME


/* X.509 certificates.  */

SCM_DEFINE (scm_gnutls_import_x509_certificate, "import-x509-certificate",
            2, 0, 0,
            (SCM data, SCM format),
            "Return a new X.509 certificate object resulting from the "
            "import of @var{data} (a uniform array) according to "
            "@var{format}.")
#define FUNC_NAME s_scm_gnutls_import_x509_certificate
{
  int err;
  gnutls_x509_crt_t c_cert;
  gnutls_x509_crt_fmt_t c_format;
  gnutls_datum_t c_data_d;
  scm_t_array_handle c_data_handle;
  const char *c_data;
  size_t c_data_len;

  SCM_VALIDATE_ARRAY (1, data);
  c_format = scm_to_gnutls_x509_certificate_format (format, 2, FUNC_NAME);

  c_data = scm_gnutls_get_array (data, &c_data_handle, &c_data_len,
                                 FUNC_NAME);
  c_data_d.data = (unsigned char *) c_data;
  c_data_d.size = c_data_len;

  err = gnutls_x509_crt_init (&c_cert);
  if (EXPECT_FALSE (err))
    {
      scm_gnutls_release_array (&c_data_handle);
      scm_gnutls_error (err, FUNC_NAME);
    }

  err = gnutls_x509_crt_import (c_cert, &c_data_d, c_format);
  scm_gnutls_release_array (&c_data_handle);

  if (EXPECT_FALSE (err))
    {
      gnutls_x509_crt_deinit (c_cert);
      scm_gnutls_error (err, FUNC_NAME);
    }

  return (scm_from_gnutls_x509_certificate (c_cert));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_import_x509_private_key, "import-x509-private-key",
            2, 0, 0,
            (SCM data, SCM format),
            "Return a new X.509 private key object resulting from the "
            "import of @var{data} (a uniform array) according to "
            "@var{format}.")
#define FUNC_NAME s_scm_gnutls_import_x509_private_key
{
  int err;
  gnutls_x509_privkey_t c_key;
  gnutls_x509_crt_fmt_t c_format;
  gnutls_datum_t c_data_d;
  scm_t_array_handle c_data_handle;
  const char *c_data;
  size_t c_data_len;

  SCM_VALIDATE_ARRAY (1, data);
  c_format = scm_to_gnutls_x509_certificate_format (format, 2, FUNC_NAME);

  c_data = scm_gnutls_get_array (data, &c_data_handle, &c_data_len,
                                 FUNC_NAME);
  c_data_d.data = (unsigned char *) c_data;
  c_data_d.size = c_data_len;

  err = gnutls_x509_privkey_init (&c_key);
  if (EXPECT_FALSE (err))
    {
      scm_gnutls_release_array (&c_data_handle);
      scm_gnutls_error (err, FUNC_NAME);
    }

  err = gnutls_x509_privkey_import (c_key, &c_data_d, c_format);
  scm_gnutls_release_array (&c_data_handle);

  if (EXPECT_FALSE (err))
    {
      gnutls_x509_privkey_deinit (c_key);
      scm_gnutls_error (err, FUNC_NAME);
    }

  return (scm_from_gnutls_x509_private_key (c_key));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_pkcs8_import_x509_private_key,
            "pkcs8-import-x509-private-key",
            2, 2, 0,
            (SCM data, SCM format, SCM pass, SCM encrypted),
            "Return a new X.509 private key object resulting from the "
            "import of @var{data} (a uniform array) according to "
            "@var{format}.  Optionally, if @var{pass} is not @code{#f}, "
            "it should be a string denoting a passphrase.  "
            "@var{encrypted} tells whether the private key is encrypted "
            "(@code{#t} by default).")
#define FUNC_NAME s_scm_gnutls_pkcs8_import_x509_private_key
{
  int err;
  gnutls_x509_privkey_t c_key;
  gnutls_x509_crt_fmt_t c_format;
  unsigned int c_flags;
  gnutls_datum_t c_data_d;
  scm_t_array_handle c_data_handle;
  const char *c_data;
  char *c_pass;
  size_t c_data_len, c_pass_len;

  SCM_VALIDATE_ARRAY (1, data);
  c_format = scm_to_gnutls_x509_certificate_format (format, 2, FUNC_NAME);
  if ((pass == SCM_UNDEFINED) || (scm_is_false (pass)))
    c_pass = NULL;
  else
    {
      c_pass_len = scm_c_string_length (pass);
      c_pass = alloca (c_pass_len + 1);
      (void) scm_to_locale_stringbuf (pass, c_pass, c_pass_len + 1);
      c_pass[c_pass_len] = '\0';
    }

  if (encrypted == SCM_UNDEFINED)
    c_flags = 0;
  else
    {
      SCM_VALIDATE_BOOL (4, encrypted);
      if (scm_is_true (encrypted))
        c_flags = 0;
      else
        c_flags = GNUTLS_PKCS8_PLAIN;
    }

  c_data = scm_gnutls_get_array (data, &c_data_handle, &c_data_len,
                                 FUNC_NAME);
  c_data_d.data = (unsigned char *) c_data;
  c_data_d.size = c_data_len;

  err = gnutls_x509_privkey_init (&c_key);
  if (EXPECT_FALSE (err))
    {
      scm_gnutls_release_array (&c_data_handle);
      scm_gnutls_error (err, FUNC_NAME);
    }

  err = gnutls_x509_privkey_import_pkcs8 (c_key, &c_data_d, c_format, c_pass,
                                          c_flags);
  scm_gnutls_release_array (&c_data_handle);

  if (EXPECT_FALSE (err))
    {
      gnutls_x509_privkey_deinit (c_key);
      scm_gnutls_error (err, FUNC_NAME);
    }

  return (scm_from_gnutls_x509_private_key (c_key));
}

#undef FUNC_NAME

/* Provide the body of a `get_dn' function.  */
#define X509_CERTIFICATE_DN_FUNCTION_BODY(get_the_dn)		\
  int err;							\
  gnutls_x509_crt_t c_cert;					\
  char *c_dn;							\
  size_t c_dn_len;						\
								\
  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);	\
								\
  /* Get the DN size.  */					\
  (void) get_the_dn (c_cert, NULL, &c_dn_len);			\
								\
  /* Get the DN itself.  */					\
  c_dn = alloca (c_dn_len);					\
  err = get_the_dn (c_cert, c_dn, &c_dn_len);			\
								\
  if (EXPECT_FALSE (err))					\
    scm_gnutls_error (err, FUNC_NAME);				\
								\
  /* XXX: The returned string is actually ASCII or UTF-8.  */	\
  return (scm_from_locale_string (c_dn));

SCM_DEFINE (scm_gnutls_x509_certificate_dn, "x509-certificate-dn",
            1, 0, 0,
            (SCM cert),
            "Return the distinguished name (DN) of X.509 certificate "
            "@var{cert}.  The form of the DN is as described in @uref{"
            "https://tools.ietf.org/html/rfc2253, RFC 2253}.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_dn
{
  X509_CERTIFICATE_DN_FUNCTION_BODY (gnutls_x509_crt_get_dn);
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_issuer_dn,
            "x509-certificate-issuer-dn",
            1, 0, 0,
            (SCM cert),
            "Return the distinguished name (DN) of X.509 certificate "
            "@var{cert}.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_issuer_dn
{
  X509_CERTIFICATE_DN_FUNCTION_BODY (gnutls_x509_crt_get_issuer_dn);
}

#undef FUNC_NAME

#undef X509_CERTIFICATE_DN_FUNCTION_BODY


/* Provide the body of a `get_dn_oid' function.  */
#define X509_CERTIFICATE_DN_OID_FUNCTION_BODY(get_dn_oid)		\
  int err;								\
  gnutls_x509_crt_t c_cert;						\
  unsigned int c_index;							\
  char *c_oid;								\
  size_t c_oid_actual_len, c_oid_len;					\
  SCM result;								\
									\
  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);		\
  c_index = scm_to_uint (index);					\
									\
  c_oid_len = 256;							\
  c_oid = scm_malloc (c_oid_len);					\
									\
  do									\
    {									\
      c_oid_actual_len = c_oid_len;					\
      err = get_dn_oid (c_cert, c_index, c_oid, &c_oid_actual_len);	\
      if (err == GNUTLS_E_SHORT_MEMORY_BUFFER)				\
	{								\
	  c_oid = scm_realloc (c_oid, c_oid_len * 2);			\
	  c_oid_len *= 2;						\
	}								\
    }									\
  while (err == GNUTLS_E_SHORT_MEMORY_BUFFER);				\
									\
  if (EXPECT_FALSE (err))						\
    {									\
      free (c_oid);							\
									\
      if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)			\
	result = SCM_BOOL_F;						\
      else								\
	scm_gnutls_error (err, FUNC_NAME);				\
    }									\
  else									\
    {									\
      if (c_oid_actual_len < c_oid_len)					\
	c_oid = scm_realloc (c_oid, c_oid_actual_len);			\
									\
      result = scm_take_locale_stringn (c_oid,				\
					c_oid_actual_len);		\
    }									\
									\
  return result;

SCM_DEFINE (scm_gnutls_x509_certificate_dn_oid, "x509-certificate-dn-oid",
            2, 0, 0,
            (SCM cert, SCM index),
            "Return OID (a string) at @var{index} from @var{cert}.  "
            "Return @code{#f} if no OID is available at @var{index}.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_dn_oid
{
  X509_CERTIFICATE_DN_OID_FUNCTION_BODY (gnutls_x509_crt_get_dn_oid);
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_issuer_dn_oid,
            "x509-certificate-issuer-dn-oid",
            2, 0, 0,
            (SCM cert, SCM index),
            "Return the OID (a string) at @var{index} from @var{cert}'s "
            "issuer DN.  Return @code{#f} if no OID is available at "
            "@var{index}.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_issuer_dn_oid
{
  X509_CERTIFICATE_DN_OID_FUNCTION_BODY (gnutls_x509_crt_get_issuer_dn_oid);
}

#undef FUNC_NAME

#undef X509_CERTIFICATE_DN_OID_FUNCTION_BODY


SCM_DEFINE (scm_gnutls_x509_certificate_matches_hostname_p,
            "x509-certificate-matches-hostname?",
            2, 0, 0,
            (SCM cert, SCM hostname),
            "Return true if @var{cert} matches @var{hostname}, a string "
            "denoting a DNS host name.  This is the basic implementation "
            "of @uref{https://tools.ietf.org/html/rfc2818, RFC 2818} (aka. "
            "HTTPS).")
#define FUNC_NAME s_scm_gnutls_x509_certificate_matches_hostname_p
{
  SCM result;
  gnutls_x509_crt_t c_cert;
  char *c_hostname;
  size_t c_hostname_len;

  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);
  SCM_VALIDATE_STRING (2, hostname);

  c_hostname_len = scm_c_string_length (hostname);
  c_hostname = alloca (c_hostname_len + 1);

  (void) scm_to_locale_stringbuf (hostname, c_hostname, c_hostname_len + 1);
  c_hostname[c_hostname_len] = '\0';

  if (gnutls_x509_crt_check_hostname (c_cert, c_hostname))
    result = SCM_BOOL_T;
  else
    result = SCM_BOOL_F;

  return result;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_signature_algorithm,
            "x509-certificate-signature-algorithm",
            1, 0, 0,
            (SCM cert),
            "Return the signature algorithm used by @var{cert} (i.e., "
            "one of the @code{sign-algorithm/} values).")
#define FUNC_NAME s_scm_gnutls_x509_certificate_signature_algorithm
{
  int c_result;
  gnutls_x509_crt_t c_cert;

  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);

  c_result = gnutls_x509_crt_get_signature_algorithm (c_cert);
  if (EXPECT_FALSE (c_result < 0))
    scm_gnutls_error (c_result, FUNC_NAME);

  return (scm_from_gnutls_sign_algorithm (c_result));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_public_key_algorithm,
            "x509-certificate-public-key-algorithm",
            1, 0, 0,
            (SCM cert),
            "Return two values: the public key algorithm (i.e., "
            "one of the @code{pk-algorithm/} values) of @var{cert} "
            "and the number of bits used.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_public_key_algorithm
{
  gnutls_x509_crt_t c_cert;
  gnutls_pk_algorithm_t c_pk;
  unsigned int c_bits;

  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);

  c_pk = gnutls_x509_crt_get_pk_algorithm (c_cert, &c_bits);

  return (scm_values (scm_list_2 (scm_from_gnutls_pk_algorithm (c_pk),
                                  scm_from_uint (c_bits))));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_key_usage,
            "x509-certificate-key-usage",
            1, 0, 0,
            (SCM cert),
            "Return the key usage of @var{cert} (i.e., a list of "
            "@code{key-usage/} values), or the empty list if @var{cert} "
            "does not contain such information.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_key_usage
{
  int err;
  SCM usage;
  gnutls_x509_crt_t c_cert;
  unsigned int c_usage, c_critical;

  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);

  err = gnutls_x509_crt_get_key_usage (c_cert, &c_usage, &c_critical);
  if (EXPECT_FALSE (err))
    {
      if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
        usage = SCM_EOL;
      else
        scm_gnutls_error (err, FUNC_NAME);
    }
  else
    usage = scm_from_gnutls_key_usage_flags (c_usage);

  return usage;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_version, "x509-certificate-version",
            1, 0, 0, (SCM cert), "Return the version of @var{cert}.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_version
{
  int c_result;
  gnutls_x509_crt_t c_cert;

  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);

  c_result = gnutls_x509_crt_get_version (c_cert);
  if (EXPECT_FALSE (c_result < 0))
    scm_gnutls_error (c_result, FUNC_NAME);

  return (scm_from_int (c_result));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_key_id, "x509-certificate-key-id",
            1, 0, 0,
            (SCM cert),
            "Return a statistically unique ID (a u8vector) for @var{cert} "
            "that depends on its public key parameters.  This is normally "
            "a 20-byte SHA-1 hash.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_key_id
{
  int err;
  SCM result;
  scm_t_array_handle c_id_handle;
  gnutls_x509_crt_t c_cert;
  scm_t_uint8 *c_id;
  size_t c_id_len = 20;

  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);

  result = scm_make_u8vector (scm_from_uint (c_id_len), SCM_INUM0);
  scm_array_get_handle (result, &c_id_handle);
  c_id = scm_array_handle_u8_writable_elements (&c_id_handle);

  err = gnutls_x509_crt_get_key_id (c_cert, 0, c_id, &c_id_len);
  scm_array_handle_release (&c_id_handle);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return result;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_authority_key_id,
            "x509-certificate-authority-key-id",
            1, 0, 0,
            (SCM cert),
            "Return the key ID (a u8vector) of the X.509 certificate "
            "authority of @var{cert}.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_authority_key_id
{
  int err;
  SCM result;
  scm_t_array_handle c_id_handle;
  gnutls_x509_crt_t c_cert;
  scm_t_uint8 *c_id;
  size_t c_id_len = 20;

  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);

  result = scm_make_u8vector (scm_from_uint (c_id_len), SCM_INUM0);
  scm_array_get_handle (result, &c_id_handle);
  c_id = scm_array_handle_u8_writable_elements (&c_id_handle);

  err = gnutls_x509_crt_get_authority_key_id (c_cert, c_id, &c_id_len, NULL);
  scm_array_handle_release (&c_id_handle);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return result;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_subject_key_id,
            "x509-certificate-subject-key-id",
            1, 0, 0,
            (SCM cert),
            "Return the subject key ID (a u8vector) for @var{cert}.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_subject_key_id
{
  int err;
  SCM result;
  scm_t_array_handle c_id_handle;
  gnutls_x509_crt_t c_cert;
  scm_t_uint8 *c_id;
  size_t c_id_len = 20;

  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);

  result = scm_make_u8vector (scm_from_uint (c_id_len), SCM_INUM0);
  scm_array_get_handle (result, &c_id_handle);
  c_id = scm_array_handle_u8_writable_elements (&c_id_handle);

  err = gnutls_x509_crt_get_subject_key_id (c_cert, c_id, &c_id_len, NULL);
  scm_array_handle_release (&c_id_handle);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return result;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_x509_certificate_subject_alternative_name,
            "x509-certificate-subject-alternative-name",
            2, 0, 0,
            (SCM cert, SCM index),
            "Return two values: the alternative name type for @var{cert} "
            "(i.e., one of the @code{x509-subject-alternative-name/} values) "
            "and the actual subject alternative name (a string) at "
            "@var{index}. Both values are @code{#f} if no alternative name "
            "is available at @var{index}.")
#define FUNC_NAME s_scm_gnutls_x509_certificate_subject_alternative_name
{
  int err;
  SCM result;
  gnutls_x509_crt_t c_cert;
  unsigned int c_index;
  char *c_name;
  size_t c_name_len = 512, c_name_actual_len;

  c_cert = scm_to_gnutls_x509_certificate (cert, 1, FUNC_NAME);
  c_index = scm_to_uint (index);

  c_name = scm_malloc (c_name_len);
  do
    {
      c_name_actual_len = c_name_len;
      err = gnutls_x509_crt_get_subject_alt_name (c_cert, c_index,
                                                  c_name, &c_name_actual_len,
                                                  NULL);
      if (err == GNUTLS_E_SHORT_MEMORY_BUFFER)
        {
          c_name = scm_realloc (c_name, c_name_len * 2);
          c_name_len *= 2;
        }
    }
  while (err == GNUTLS_E_SHORT_MEMORY_BUFFER);

  if (EXPECT_FALSE (err < 0))
    {
      free (c_name);

      if (err == GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE)
        result = scm_values (scm_list_2 (SCM_BOOL_F, SCM_BOOL_F));
      else
        scm_gnutls_error (err, FUNC_NAME);
    }
  else
    {
      if (c_name_actual_len < c_name_len)
        c_name = scm_realloc (c_name, c_name_actual_len);

      result =
        scm_values (scm_list_2
                    (scm_from_gnutls_x509_subject_alternative_name (err),
                     scm_take_locale_string (c_name)));
    }

  return result;
}

#undef FUNC_NAME


/* OpenPGP keys.  */


/* Maximum size we support for the name of OpenPGP keys.  */
#define GUILE_GNUTLS_MAX_OPENPGP_NAME_LENGTH  2048

SCM_DEFINE (scm_gnutls_import_openpgp_certificate,
            "import-openpgp-certificate", 2, 0, 0, (SCM data, SCM format),
            "Return a new OpenPGP certificate object resulting from the "
            "import of @var{data} (a uniform array) according to "
            "@var{format}.")
#define FUNC_NAME s_scm_gnutls_import_openpgp_certificate
{
  int err;
  gnutls_openpgp_crt_t c_key;
  gnutls_openpgp_crt_fmt_t c_format;
  gnutls_datum_t c_data_d;
  scm_t_array_handle c_data_handle;
  const char *c_data;
  size_t c_data_len;

  SCM_VALIDATE_ARRAY (1, data);
  c_format = scm_to_gnutls_openpgp_certificate_format (format, 2, FUNC_NAME);

  c_data = scm_gnutls_get_array (data, &c_data_handle, &c_data_len,
                                 FUNC_NAME);
  c_data_d.data = (unsigned char *) c_data;
  c_data_d.size = c_data_len;

  err = gnutls_openpgp_crt_init (&c_key);
  if (EXPECT_FALSE (err))
    {
      scm_gnutls_release_array (&c_data_handle);
      scm_gnutls_error (err, FUNC_NAME);
    }

  err = gnutls_openpgp_crt_import (c_key, &c_data_d, c_format);
  scm_gnutls_release_array (&c_data_handle);

  if (EXPECT_FALSE (err))
    {
      gnutls_openpgp_crt_deinit (c_key);
      scm_gnutls_error (err, FUNC_NAME);
    }

  return (scm_from_gnutls_openpgp_certificate (c_key));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_import_openpgp_private_key,
            "import-openpgp-private-key", 2, 1, 0, (SCM data, SCM format,
                                                    SCM pass),
            "Return a new OpenPGP private key object resulting from the "
            "import of @var{data} (a uniform array) according to "
            "@var{format}.  Optionally, a passphrase may be provided.")
#define FUNC_NAME s_scm_gnutls_import_openpgp_private_key
{
  int err;
  gnutls_openpgp_privkey_t c_key;
  gnutls_openpgp_crt_fmt_t c_format;
  gnutls_datum_t c_data_d;
  scm_t_array_handle c_data_handle;
  const char *c_data;
  char *c_pass;
  size_t c_data_len, c_pass_len;

  SCM_VALIDATE_ARRAY (1, data);
  c_format = scm_to_gnutls_openpgp_certificate_format (format, 2, FUNC_NAME);
  if ((pass == SCM_UNDEFINED) || (scm_is_false (pass)))
    c_pass = NULL;
  else
    {
      c_pass_len = scm_c_string_length (pass);
      c_pass = alloca (c_pass_len + 1);
      (void) scm_to_locale_stringbuf (pass, c_pass, c_pass_len + 1);
      c_pass[c_pass_len] = '\0';
    }

  c_data = scm_gnutls_get_array (data, &c_data_handle, &c_data_len,
                                 FUNC_NAME);
  c_data_d.data = (unsigned char *) c_data;
  c_data_d.size = c_data_len;

  err = gnutls_openpgp_privkey_init (&c_key);
  if (EXPECT_FALSE (err))
    {
      scm_gnutls_release_array (&c_data_handle);
      scm_gnutls_error (err, FUNC_NAME);
    }

  err = gnutls_openpgp_privkey_import (c_key, &c_data_d, c_format, c_pass,
                                       0 /* currently unused */ );
  scm_gnutls_release_array (&c_data_handle);

  if (EXPECT_FALSE (err))
    {
      gnutls_openpgp_privkey_deinit (c_key);
      scm_gnutls_error (err, FUNC_NAME);
    }

  return (scm_from_gnutls_openpgp_private_key (c_key));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_certificate_id, "openpgp-certificate-id",
            1, 0, 0,
            (SCM key),
            "Return the ID (an 8-element u8vector) of certificate "
            "@var{key}.")
#define FUNC_NAME s_scm_gnutls_openpgp_certificate_id
{
  int err;
  unsigned char *c_id;
  gnutls_openpgp_crt_t c_key;

  c_key = scm_to_gnutls_openpgp_certificate (key, 1, FUNC_NAME);

  c_id = (unsigned char *) malloc (8);
  if (c_id == NULL)
    scm_gnutls_error (GNUTLS_E_MEMORY_ERROR, FUNC_NAME);

  err = gnutls_openpgp_crt_get_key_id (c_key, c_id);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_take_u8vector (c_id, 8));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_certificate_id_x, "openpgp-certificate-id!",
            2, 0, 0,
            (SCM key, SCM id),
            "Store the ID (an 8 byte sequence) of certificate "
            "@var{key} in @var{id} (a u8vector).")
#define FUNC_NAME s_scm_gnutls_openpgp_certificate_id_x
{
  int err;
  char *c_id;
  scm_t_array_handle c_id_handle;
  size_t c_id_size;
  gnutls_openpgp_crt_t c_key;

  c_key = scm_to_gnutls_openpgp_certificate (key, 1, FUNC_NAME);
  c_id = scm_gnutls_get_writable_array (id, &c_id_handle, &c_id_size,
                                        FUNC_NAME);

  if (EXPECT_FALSE (c_id_size < 8))
    {
      scm_gnutls_release_array (&c_id_handle);
      scm_misc_error (FUNC_NAME, "ID vector too small: ~A", scm_list_1 (id));
    }

  err = gnutls_openpgp_crt_get_key_id (c_key, (unsigned char *) c_id);
  scm_gnutls_release_array (&c_id_handle);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_certificate_fingerpint_x,
            "openpgp-certificate-fingerprint!",
            2, 0, 0,
            (SCM key, SCM fpr),
            "Store in @var{fpr} (a u8vector) the fingerprint of @var{key}.  "
            "Return the number of bytes stored in @var{fpr}.")
#define FUNC_NAME s_scm_gnutls_openpgp_certificate_fingerpint_x
{
  int err;
  gnutls_openpgp_crt_t c_key;
  char *c_fpr;
  scm_t_array_handle c_fpr_handle;
  size_t c_fpr_len, c_actual_len = 0;

  c_key = scm_to_gnutls_openpgp_certificate (key, 1, FUNC_NAME);
  SCM_VALIDATE_ARRAY (2, fpr);

  c_fpr = scm_gnutls_get_writable_array (fpr, &c_fpr_handle, &c_fpr_len,
                                         FUNC_NAME);

  err = gnutls_openpgp_crt_get_fingerprint (c_key, c_fpr, &c_actual_len);
  scm_gnutls_release_array (&c_fpr_handle);

  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_from_size_t (c_actual_len));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_certificate_fingerprint,
            "openpgp-certificate-fingerprint",
            1, 0, 0,
            (SCM key),
            "Return a new u8vector denoting the fingerprint of " "@var{key}.")
#define FUNC_NAME s_scm_gnutls_openpgp_certificate_fingerprint
{
  int err;
  gnutls_openpgp_crt_t c_key;
  unsigned char *c_fpr;
  size_t c_fpr_len, c_actual_len;

  c_key = scm_to_gnutls_openpgp_certificate (key, 1, FUNC_NAME);

  /* V4 fingerprints are 160-bit SHA-1 hashes (see RFC2440).  */
  c_fpr_len = 20;
  c_fpr = (unsigned char *) malloc (c_fpr_len);
  if (EXPECT_FALSE (c_fpr == NULL))
    scm_gnutls_error (GNUTLS_E_MEMORY_ERROR, FUNC_NAME);

  do
    {
      c_actual_len = 0;
      err = gnutls_openpgp_crt_get_fingerprint (c_key, c_fpr, &c_actual_len);
      if (err == GNUTLS_E_SHORT_MEMORY_BUFFER)
        {
          /* Grow C_FPR.  */
          unsigned char *c_new;

          c_new = (unsigned char *) realloc (c_fpr, c_fpr_len * 2);
          if (EXPECT_FALSE (c_new == NULL))
            {
              free (c_fpr);
              scm_gnutls_error (GNUTLS_E_MEMORY_ERROR, FUNC_NAME);
            }
          else
            {
              c_fpr_len *= 2;
              c_fpr = c_new;
            }
        }
    }
  while (err == GNUTLS_E_SHORT_MEMORY_BUFFER);

  if (EXPECT_FALSE (err))
    {
      free (c_fpr);
      scm_gnutls_error (err, FUNC_NAME);
    }

  if (c_actual_len < c_fpr_len)
    /* Shrink C_FPR.  */
    c_fpr = realloc (c_fpr, c_actual_len);

  return (scm_take_u8vector (c_fpr, c_actual_len));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_certificate_name, "openpgp-certificate-name",
            2, 0, 0,
            (SCM key, SCM index),
            "Return the @var{index}th name of @var{key}.")
#define FUNC_NAME s_scm_gnutls_openpgp_certificate_name
{
  int err;
  gnutls_openpgp_crt_t c_key;
  int c_index;
  char c_name[GUILE_GNUTLS_MAX_OPENPGP_NAME_LENGTH];
  size_t c_name_len = sizeof (c_name);

  c_key = scm_to_gnutls_openpgp_certificate (key, 1, FUNC_NAME);
  c_index = scm_to_int (index);

  err = gnutls_openpgp_crt_get_name (c_key, c_index, c_name, &c_name_len);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  /* XXX: The name is really UTF-8.  */
  return (scm_from_locale_string (c_name));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_certificate_names, "openpgp-certificate-names",
            1, 0, 0, (SCM key), "Return the list of names for @var{key}.")
#define FUNC_NAME s_scm_gnutls_openpgp_certificate_names
{
  int err;
  SCM result = SCM_EOL;
  gnutls_openpgp_crt_t c_key;
  int c_index = 0;
  char c_name[GUILE_GNUTLS_MAX_OPENPGP_NAME_LENGTH];
  size_t c_name_len = sizeof (c_name);

  c_key = scm_to_gnutls_openpgp_certificate (key, 1, FUNC_NAME);

  do
    {
      err = gnutls_openpgp_crt_get_name (c_key, c_index, c_name, &c_name_len);
      if (!err)
        {
          result = scm_cons (scm_from_locale_string (c_name), result);
          c_index++;
        }
    }
  while (!err);

  if (EXPECT_FALSE (err != GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_reverse_x (result, SCM_EOL));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_certificate_algorithm,
            "openpgp-certificate-algorithm",
            1, 0, 0,
            (SCM key),
            "Return two values: the certificate algorithm used by "
            "@var{key} and the number of bits used.")
#define FUNC_NAME s_scm_gnutls_openpgp_certificate_algorithm
{
  gnutls_openpgp_crt_t c_key;
  unsigned int c_bits;
  gnutls_pk_algorithm_t c_algo;

  c_key = scm_to_gnutls_openpgp_certificate (key, 1, FUNC_NAME);
  c_algo = gnutls_openpgp_crt_get_pk_algorithm (c_key, &c_bits);

  return (scm_values (scm_list_2 (scm_from_gnutls_pk_algorithm (c_algo),
                                  scm_from_uint (c_bits))));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_certificate_version,
            "openpgp-certificate-version",
            1, 0, 0,
            (SCM key),
            "Return the version of the OpenPGP message format (RFC2440) "
            "honored by @var{key}.")
#define FUNC_NAME s_scm_gnutls_openpgp_certificate_version
{
  int c_version;
  gnutls_openpgp_crt_t c_key;

  c_key = scm_to_gnutls_openpgp_certificate (key, 1, FUNC_NAME);
  c_version = gnutls_openpgp_crt_get_version (c_key);

  return (scm_from_int (c_version));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_certificate_usage, "openpgp-certificate-usage",
            1, 0, 0,
            (SCM key),
            "Return a list of values denoting the key usage of @var{key}.")
#define FUNC_NAME s_scm_gnutls_openpgp_certificate_usage
{
  int err;
  unsigned int c_usage = 0;
  gnutls_openpgp_crt_t c_key;

  c_key = scm_to_gnutls_openpgp_certificate (key, 1, FUNC_NAME);

  err = gnutls_openpgp_crt_get_key_usage (c_key, &c_usage);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return (scm_from_gnutls_key_usage_flags (c_usage));
}

#undef FUNC_NAME



/* OpenPGP keyrings.  */

SCM_DEFINE (scm_gnutls_import_openpgp_keyring, "import-openpgp-keyring",
            2, 0, 0,
            (SCM data, SCM format),
            "Import @var{data} (a u8vector) according to @var{format} "
            "and return the imported keyring.")
#define FUNC_NAME s_scm_gnutls_import_openpgp_keyring
{
  int err;
  gnutls_openpgp_keyring_t c_keyring;
  gnutls_openpgp_crt_fmt_t c_format;
  gnutls_datum_t c_data_d;
  scm_t_array_handle c_data_handle;
  const char *c_data;
  size_t c_data_len;

  SCM_VALIDATE_ARRAY (1, data);
  c_format = scm_to_gnutls_openpgp_certificate_format (format, 2, FUNC_NAME);

  c_data = scm_gnutls_get_array (data, &c_data_handle, &c_data_len,
                                 FUNC_NAME);

  c_data_d.data = (unsigned char *) c_data;
  c_data_d.size = c_data_len;

  err = gnutls_openpgp_keyring_init (&c_keyring);
  if (EXPECT_FALSE (err))
    {
      scm_gnutls_release_array (&c_data_handle);
      scm_gnutls_error (err, FUNC_NAME);
    }

  err = gnutls_openpgp_keyring_import (c_keyring, &c_data_d, c_format);
  scm_gnutls_release_array (&c_data_handle);

  if (EXPECT_FALSE (err))
    {
      gnutls_openpgp_keyring_deinit (c_keyring);
      scm_gnutls_error (err, FUNC_NAME);
    }

  return (scm_from_gnutls_openpgp_keyring (c_keyring));
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_openpgp_keyring_contains_key_id_p,
            "openpgp-keyring-contains-key-id?",
            2, 0, 0,
            (SCM keyring, SCM id),
            "Return @code{#f} if key ID @var{id} is in @var{keyring}, "
            "@code{#f} otherwise.")
#define FUNC_NAME s_scm_gnutls_openpgp_keyring_contains_key_id_p
{
  int c_result;
  gnutls_openpgp_keyring_t c_keyring;
  scm_t_array_handle c_id_handle;
  const char *c_id;
  size_t c_id_len;

  c_keyring = scm_to_gnutls_openpgp_keyring (keyring, 1, FUNC_NAME);
  SCM_VALIDATE_ARRAY (1, id);

  c_id = scm_gnutls_get_array (id, &c_id_handle, &c_id_len, FUNC_NAME);
  if (EXPECT_FALSE (c_id_len != 8))
    {
      scm_gnutls_release_array (&c_id_handle);
      scm_wrong_type_arg (FUNC_NAME, 1, id);
    }

  c_result = gnutls_openpgp_keyring_check_id (c_keyring,
                                              (unsigned char *) c_id,
                                              0 /* unused */ );

  scm_gnutls_release_array (&c_id_handle);

  return (scm_from_bool (c_result == 0));
}

#undef FUNC_NAME


/* OpenPGP certificates.  */

SCM_DEFINE (scm_gnutls_set_certificate_credentials_openpgp_keys_x,
            "set-certificate-credentials-openpgp-keys!",
            3, 0, 0,
            (SCM cred, SCM pub, SCM sec),
            "Use certificate @var{pub} and secret key @var{sec} in "
            "certificate credentials @var{cred}.")
#define FUNC_NAME s_scm_gnutls_set_certificate_credentials_openpgp_keys_x
{
  int err;
  gnutls_certificate_credentials_t c_cred;
  gnutls_openpgp_crt_t c_pub;
  gnutls_openpgp_privkey_t c_sec;

  c_cred = scm_to_gnutls_certificate_credentials (cred, 1, FUNC_NAME);
  c_pub = scm_to_gnutls_openpgp_certificate (pub, 2, FUNC_NAME);
  c_sec = scm_to_gnutls_openpgp_private_key (sec, 3, FUNC_NAME);

  err = gnutls_certificate_set_openpgp_key (c_cred, c_pub, c_sec);
  if (EXPECT_FALSE (err))
    scm_gnutls_error (err, FUNC_NAME);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME



/* Debugging.  */

static SCM log_procedure = SCM_BOOL_F;

static void
scm_gnutls_log (int level, const char *str)
{
  if (scm_is_true (log_procedure))
    (void) scm_call_2 (log_procedure, scm_from_int (level),
                       scm_from_locale_string (str));
}

SCM_DEFINE (scm_gnutls_set_log_procedure_x, "set-log-procedure!",
            1, 0, 0,
            (SCM proc),
            "Use @var{proc} (a two-argument procedure) as the global "
            "GnuTLS log procedure.")
#define FUNC_NAME s_scm_gnutls_set_log_procedure_x
{
  SCM_VALIDATE_PROC (1, proc);

  if (scm_is_true (log_procedure))
    (void) scm_gc_unprotect_object (log_procedure);

  log_procedure = scm_gc_protect_object (proc);
  gnutls_global_set_log_function (scm_gnutls_log);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME

SCM_DEFINE (scm_gnutls_set_log_level_x, "set-log-level!", 1, 0, 0,
            (SCM level),
            "Enable GnuTLS logging up to @var{level} (an integer).")
#define FUNC_NAME s_scm_gnutls_set_log_level_x
{
  unsigned int c_level;

  c_level = scm_to_uint (level);
  gnutls_global_set_log_level (c_level);

  return SCM_UNSPECIFIED;
}

#undef FUNC_NAME


/* Initialization.  */

void
scm_init_gnutls (void)
{
#include "core.x"

  /* Use Guile's allocation routines, which will run the GC if need be.  */
  (void) gnutls_global_init ();

  scm_gnutls_define_enums ();

  scm_init_gnutls_error ();

  scm_init_gnutls_session_record_port_type ();

  weak_refs = scm_make_weak_key_hash_table (scm_from_int (42));
  weak_refs = scm_permanent_object (weak_refs);
}
