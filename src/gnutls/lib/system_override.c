/*
 * Copyright (C) 2000-2012 Free Software Foundation, Inc.
 *
 * Author: Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>
 *
 */

/* This file contains function that will override the 
 * default berkeley sockets API per session.
 */

#include "gnutls_int.h"
#include "errors.h"
#include <num.h>
#include <record.h>
#include <buffers.h>
#include <mbuffers.h>
#include <state.h>
#include <dtls.h>
#include <system.h>

#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif

/**
 * gnutls_transport_set_errno:
 * @session: is a #gnutls_session_t type.
 * @err: error value to store in session-specific errno variable.
 *
 * Store @err in the session-specific errno variable.  Useful values
 * for @err are EINTR, EAGAIN and EMSGSIZE, other values are treated will be
 * treated as real errors in the push/pull function.
 *
 * This function is useful in replacement push and pull functions set by
 * gnutls_transport_set_push_function() and
 * gnutls_transport_set_pull_function() under Windows, where the
 * replacements may not have access to the same @errno
 * variable that is used by GnuTLS (e.g., the application is linked to
 * msvcr71.dll and gnutls is linked to msvcrt.dll).
 *
 * This function is unreliable if you are using the same
 * @session in different threads for sending and receiving.
 *
 **/
void gnutls_transport_set_errno(gnutls_session_t session, int err)
{
	session->internals.errnum = err;
}

/**
 * gnutls_transport_set_pull_function:
 * @session: is a #gnutls_session_t type.
 * @pull_func: a callback function similar to read()
 *
 * This is the function where you set a function for gnutls to receive
 * data.  Normally, if you use berkeley style sockets, do not need to
 * use this function since the default recv(2) will probably be ok.
 * The callback should return 0 on connection termination, a positive
 * number indicating the number of bytes received, and -1 on error.
 *
 * @gnutls_pull_func is of the form,
 * ssize_t (*gnutls_pull_func)(gnutls_transport_ptr_t, void*, size_t);
 **/
void
gnutls_transport_set_pull_function(gnutls_session_t session,
				   gnutls_pull_func pull_func)
{
	session->internals.pull_func = pull_func;
}

/**
 * gnutls_transport_set_pull_timeout_function:
 * @session: is a #gnutls_session_t type.
 * @func: a callback function
 *
 * This is the function where you set a function for gnutls to know
 * whether data are ready to be received. It should wait for data a
 * given time frame in milliseconds. The callback should return 0 on 
 * timeout, a positive number if data can be received, and -1 on error.
 * You'll need to override this function if select() is not suitable
 * for the provided transport calls.
 *
 * As with select(), if the timeout value is zero the callback should return
 * zero if no data are immediately available. The special value
 * %GNUTLS_INDEFINITE_TIMEOUT indicates that the callback should wait indefinitely
 * for data.
 *
 * @gnutls_pull_timeout_func is of the form,
 * int (*gnutls_pull_timeout_func)(gnutls_transport_ptr_t, unsigned int ms);
 *
 * This callback is necessary when gnutls_handshake_set_timeout() or 
 * gnutls_record_set_timeout() are set, and for calculating the DTLS mode
 * timeouts.
 *
 * In short, this callback should be set when a custom pull function is
 * registered. The callback will not be used when the session is in TLS mode with
 * non-blocking sockets. That is, when %GNUTLS_NONBLOCK is specified for a TLS
 * session in gnutls_init(). For compatibility with future GnuTLS versions
 * it is recommended to always set this function when a custom pull function
 * is registered.
 *
 * The helper function gnutls_system_recv_timeout() is provided to
 * simplify writing callbacks. 
 *
 * Since: 3.0
 **/
void
gnutls_transport_set_pull_timeout_function(gnutls_session_t session,
					   gnutls_pull_timeout_func func)
{
	session->internals.pull_timeout_func = func;
}

/**
 * gnutls_transport_set_push_function:
 * @session: is a #gnutls_session_t type.
 * @push_func: a callback function similar to write()
 *
 * This is the function where you set a push function for gnutls to
 * use in order to send data.  If you are going to use berkeley style
 * sockets, you do not need to use this function since the default
 * send(2) will probably be ok.  Otherwise you should specify this
 * function for gnutls to be able to send data.
 * The callback should return a positive number indicating the
 * bytes sent, and -1 on error.
 *
 * @push_func is of the form,
 * ssize_t (*gnutls_push_func)(gnutls_transport_ptr_t, const void*, size_t);
 *
 **/
void
gnutls_transport_set_push_function(gnutls_session_t session,
				   gnutls_push_func push_func)
{
	session->internals.push_func = push_func;
	session->internals.vec_push_func = NULL;
}

/**
 * gnutls_transport_set_vec_push_function:
 * @session: is a #gnutls_session_t type.
 * @vec_func: a callback function similar to writev()
 *
 * Using this function you can override the default writev(2)
 * function for gnutls to send data. Setting this callback 
 * instead of gnutls_transport_set_push_function() is recommended
 * since it introduces less overhead in the TLS handshake process.
 *
 * @vec_func is of the form,
 * ssize_t (*gnutls_vec_push_func) (gnutls_transport_ptr_t, const giovec_t * iov, int iovcnt);
 *
 * Since: 2.12.0
 **/
void
gnutls_transport_set_vec_push_function(gnutls_session_t session,
				       gnutls_vec_push_func vec_func)
{
	session->internals.push_func = NULL;
	session->internals.vec_push_func = vec_func;
}

/**
 * gnutls_transport_set_errno_function:
 * @session: is a #gnutls_session_t type.
 * @errno_func: a callback function similar to write()
 *
 * This is the function where you set a function to retrieve errno
 * after a failed push or pull operation.
 *
 * @errno_func is of the form,
 * int (*gnutls_errno_func)(gnutls_transport_ptr_t);
 * and should return the errno.
 *
 * Since: 2.12.0
 **/
void
gnutls_transport_set_errno_function(gnutls_session_t session,
				    gnutls_errno_func errno_func)
{
	session->internals.errno_func = errno_func;
}
