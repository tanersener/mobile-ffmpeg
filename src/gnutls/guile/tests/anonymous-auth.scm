;;; GnuTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007-2013, 2016 Free Software Foundation, Inc.
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
;;; Test session establishment using anonymous authentication.  Exercise the
;;; record layer low-level API.
;;;

(use-modules (gnutls)
             (gnutls build tests)
             (srfi srfi-4))


;; TLS session settings.
(define priorities
  "NONE:+VERS-TLS-ALL:+CIPHER-ALL:+MAC-ALL:+SIGN-ALL:+COMP-ALL:+ANON-DH")

;; Message sent by the client.
(define %message (apply u8vector (iota 256)))

(define (import-something import-proc file fmt)
  (let* ((path (search-path %load-path file))
         (size (stat:size (stat path)))
         (raw  (make-u8vector size)))
    (uniform-vector-read! raw (open-input-file path))
    (import-proc raw fmt)))

(define (import-dh-params file)
  (import-something pkcs3-import-dh-parameters file
                    x509-certificate-format/pem))

;; Debugging.
;; (set-log-level! 100)
;; (set-log-procedure! (lambda (level str)
;;                       (format #t "[~a|~a] ~a" (getpid) level str)))

(run-test
 (lambda ()
   (let ((socket-pair (socketpair PF_UNIX SOCK_STREAM 0)))
     (with-child-process pid
       ;; server-side
       (let ((server (make-session connection-end/server)))
         (set-session-priorities! server priorities)

         (set-session-transport-fd! server (port->fdes (cdr socket-pair)))
         (let ((cred (make-anonymous-server-credentials))
               (dh-params (import-dh-params "dh-parameters.pem")))
           ;; Note: DH parameter generation can take some time.
           (set-anonymous-server-dh-parameters! cred dh-params)
           (set-session-credentials! server cred))
         (set-session-dh-prime-bits! server 1024)

         (handshake server)
         (let* ((buf (make-u8vector (u8vector-length %message)))
                (amount (record-receive! server buf)))
           (bye server close-request/rdwr)
           (and (zero? (cdr (waitpid pid)))
                (= amount (u8vector-length %message))
                (equal? buf %message))))

       ;; client-side (child process)
       (let ((client (make-session connection-end/client)))
         (set-session-priorities! client priorities)
         (set-session-server-name! client
                                   server-name-type/dns (gethostname))
         (set-session-transport-fd! client (port->fdes (car socket-pair)))
         (set-session-credentials! client (make-anonymous-client-credentials))
         (set-session-dh-prime-bits! client 1024)

         (handshake client)
         (record-send client %message)
         (bye client close-request/rdwr)

         (primitive-exit))))))

;;; arch-tag: 8c98de24-0a53-4290-974e-4b071ad162a0
