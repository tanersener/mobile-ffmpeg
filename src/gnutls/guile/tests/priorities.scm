;;; GnuTLS --- Guile bindings for GnuTLS
;;; Copyright (C) 2011-2012 Free Software Foundation, Inc.
;;;
;;; GnuTLS is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; GnuTLS is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with GnuTLS-EXTRA; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
;;; USA.

;;; Written by Ludovic Courtès <ludo@gnu.org>.


;;;
;;; Exercise the priority API of GnuTLS.
;;;

(use-modules (gnutls)
             (gnutls build tests)
             (srfi srfi-1)
             (srfi srfi-26))

(define %valid-priority-strings
  ;; Valid priority strings (from the manual).
  '("NONE:+VERS-TLS1.2:+MAC-ALL:+RSA:+AES-128-CBC:+SIGN-ALL:+COMP-NULL"
    "NORMAL:-ARCFOUR-128"
    "SECURE128:-VERS-SSL3.0:+COMP-NULL"
    "NONE:+VERS-TLS1.2:+AES-128-CBC:+RSA:+SHA1:+COMP-NULL:+SIGN-RSA-SHA1"))

(define %invalid-priority-strings
  ;; Invalid strings: the prefix and the suffix that leads to a parse error.
  '(("" . "THIS-DOES-NOT-WORK")
    ("NORMAL:" . "FAIL-HERE")
    ("SECURE128:-VERS-SSL3.0:" . "+FAIL-HERE")
    ("NONE:+VERS-TLS1.2:+AES-128-CBC:"
     . "+FAIL-HERE:+SHA1:+COMP-NULL:+SIGN-RSA-SHA1")))

(run-test

    (lambda ()
      (let ((s (make-session connection-end/client)))
        ;; We shouldn't have any exception with the valid priority strings.
        (for-each (cut set-session-priorities! s <>)
                  %valid-priority-strings)

        (every (lambda (prefix+suffix)
                 (let* ((prefix (car prefix+suffix))
                        (suffix (cdr prefix+suffix))
                        (pos    (string-length prefix))
                        (string (string-append prefix suffix)))
                   (catch 'gnutls-error
                     (lambda ()
                       (let ((s (make-session connection-end/client)))
                         ;; The following call should raise an exception.
                         (set-session-priorities! s string)
                         #f))
                     (lambda (key err function error-location . unused)
                       (and (eq? key 'gnutls-error)
                            (eq? err error/invalid-request)
                            (eq? function 'set-session-priorities!)
                            (= error-location pos))))))
               %invalid-priority-strings))))
