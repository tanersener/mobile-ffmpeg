;;; GnuTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007-2012 Free Software Foundation, Inc.
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
;;; Test SRP base64 encoding and decoding.
;;;

(use-modules (gnutls)
             (gnutls build tests))

(define %message
  "GnuTLS is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.")

(run-test
 (lambda ()
   (let ((encoded (srp-base64-encode %message)))
     (and (string? encoded)
          (string=? (srp-base64-decode encoded)
                    %message)))))


;;; arch-tag: ea1534a5-d513-4208-9a75-54bd4710f915
