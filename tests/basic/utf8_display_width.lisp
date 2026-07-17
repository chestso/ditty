(load "tests/test-helpers.lisp")

;; ASCII
(assert-equal 4 (utf8-display-width "ABCD") "ascii width")

;; Wide characters (CJK)
(assert-equal 2 (utf8-display-width "中") "cjk width")

;; Combining characters (zero width)
(assert-equal 0 (utf8-display-width (make-string 1 (code-char 768)))
 "combining width")

;; Mixed
(assert-equal 3 (utf8-display-width "A中") "mixed width")

;; Empty
(assert-equal 0 (utf8-display-width "") "empty width")

;; Error
(assert-error (utf8-display-width 123) "non-string width")
