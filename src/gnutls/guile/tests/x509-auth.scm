;;; GnuTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007-2014, 2016 Free Software Foundation, Inc.
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
;;; Test session establishment using X.509 certificate authentication.
;;; Based on `openpgp-auth.scm'.
;;;

(use-modules (gnutls)
             (gnutls build tests)
             (srfi srfi-4))

;; TLS session settings.
(define priorities
  "NORMAL")

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
  (import-something import-proc file x509-certificate-format/pem))

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
         (pub         (import-key import-x509-certificate
                                  "x509-certificate.pem"))
         (sec         (import-key import-x509-private-key
                                  "x509-key.pem")))
     (with-child-process pid

       ;; server-side
       (let ((server (make-session connection-end/server))
             (dh     (import-dh-params "dh-parameters.pem")))
         (set-session-priorities! server priorities)
         (set-server-session-certificate-request! server
                                                  certificate-request/require)

         (set-session-transport-fd! server (port->fdes (cdr socket-pair)))
         (let ((cred (make-certificate-credentials))
               (trust-file (search-path %load-path
                                        "x509-certificate.pem"))
               (trust-fmt  x509-certificate-format/pem))
           (set-certificate-credentials-dh-parameters! cred dh)
           (set-certificate-credentials-x509-keys! cred (list pub) sec)
           (set-certificate-credentials-x509-trust-file! cred
                                                         trust-file
                                                         trust-fmt)
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
         (set-certificate-credentials-x509-keys! cred (list pub) sec)
         (set-session-credentials! client cred)
         (set-session-dh-prime-bits! client 1024)

         (set-session-transport-fd! client (port->fdes (car socket-pair)))

         (handshake client)
         (write %message (session-record-port client))
         (bye client close-request/rdwr)

         (primitive-exit))))))

;;; arch-tag: 1f88f835-a5c8-4fd6-94b6-5a13571ba03d
