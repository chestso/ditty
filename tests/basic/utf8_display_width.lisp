(load "tests/test-helpers.lisp")

;; ASCII
(assert-equal (utf8-display-width "ABCD") 4 "ascii width")

;; Wide characters (CJK)
(assert-equal (utf8-display-width "中") 2 "cjk width")

;; Combining characters (zero width)
(assert-equal (utf8-display-width (make-string 1 (code-char 768))) 0
 "combining width")

;; Mixed
(assert-equal (utf8-display-width "A中") 3 "mixed width")

;; Empty
(assert-equal (utf8-display-width "") 0 "empty width")

;; Error
(assert-error (utf8-display-width 123) "non-string width")
