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

;; East Asian Ambiguous codepoints are narrow (width 1)
(assert-equal (utf8-display-width (make-string 1 (code-char #x2460))) 1
 "ambiguous circled digit is narrow")
(assert-equal (utf8-display-width (make-string 1 (code-char #x2660))) 1
 "ambiguous spade suit is narrow")
(assert-equal (utf8-display-width (make-string 1 (code-char #x26A0))) 1
 "ambiguous warning sign is narrow")
(assert-equal (utf8-display-width (make-string 1 (code-char #x2192))) 1
 "ambiguous arrow is narrow")
(assert-equal (utf8-display-width (make-string 1 (code-char #x00A9))) 1
 "ambiguous copyright is narrow")
(assert-equal (utf8-display-width (make-string 1 (code-char #x00B1))) 1
 "ambiguous plus-minus is narrow")

;; VS16 forces emoji presentation to 2 cells
(assert-equal
 (utf8-display-width
  (string-append (make-string 1 (code-char #x2660))
   (make-string 1 (code-char #xFE0F))))
 2
 "VS16 forces spade to 2")
(assert-equal
 (utf8-display-width
  (string-append (make-string 1 (code-char #x26A0))
   (make-string 1 (code-char #xFE0F))))
 2
 "VS16 forces warning to 2")

;; VS15 forces text presentation to 1 cell
(assert-equal
 (utf8-display-width
  (string-append (make-string 1 (code-char #x2660))
   (make-string 1 (code-char #xFE0E))))
 1
 "VS15 keeps spade at 1")

;; VS15 wins over VS16 when both are present
(assert-equal
 (utf8-display-width
  (string-append (make-string 1 (code-char #x2660))
   (make-string 1 (code-char #xFE0F))
   (make-string 1 (code-char #xFE0E))))
 1
 "VS15 wins over VS16")

;; Regional indicator pairs collapse to a single 2-cell flag cluster
(assert-equal
 (utf8-display-width
  (string-append (make-string 1 (code-char #x1F1FA))
   (make-string 1 (code-char #x1F1F8))))
 2
 "RI pair forms flag")
(assert-equal
 (utf8-display-width
  (string-append (make-string 1 (code-char #x1F1FA))
   (make-string 1 (code-char #x1F1F8))
   (make-string 1 (code-char #x1F1FA))
   (make-string 1 (code-char #x1F1F8))))
 2
 "two RI pairs are two flags")
(assert-equal (utf8-display-width (make-string 1 (code-char #x1F1FA))) 1
 "unpaired RI is width 1")

;; ZWJ emoji families collapse to a single 2-cell cluster
(assert-equal
 (utf8-display-width
  (string-append
   (make-string 1 (code-char #x1F468)) ; man
   (make-string 1 (code-char #x200D)) ; ZWJ
   (make-string 1 (code-char #x1F469)) ; woman
   (make-string 1 (code-char #x200D)) ; ZWJ
   (make-string 1 (code-char #x1F467)) ; girl
   (make-string 1 (code-char #x200D)) ; ZWJ
   (make-string 1 (code-char #x1F466)))) ; boy
 2
 "ZWJ family collapses to 2")
(assert-equal
 (utf8-display-width
  (string-append
   (make-string 1 (code-char #x1F468)) ; man
   (make-string 1 (code-char #x200D)) ; ZWJ
   (make-string 1 (code-char #x1F469)))) ; woman
 2
 "ZWJ pair collapses to 2")

;; Combining marks attach to a base and add no width
(assert-equal
 (utf8-display-width
  (string-append (make-string 1 (code-char 97)) ; a
   (make-string 1 (code-char 768)))) ; combining acute
 1
 "combining mark attaches to base")

;; Fullwidth and CJK unchanged
(assert-equal (utf8-display-width (make-string 1 (code-char #xFF21))) 2
 "fullwidth latin A is 2")
