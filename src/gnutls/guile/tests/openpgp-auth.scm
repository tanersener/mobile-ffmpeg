;;; GnuTLS-extra --- Guile bindings for GnuTLS-EXTRA.
;;; Copyright (C) 2007-2014, 2016 Free Software Foundation, Inc.
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
;;; Test session establishment using OpenPGP certificate authentication.
;;;

(use-modules (gnutls)
             (gnutls build tests)
             (srfi srfi-4))


;; TLS session settings.
(define priorities
  "NONE:+VERS-TLS-ALL:+CTYPE-OPENPGP:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+DHE-RSA:+DHE-DSS:+COMP-ALL")

;; Message sent by the client.
(define %message
  (cons "hello, world!" (iota 4444)))

(define (import-something import-proc file fmt)
  (let* ((path (search-path %load-path file))
         (size (stat:size (stat path)))
         (raw  (make-u8vector size)))
    (uniform-vector-read! raw (open-input-file path))
    (import-proc raw fmt)))

(define (import-key import-proc file)
  (import-something import-proc file openpgp-certificate-format/base64))

(define (import-dh-params file)
  (import-something pkcs3-import-dh-parameters file
                    x509-certificate-format/pem))

;; Debugging.
;; (set-log-level! 3)
;; (set-log-procedure! (lambda (level str)
;;                       (format #t "[~a|~a] ~a" (getpid) level str)))

(run-test
 (lambda ()
   (let ((socket-pair (socketpair PF_UNIX SOCK_STREAM 0))
         (pub         (import-key import-openpgp-certificate
                                  "openpgp-pub.asc"))
         (sec         (import-key import-openpgp-private-key
                                  "openpgp-sec.asc")))
     (with-child-process pid
       ;; server-side
       (let ((server (make-session connection-end/server))
             (dh     (import-dh-params "dh-parameters.pem")))
         (set-session-priorities! server priorities)
         (set-server-session-certificate-request! server
                                                  certificate-request/require)

         (set-session-transport-fd! server (port->fdes (cdr socket-pair)))
         (let ((cred (make-certificate-credentials)))
           (set-certificate-credentials-dh-parameters! cred dh)
           (set-certificate-credentials-openpgp-keys! cred pub sec)
           (set-session-credentials! server cred))
         (set-session-dh-prime-bits! server 1024)

         (handshake server)
         (let ((msg (read (session-record-port server)))
               (auth-type (session-authentication-type server)))
           (bye server close-request/rdwr)
           (and (zero? (cdr (waitpid pid)))
                (eq? auth-type credentials/certificate)
                (equal? msg %message))))

       ;; client-side (child process)
       (let ((client (make-session connection-end/client))
             (cred   (make-certificate-credentials)))
         (set-session-priorities! client priorities)

         (set-certificate-credentials-openpgp-keys! cred pub sec)
         (set-session-credentials! client cred)
         (set-session-dh-prime-bits! client 1024)

         (set-session-transport-fd! client (port->fdes (car socket-pair)))

         (handshake client)
         (write %message (session-record-port client))
         (bye client close-request/rdwr)

         (primitive-exit))))))

;;; arch-tag: 1a973ed5-f45d-45a4-8160-900b6a8c27ff
