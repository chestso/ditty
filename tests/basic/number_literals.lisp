(load "tests/test-helpers.lisp")

;; Hex
(assert-equal 255 #xFF "hex uppercase")
(assert-equal 255 #xff "hex lowercase")
(assert-equal -255 (- #xFF) "hex negated")
(assert-equal 65535 #xFFFF "hex 4-digit")

;; Octal
(assert-equal 8 #o10 "octal")
(assert-equal 64 #o100 "octal 64")

;; Binary
(assert-equal 3 #b11 "binary")
(assert-equal 15 #b1111 "binary 15")

;; Mixed arithmetic
(assert-equal (+ #xFF 1) 256 "hex in arithmetic")
(assert-equal (* #o10 2) 16 "octal in arithmetic")
