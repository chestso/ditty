;; Bitwise operations
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; ---- logand (bitwise AND) ----

;; Basic operations
(assert-equal 7 (logand 15 7) "logand: 15 & 7 = 7")
(assert-equal 8 (logand 12 10) "logand: 12 & 10 = 8")
(assert-equal 0 (logand 8 1) "logand: 8 & 1 = 0")

;; Identity: no args returns -1 (all bits set)
(assert-equal -1 (logand) "logand: identity is -1")

;; Single arg returns itself
(assert-equal 42 (logand 42) "logand: single arg returns itself")

;; Variadic
(assert-equal 0 (logand 15 7 0) "logand: variadic with zero")
(assert-equal 1 (logand 7 3 1) "logand: variadic three args")

;; Negative numbers (two's complement)
(assert-equal 6 (logand -1 6) "logand: -1 & 6 = 6")
(assert-equal -8 (logand -4 -8) "logand: negative & negative")

;; ---- logior (bitwise inclusive OR) ----

;; Basic operations
(assert-equal 14 (logior 12 10) "logior: 12 | 10 = 14")
(assert-equal 9 (logior 8 1) "logior: 8 | 1 = 9")

;; Identity: no args returns 0
(assert-equal 0 (logior) "logior: identity is 0")

;; Single arg returns itself
(assert-equal 42 (logior 42) "logior: single arg returns itself")

;; Variadic
(assert-equal 15 (logior 1 2 4 8) "logior: variadic four args")

;; ---- logxor (bitwise exclusive OR) ----

;; Basic operations
(assert-equal 6 (logxor 12 10) "logxor: 12 ^ 10 = 6")
(assert-equal 9 (logxor 8 1) "logxor: 8 ^ 1 = 9")

;; Identity: no args returns 0
(assert-equal 0 (logxor) "logxor: identity is 0")

;; Single arg returns itself
(assert-equal 42 (logxor 42) "logxor: single arg returns itself")

;; XOR with self is 0
(assert-equal 0 (logxor 42 42) "logxor: x ^ x = 0")

;; ---- lognot (bitwise NOT) ----

;; Basic operations
(assert-equal -1 (lognot 0) "lognot: ~0 = -1")
(assert-equal 0 (lognot -1) "lognot: ~(-1) = 0")
(assert-equal -6 (lognot 5) "lognot: ~5 = -6")

;; Double negation
(assert-equal 42 (lognot (lognot 42)) "lognot: double negation")

;; ---- ash (arithmetic shift) ----

;; Left shift (multiply by 2^n)
(assert-equal 8 (ash 1 3) "ash: 1 << 3 = 8")
(assert-equal 16 (ash 2 3) "ash: 2 << 3 = 16")
(assert-equal 0 (ash 0 5) "ash: 0 << 5 = 0")

;; Right shift (divide by 2^n, truncates toward negative infinity)
(assert-equal 1 (ash 8 -3) "ash: 8 >> 3 = 1")
(assert-equal 3 (ash 7 -1) "ash: 7 >> 1 = 3")
(assert-equal 0 (ash 1 -1) "ash: 1 >> 1 = 0")

;; Shift by 0 returns unchanged
(assert-equal 42 (ash 42 0) "ash: shift by 0")

;; Negative numbers (sign-extending right shift)
(assert-equal -2 (ash -4 -1) "ash: -4 >> 1 = -2")
(assert-equal -1 (ash -8 -3) "ash: -8 >> 3 = -1")

;; ---- logcount (count 1-bits) ----

;; Basic operations
(assert-equal 0 (logcount 0) "logcount: 0 has 0 bits")
(assert-equal 1 (logcount 1) "logcount: 1 has 1 bit")
(assert-equal 2 (logcount 3) "logcount: 3 has 2 bits")
(assert-equal 3 (logcount 7) "logcount: 7 has 3 bits")
(assert-equal 4 (logcount 15) "logcount: 15 has 4 bits")

;; Larger numbers
(assert-equal 3 (logcount 42) "logcount: 42 = 0b101010 has 3 bits")
