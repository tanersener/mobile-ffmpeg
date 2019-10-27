;;; GnuTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007-2012, 2019 Free Software Foundation, Inc.
;;;
;;; GnuTLS is free software; you can redistribute it and/or
;;; modify it under the terms of the GNU Lesser General Public
;;; License as published by the Free Software Foundation; either
;;; version 2.1 of the License, or (at your option) any later version.
;;;
;;; GnuTLS is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
;;; Lesser General Public License for more details.
;;;
;;; You should have received a copy of the GNU Lesser General Public
;;; License along with GnuTLS; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

;;; Written by Ludovic Courtès <ludo@chbouib.org>.


;;;
;;; Test the error/exception mechanism.
;;;

(use-modules (gnutls)
             (gnutls build tests))

(run-test
 (lambda ()
   (and (fatal-error? error/hash-failed)
        (not (fatal-error? error/reauth-request))

        (let ((s (make-session connection-end/server)))
          (catch 'gnutls-error
            (lambda ()
              (handshake s))
            (lambda (key err function . currently-unused)
              (and (eq? key 'gnutls-error)
                   err
                   (fatal-error? err)
                   (string? (error->string err))
                   (eq? function 'handshake))))))))

;;; arch-tag: 73ed6229-378d-4a12-a5c6-4c2586c6e3a2
