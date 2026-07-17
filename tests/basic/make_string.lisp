(load "tests/test-helpers.lisp")

;; ASCII fill
(assert-equal "AAAA" (make-string 4 #\A) "make-string ascii")
(assert-equal "    " (make-string 4) "make-string default space")
(assert-equal "" (make-string 0 #\A) "make-string empty")

;; Unicode fill (character count, not byte count)
(assert-equal 4 (string-length (make-string 4 #\u03b1))
 "make-string unicode length")

;; Error cases
(assert-error (make-string -1 #\A) "make-string negative length")
(assert-error (make-string 4 "A") "make-string non-char fill")
