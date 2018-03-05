;;; extract-c-doc.scm  --  Output Texinfo from "snarffed" C files.
;;;
;;; Copyright 2006-2012 Free Software Foundation, Inc.
;;;
;;;
;;; This program is free software; you can redistribute it and/or modify
;;; it under the terms of the GNU General Public License as published by
;;; the Free Software Foundation; either version 3 of the License, or
;;; (at your option) any later version.
;;;
;;; This program is distributed in the hope that it will be useful,
;;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;; GNU General Public License for more details.
;;;
;;; You should have received a copy of the GNU General Public License
;;; along with this program; if not, write to the Free Software
;;; Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

;;; Written by Ludovic Courtès <ludo@chbouib.org>.

(use-modules (system documentation c-snarf)
             (system documentation output)

             (srfi srfi-1))

(define (main file cpp+args cpp-flags . procs)
  ;; Arguments:
  ;;
  ;; 1. C file to be processed;
  ;; 2. how to invoke the CPP (e.g., "cpp -E");
  ;; 3. additional CPP flags (e.g., "-I /usr/local/include");
  ;; 4. optionally, a list of Scheme procedure names whose documentation is
  ;;    to be output.  If no such list is passed, then documentation for
  ;;    all the Scheme functions available in the C source file is issued.
  ;;
  (let* ((cpp+args  (string-tokenize cpp+args))
         (cpp       (car cpp+args))
         (cpp-flags (append (cdr cpp+args)
                            (string-tokenize cpp-flags)
                            (list "-DSCM_MAGIC_SNARF_DOCS "))))
    ;;(format (current-error-port) "cpp-flags: ~a~%" cpp-flags)
    (format (current-error-port) "extracting Texinfo doc from `~a'...  "
            file)

    ;; Don't mention the name of C functions.
    (*document-c-functions?* #f)

    (let ((proc-doc-list
           (run-cpp-and-extract-snarfing file cpp cpp-flags)))
      (display "@c Automatically generated, do not edit.\n")
      (display (string-concatenate
                (map procedure-texi-documentation
                     (if (null? procs)
                         proc-doc-list
                         (filter (lambda (proc-doc)
                                   (let ((proc-name
                                          (assq-ref proc-doc
                                                    'scheme-name)))
                                     (member proc-name procs)))
                                 proc-doc-list))))))
    (format (current-error-port) "done.~%")
    (exit 0)))


;;; Local Variables:
;;; mode: scheme
;;; coding: latin-1
;;; End:
