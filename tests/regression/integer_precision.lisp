(load "tests/test-helpers.lisp")

;; Integer arithmetic stays exact above 2^53
(assert-equal (logand (+ 9007199254740993 0) 1) 1 "+ keeps integers exact")
(assert-equal (+ 9007199254740993 1) 9007199254740994 "+ exact increment")
(assert-equal (- 9007199254740993 0) 9007199254740993 "- keeps integers exact")
(assert-equal (* 9007199254740993 1) 9007199254740993 "* keeps integers exact")

(assert-true (integer? (+ 9007199254740993 0)) "integer type preserved")

;; Intermediate overflow that cancels stays exact
(assert-equal (+ 9223372036854775807 1 -1) 9223372036854775807
 "addition survives an intermediate overflow")
(assert-equal (* 9223372036854775807 2 0) 0
 "multiplication survives an intermediate overflow")

;; True overflow falls back to a float
(assert-false (integer? (+ 9223372036854775807 1))
 "addition overflow returns a float")
(assert-false (integer? (* 9223372036854775807 2))
 "multiplication overflow returns a float")

(assert-true (approx= (+ 9223372036854775807 1) 9223372036854775808.0)
 "overflow value is 2^63")

;; LLONG_MIN is representable, but its negation is not an integer
(define min-int (- -9223372036854775807 1))

(assert-true (integer? min-int) "LLONG_MIN is representable")

(assert-false (integer? (- min-int)) "negating LLONG_MIN returns a float")

;; max/min compare exactly and the result type follows the winner
(assert-equal (logand (max 9007199254740993 9007199254740992) 1) 1
 "max is exact above 2^53")

(assert-true (integer? (max 3 2.5)) "max keeps an integer winner")

(assert-equal (max 3 2.5) 3 "max returns the winning integer")

(assert-true (integer? (min 1 2.5 2)) "min keeps an integer winner")

(assert-equal (min 1 2.5 2) 1 "min returns the winning integer")

(assert-true (approx= (min 2.5 3) 2.5) "min returns the winning float")

(assert-false (integer? (min 2.5 3)) "min float winner stays float")

;; even?/odd? are exact
(assert-false (even? 9007199254740993) "even? is exact above 2^53")

(assert-true (odd? 9007199254740993) "odd? is exact above 2^53")
(assert-true (even? 9007199254740992) "even? on an even large integer")

(assert-false (even? 3.0) "even? truncates floats")

(assert-true (odd? -3.0) "odd? truncates negative floats")

;; ash: defined shifts at and beyond the word size
(assert-equal (ash -1 63) min-int "ash -1 63 is LLONG_MIN")

(assert-false (integer? (ash 1 63)) "ash overflow returns a float")

(assert-true (approx= (ash 1 63) 9223372036854775808.0) "ash 1 63 is 2^63")

(assert-equal (ash 1 -64) 0 "ash right shift beyond the word size is zero")
(assert-equal (ash -1 -64) -1 "ash negative right shift beyond the word size")
(assert-equal (ash -1 -1000) -1 "ash huge negative shift keeps the sign")
(assert-equal (ash 0 10000) 0 "ash zero by a huge amount is zero")

(assert-error (ash 1 10000) "ash overflow beyond the float range")
