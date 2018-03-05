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

;;; Written by Ludovic Courtès <ludo@chbouib.org>.


;;;
;;; Exercise the OpenPGP key API part of GnuTLS-extra.
;;;

(use-modules (gnutls)
             (gnutls build tests)
             (srfi srfi-1)
             (srfi srfi-4)
             (srfi srfi-11))

(define %certificate-file
  (search-path %load-path "openpgp-elg-pub.asc"))

(define %private-key-file
  (search-path %load-path "openpgp-elg-sec.asc"))

(define %key-id
  ;; Change me if you change the key files.
  '#u8(#xbd #x57 #x2c #xdc #xcc #xc0 #x7c #x35))

(define (file-size file)
  (stat:size (stat file)))


(run-test
    (lambda ()
      (let ((raw-pubkey  (make-u8vector (file-size %certificate-file)))
            (raw-privkey (make-u8vector (file-size %private-key-file))))

        (uniform-vector-read! raw-pubkey (open-input-file %certificate-file))
        (uniform-vector-read! raw-privkey (open-input-file %private-key-file))

        (let ((pub (import-openpgp-certificate raw-pubkey
                                           openpgp-certificate-format/base64))
              (sec (import-openpgp-private-key raw-privkey
                                           openpgp-certificate-format/base64)))

          (and (openpgp-certificate? pub)
               (openpgp-private-key? sec)
               (equal? (openpgp-certificate-id pub) %key-id)
               (u8vector? (openpgp-certificate-fingerprint pub))
               (every string? (openpgp-certificate-names pub))
               (member (openpgp-certificate-version pub) '(3 4))
               (list? (openpgp-certificate-usage pub))
               (let-values (((pk bits)
                             (openpgp-certificate-algorithm pub)))
                 (and (string? (pk-algorithm->string pk))
                      (number? bits))))))))

;;; arch-tag: 2ee2a377-7f4d-4031-92a8-275090e4f83d
