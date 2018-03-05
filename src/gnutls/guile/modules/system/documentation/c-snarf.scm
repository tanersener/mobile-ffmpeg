;;; c-snarf.scm  --  Parsing documentation "snarffed" from C files.
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

(define-module (system documentation c-snarf)
  :use-module (ice-9 popen)
  :use-module (ice-9 rdelim)

  :use-module (srfi srfi-13)
  :use-module (srfi srfi-14)
  :use-module (srfi srfi-39)

  :export (run-cpp-and-extract-snarfing
           parse-snarfing
           parse-snarfed-line))

;;; Author:  Ludovic Courtès
;;;
;;; Commentary:
;;;
;;; This module provides tools to parse and otherwise manipulate
;;; documentation "snarffed" from C files, i.e., information obtained by
;;; running the C preprocessor with the @code{-DSCM_MAGIC_SNARF_DOCS} flag.
;;;
;;; Code:



;;;
;;; High-level API.
;;;

(define (run-cpp-and-extract-snarfing file cpp cpp-flags)
  (let ((pipe (apply open-pipe* OPEN_READ
                     (cons cpp (append cpp-flags (list file))))))
    (parse-snarfing pipe)))


;;;
;;; Parsing magic-snarffed CPP output.
;;;

(define (parse-c-argument-list arg-string)
  "Parse @var{arg-string} (a string representing a ANSI C argument list,
e.g., @var{(const SCM first, SCM second_arg)}) and return a list of strings
denoting the argument names."
  (define %c-symbol-char-set
    (char-set-adjoin char-set:letter+digit #\_))

  (let loop ((args (string-tokenize (string-trim-both arg-string #\space)
				    %c-symbol-char-set))
	     (type? #t)
	     (result '()))
    (if (null? args)
	(reverse! result)
	(let ((the-arg (car args)))
	  (cond ((and type? (string=? the-arg "const"))
		 (loop (cdr args) type? result))
		((and type? (string=? the-arg "SCM"))
		 (loop (cdr args) (not type?) result))
                (type? ;; any other type, e.g., `void'
                 (loop (cdr args) (not type?) result))
		(else
		 (loop (cdr args) (not type?) (cons the-arg result))))))))

(define (parse-documentation-item item)
  "Parse @var{item} (a string), a single function string produced by the C
preprocessor.  The result is an alist whose keys represent specific aspects
of a procedure's documentation: @code{c-name}, @code{scheme-name},
 @code{documentation} (a Texinfo documentation string), etc."

  (define (read-strings)
    ;; Read several subsequent strings and return their concatenation.
    (let loop ((str (read))
               (result '()))
      (if (or (eof-object? str)
              (not (string? str)))
          (string-concatenate (reverse! result))
          (loop (read) (cons str result)))))

  (let* ((item (string-trim-both item #\space))
	 (space (string-index item #\space)))
    (if (not space)
	(error "invalid documentation item" item)
	(let ((kind (substring item 0 space))
	      (rest (substring item space (string-length item))))
	  (cond ((string=? kind "cname")
		 (cons 'c-name (string-trim-both rest #\space)))
		((string=? kind "fname")
		 (cons 'scheme-name
                       (with-input-from-string rest read-strings)))
		((string=? kind "type")
		 (cons 'type (with-input-from-string rest read)))
		((string=? kind "location")
		 (cons 'location
		       (with-input-from-string rest
			 (lambda ()
			   (let loop ((str (read))
				      (result '()))
			     (if (eof-object? str)
				 (reverse! result)
				 (loop (read) (cons str result))))))))
		((string=? kind "arglist")
		 (cons 'arguments
		       (parse-c-argument-list rest)))
		((string=? kind "argsig")
		 (cons 'signature
		       (with-input-from-string rest
			 (lambda ()
			   (let ((req (read)) (opt (read)) (rst? (read)))
			     (list (cons 'required req)
				   (cons 'optional opt)
				   (cons 'rest?    (= 1 rst?))))))))
		(else
		 ;; docstring (may consist of several C strings which we
		 ;; assume to be equivalent to Scheme strings)
		 (cons 'documentation
		       (with-input-from-string item read-strings))))))))

(define (parse-snarfed-line line)
  "Parse @var{line}, a string that contains documentation returned for a
single function by the C preprocessor with the @code{-DSCM_MAGIC_SNARF_DOCS}
option.  @var{line} is assumed to be a complete \"^^ { ... ^^ }\" sequence."
  (define (caret-split str)
    (let loop ((str str)
	       (result '()))
      (if (string=? str "")
	  (reverse! result)
	  (let ((caret (string-index str #\^))
		(len (string-length str)))
	    (if caret
		(if (and (> (- len caret) 0)
			 (eq? (string-ref str (+ caret 1)) #\^))
		    (loop (substring str (+ 2 caret) len)
			  (cons (string-take str (- caret 1)) result))
		    (error "single caret not allowed" str))
		(loop "" (cons str result)))))))

  (let ((items (caret-split (substring line 4
				       (- (string-length line) 4)))))
    (map parse-documentation-item items)))


(define (parse-snarfing port)
  "Read C preprocessor (where the @code{SCM_MAGIC_SNARF_DOCS} macro is
defined) output from @var{port} a return a list of alist, each of which
contains information about a specific function described in the C
preprocessor output."
  (define start-marker "^^ {")
  (define end-marker   "^^ }")

  (define (read-snarf-lines start)
    ;; Read the snarf lines that follow START until and end marker is found.
    (let loop ((line   start)
               (result '()))
      (cond ((eof-object? line)
             ;; EOF in the middle of a "^^ { ... ^^ }" sequence; shouldn't
             ;; happen.
             line)
            ((string-contains line end-marker)
             =>
             (lambda (end)
               (let ((result (cons (string-take line (+ 3 end))
                                   result)))
                 (string-concatenate-reverse result))))
            ((string-prefix? "#" line)
             ;; Presumably a "# LINENUM" directive; skip it.
             (loop (read-line port) result))
            (else
             (loop (read-line port)
                   (cons line result))))))

  (let loop ((line (read-line port))
	     (result '()))
    (cond ((eof-object? line)
           result)
          ((string-contains line start-marker)
           =>
           (lambda (start)
             (let ((line
                    (read-snarf-lines (string-drop line start))))
               (loop (read-line port)
                     (cons (parse-snarfed-line line) result)))))
          (else
           (loop (read-line port) result)))))


;;; c-snarf.scm ends here

;;; Local Variables:
;;; mode: scheme
;;; coding: latin-1
;;; End:

;;; arch-tag: dcba2446-ee43-46d8-a47e-e6e12f121988
