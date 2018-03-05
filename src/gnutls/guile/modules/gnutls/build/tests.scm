;;; GnuTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2011-2012, 2016 Free Software Foundation, Inc.
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

;;; Written by Ludovic Courtès <ludo@gnu.org>.

(define-module (gnutls build tests)
  #:export (run-test
            with-child-process))

(define (run-test thunk)
  "Call `(exit (THUNK))'.  If THUNK raises an exception, then call `(exit 1)' and
display a backtrace.  Otherwise, return THUNK's return value."
  (exit
   (catch #t
     thunk
     (lambda (key . args)
       ;; Never reached.
       (exit 1))
     (lambda (key . args)
       (dynamic-wind ;; to be on the safe side
         (lambda () #t)
         (lambda ()
           (format (current-error-port)
                   "~%throw to `~a' with args ~s~%" key args)
           (display-backtrace (make-stack #t) (current-output-port)))
         (lambda ()
           (exit 1)))
       (exit 1)))))

(define (call-with-child-process child parent)
  "Run thunk CHILD in a child process and invoke PARENT from the parent
process, passing it the PID of the child process.  Make sure the child
process exits upon failure."
  (let ((pid (primitive-fork)))
    (if (zero? pid)
        (dynamic-wind
          (const #t)
          (lambda ()
            (primitive-exit (if (child) 0 1)))
          (lambda ()
            (primitive-exit 2)))
        (parent pid))))

(cond-expand
  ((not guile-2)                                  ;1.8, yay!
   (use-modules (ice-9 syncase))

   (define-syntax define-syntax-rule
     (syntax-rules ()
       ((_ (name args ...) docstring body)
        (define-syntax name
          (syntax-rules ()
            ((_ args ...) body))))))

   (export define-syntax-rule))

  (else                                           ;2.0 and 2.2
   (use-modules (rnrs io ports)
                (rnrs bytevectors)
                (ice-9 match))

   (define-syntax-rule (define-replacement (name args ...) body ...)
     ;; Define a compatibility replacement for NAME, if needed.
     (define-public name
       (if (module-defined? the-scm-module 'name)
           (module-ref the-scm-module 'name)
           (lambda (args ...)
             body ...))))

   ;; 'uniform-vector-read!' and 'uniform-vector-write' are deprecated in 2.0
   ;; and absent in 2.2.

   (define-replacement (uniform-vector-read! buf port)
     (match (get-bytevector-n! port buf
                               0 (bytevector-length buf))
       ((? eof-object?) 0)
       ((? integer? n)  n)))

   (define-replacement (uniform-vector-write buf port)
     (put-bytevector port buf))))


(define-syntax-rule (with-child-process pid parent child)
  "Fork and evaluate expression PARENT in the current process, with PID bound
to the PID of its child process; the child process evaluated CHILD."
  (call-with-child-process
   (lambda () child)
   (lambda (pid) parent)))

;;; Local Variables:
;;; eval: (put 'define-replacement 'scheme-indent-function 1)
;;; End:
