;; Per-directory local variables for GNU Emacs 23 and later.

((nil
  . ((fill-column . 78)
     (tab-width   .  8)))
 (c-mode . ((c-file-style . "gnu")))
 (scheme-mode
  .
  ((indent-tabs-mode . nil)
   (eval . (put 'with-child-process 'scheme-indent-function 1))))
 (texinfo-mode . ((indent-tabs-mode . nil)
		  (fill-column . 72))))
