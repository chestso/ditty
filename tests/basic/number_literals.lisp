(load "tests/test-helpers.lisp")

;; Hex
(assert-equal #xFF 255 "hex uppercase")
(assert-equal #xff 255 "hex lowercase")
(assert-equal (- #xFF) -255 "hex negated")
(assert-equal #xFFFF 65535 "hex 4-digit")

;; Octal
(assert-equal #o10 8 "octal")
(assert-equal #o100 64 "octal 64")

;; Binary
(assert-equal #b11 3 "binary")
(assert-equal #b1111 15 "binary 15")

;; Mixed arithmetic
(assert-equal 256 (+ #xFF 1) "hex in arithmetic")
(assert-equal 16 (* #o10 2) "octal in arithmetic")
