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
;;; Exercise the DH/RSA PKCS3/PKCS1 export/import functions.
;;;

(use-modules (gnutls)
             (gnutls build tests)
             (srfi srfi-4))

(define (import-something import-proc file fmt)
  (let* ((path (search-path %load-path file))
         (size (stat:size (stat path)))
         (raw  (make-u8vector size)))
    (uniform-vector-read! raw (open-input-file path))
    (import-proc raw fmt)))

(define (import-dh-params file)
  (import-something pkcs3-import-dh-parameters file
                    x509-certificate-format/pem))

(run-test
    (lambda ()
      (let* ((dh-params (import-dh-params "dh-parameters.pem"))
             (export
              (pkcs3-export-dh-parameters dh-params
                                          x509-certificate-format/pem)))
        (and (u8vector? export)
             (let ((import
                    (pkcs3-import-dh-parameters export
                                                x509-certificate-format/pem)))
               (dh-parameters? import))))))

;;; arch-tag: adff0f07-479e-421e-b47f-8956e06b9902
