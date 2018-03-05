;;; Help produce Guile wrappers for GnuTLS types.
;;;
;;; GnuTLS --- Guile bindings for GnuTLS.
;;; Copyright (C) 2007, 2010-2012 Free Software Foundation, Inc.
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


(use-modules (gnutls build smobs))


;;;
;;; The program.
;;;

(define (main . args)
  (let ((port (current-output-port)))
    (for-each (lambda (type)
                (output-smob-type-definition type port)
                (output-smob-type-predicate type port))
              %gnutls-smobs)))

(apply main (cdr (command-line)))

;;; Local Variables:
;;; mode: scheme
;;; coding: latin-1
;;; End:

;;; arch-tag: 364811a0-6d0a-431a-ae50-d2f7dc529903
