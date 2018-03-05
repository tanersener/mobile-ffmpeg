;;; GnuTLS-extra --- Guile bindings for GnuTLS-EXTRA.
;;; Copyright (C) 2007-2012 Free Software Foundation, Inc.
;;;
;;; GnuTLS-extra is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; GnuTLS-extra is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with GnuTLS-EXTRA; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
;;; USA.

;;; Written by Ludovic Courtès <ludo@gnu.org>

(define-module (gnutls extra)

  :use-module (gnutls)

  :re-export (;; OpenPGP keys
           openpgp-certificate? openpgp-private-key?
           import-openpgp-certificate import-openpgp-private-key
           openpgp-certificate-id openpgp-certificate-id!
           openpgp-certificate-fingerprint openpgp-certificate-fingerprint!
           openpgp-certificate-name openpgp-certificate-names
           openpgp-certificate-algorithm openpgp-certificate-version
           openpgp-certificate-usage

           ;; OpenPGP keyrings
           openpgp-keyring? import-openpgp-keyring
           openpgp-keyring-contains-key-id?

           ;; certificate credentials
           set-certificate-credentials-openpgp-keys!

           ;; enum->string functions
           openpgp-certificate-format->string

           ;; enum values
           openpgp-certificate-format/raw
           openpgp-certificate-format/base64))



;;;
;;; This module will be removed in a future version.
;;;

(issue-deprecation-warning
 "The (gnutls extra) module is deprecated; use (gnutls) instead")


;;;
;;; Aliases kept for backward compatibility with GnuTLS 2.0.x.  These aliases
;;; are deprecated in 2.2 and should be removed in 2.4.x.
;;;

(define-public openpgp-public-key? openpgp-certificate?)
(define-public import-openpgp-public-key import-openpgp-certificate)
(define-public openpgp-public-key-id openpgp-certificate-id)
(define-public openpgp-public-key-id! openpgp-certificate-id!)
(define-public openpgp-public-key-fingerprint openpgp-certificate-fingerprint)
(define-public openpgp-public-key-fingerprint! openpgp-certificate-fingerprint!)
(define-public openpgp-public-key-name openpgp-certificate-name)
(define-public openpgp-public-key-names openpgp-certificate-names)
(define-public openpgp-public-key-algorithm openpgp-certificate-algorithm)
(define-public openpgp-public-key-version openpgp-certificate-version)
(define-public openpgp-public-key-usage openpgp-certificate-usage)

(define-public openpgp-key-format->string openpgp-certificate-format->string)
(define-public openpgp-key-format/raw openpgp-certificate-format/raw)
(define-public openpgp-key-format/base64 openpgp-certificate-format/base64)


;;; Local Variables:
;;; mode: scheme
;;; coding: latin-1
;;; End:
