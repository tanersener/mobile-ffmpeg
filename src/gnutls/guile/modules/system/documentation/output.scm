;;; output.scm  --  Output documentation "snarffed" from C files in Texi/GDF.
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

(define-module (system documentation output)
  :use-module (srfi srfi-1)
  :use-module (srfi srfi-13)
  :use-module (srfi srfi-39)
  :autoload   (system documentation c-snarf) (run-cpp-and-extract-snarfing)

  :export (schemify-name scheme-procedure-texi-line
           procedure-gdf-string procedure-texi-documentation
           output-procedure-texi-documentation-from-c-file
           *document-c-functions?*))

;;; Author:  Ludovic Courtès
;;;
;;; Commentary:
;;;
;;; This module provides support function to issue Texinfo or GDF (Guile
;;; Documentation Format) documentation from "snarffed" C files.
;;;
;;; Code:


;;;
;;; Utility.
;;;

(define (schemify-name str)
  "Turn @var{str}, a C variable or function name, into a more ``Schemey''
form, e.g., one with dashed instead of underscores, etc."
  (string-map (lambda (chr)
                (if (eq? chr #\_)
                    #\-
                    chr))
              (if (string-suffix? "_p" str)
                  (string-append (substring str 0
                                            (- (string-length str) 2))
                                 "?")
                  str)))


;;;
;;; Issuing Texinfo and GDF-formatted doc (i.e., `guile-procedures.texi').
;;; GDF = Guile Documentation Format
;;;

(define *document-c-functions?*
  ;; Whether to mention C function names along with Scheme procedure names.
  (make-parameter #t))

(define (scheme-procedure-texi-line proc-name args
                                    required-args optional-args
                                    rest-arg?)
  "Return a Texinfo string describing the Scheme procedure named
@var{proc-name}, whose arguments are listed in @var{args} (a list of strings)
and whose signature is defined by @var{required-args}, @var{optional-args}
and @var{rest-arg?}."
  (string-append "@deffn {Scheme Procedure} " proc-name " "
                 (string-join (take args required-args) " ")
                 (string-join (take (drop args required-args)
                                    (+ optional-args
                                       (if rest-arg? 1 0)))
                              " [" 'prefix)
                 (if rest-arg? "...]" "")
                 (make-string optional-args #\])))

(define (procedure-gdf-string proc-doc)
  "Issue a Texinfo/GDF docstring corresponding to @var{proc-doc}, a
documentation alist as returned by @code{parse-snarfed-line}.  To produce
actual GDF-formatted doc, the resulting string must be processed by
@code{makeinfo}."
  (let* ((proc-name     (assq-ref proc-doc 'scheme-name))
         (args          (assq-ref proc-doc 'arguments))
         (signature     (assq-ref proc-doc 'signature))
         (required-args (assq-ref signature 'required))
         (optional-args (assq-ref signature 'optional))
         (rest-arg?     (assq-ref signature 'rest?))
         (location      (assq-ref proc-doc 'location))
         (file-name     (car location))
         (line          (cadr location))
         (documentation (assq-ref proc-doc 'documentation)))
    (string-append "" ;; form feed
                   proc-name (string #\newline)
                   (format #f "@c snarfed from ~a:~a~%"
                           file-name line)

                   (scheme-procedure-texi-line proc-name
                                               (map schemify-name args)
                                               required-args optional-args
                                               rest-arg?)

                   (string #\newline)
                   documentation (string #\newline)
                   "@end deffn" (string #\newline))))

(define (procedure-texi-documentation proc-doc)
  "Issue a Texinfo docstring corresponding to @var{proc-doc}, a documentation
alist as returned by @var{parse-snarfed-line}.  The resulting Texinfo string
is meant for use in a manual since it also documents the corresponding C
function."
  (let* ((proc-name     (assq-ref proc-doc 'scheme-name))
         (c-name        (assq-ref proc-doc 'c-name))
         (args          (assq-ref proc-doc 'arguments))
         (signature     (assq-ref proc-doc 'signature))
         (required-args (assq-ref signature 'required))
         (optional-args (assq-ref signature 'optional))
         (rest-arg?     (assq-ref signature 'rest?))
         (location      (assq-ref proc-doc 'location))
         (file-name     (car location))
         (line          (cadr location))
         (documentation (assq-ref proc-doc 'documentation)))
  (string-append (string #\newline)
		 (format #f "@c snarfed from ~a:~a~%"
			 file-name line)

                 ;; document the Scheme procedure
                 (scheme-procedure-texi-line proc-name
                                             (map schemify-name args)
                                             required-args optional-args
                                             rest-arg?)
                 (string #\newline)

                 (if (*document-c-functions?*)
                     (string-append
                      ;; document the C function
                      "@deffnx {C Function} " c-name " ("
                      (if (null? args)
                          "void"
                          (string-join (map (lambda (arg)
                                              (string-append "SCM " arg))
                                            args)
                                       ", "))
                      ")" (string #\newline))
                     "")

		 documentation (string #\newline)
                 "@end deffn" (string #\newline))))


;;;
;;; Very high-level interface.
;;;

(define (output-procedure-texi-documentation-from-c-file c-file cpp cflags
                                                         port)
  (for-each (lambda (texi-string)
              (display texi-string port))
            (map procedure-texi-documentation
                 (run-cpp-and-extract-snarfing c-file cpp cflags))))


;;; output.scm ends here

;;; Local Variables:
;;; mode: scheme
;;; coding: latin-1
;;; End:

;;; arch-tag: 20ca493a-6f1a-4d7f-9d24-ccce0d32df49
