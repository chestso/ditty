(load "tests/test-helpers.lisp")

;; floor: division rounded toward negative infinity
(assert-equal (floor 7 3) 2 "floor positive")
(assert-equal (floor -7 3) -3 "floor negative dividend")
(assert-equal (floor 7 -3) -3 "floor negative divisor")
(assert-equal (floor -7 -3) 2 "floor both negative")

;; modulo: result has same sign as divisor
(assert-equal (modulo 7 3) 1 "modulo positive")
(assert-equal (modulo -7 3) 2 "modulo negative dividend")
(assert-equal (modulo 7 -3) -2 "modulo negative divisor")
(assert-equal (modulo -7 -3) -1 "modulo both negative")

;; quotient: truncates toward zero
(assert-equal (quotient 7.5 2) 3 "quotient float dividend")
(assert-equal (quotient -7.5 2) -3 "quotient negative float dividend")
(assert-equal (quotient 7 2.0) 3 "quotient float divisor")
(assert-equal (quotient 10.8 3.2) 3 "quotient two floats")

(assert-true (integer? (quotient 7.5 2)) "quotient float result is an integer")
(assert-true (integer? (quotient 7 2.0)) "quotient mixed result is an integer")
;; integer behavior unchanged
(assert-equal (quotient -7 2) -3 "quotient negative dividend")
(assert-equal (quotient 7 -2) -3 "quotient negative divisor")
(assert-equal (logand (quotient 9007199254740993 1) 1) 1
 "quotient integer exactness")

(assert-error (quotient 7.5 0) "quotient float by zero")

;; remainder: result has same sign as dividend
(assert-equal (remainder 17 5) 2 "remainder positive")
(assert-equal (remainder -17 5) -2 "remainder negative dividend")
(assert-equal (remainder 20 5) 0 "remainder exact")

(assert-true (approx= (remainder 7.5 2) 1.5) "remainder float")
(assert-true (approx= (remainder -7.5 2) -1.5) "remainder negative float")
(assert-true (approx= (remainder 7.5 -2) 1.5)
 "remainder float negative divisor")
(assert-true (approx= (remainder 5.5 2.5) 0.5) "remainder fraction")
(assert-true (approx= (remainder 7 2.0) 1) "remainder mixed")

(assert-false (integer? (remainder 7.5 2)) "remainder float result is a float")
(assert-false (integer? (remainder 7 2.0)) "remainder mixed result is a float")
;; integer behavior unchanged
(assert-equal (remainder -7 3) -1 "remainder negative dividend integer")

(assert-true (integer? (remainder -7 3)) "remainder integer result")

(assert-equal (remainder 7 -3) 1 "remainder negative divisor integer")
(assert-equal (remainder 5 -1) 0 "remainder by minus one")
(assert-equal (remainder 9007199254740993 2) 1 "remainder integer exactness")

(assert-error (remainder 7.5 0) "remainder float by zero")
(assert-error (remainder 7 0.0) "remainder by float zero")

;; modulo: result has same sign as divisor
(assert-true (approx= (modulo 7.5 2) 1.5) "modulo float")
(assert-true (approx= (modulo -7.5 2) 0.5) "modulo negative float")
(assert-true (approx= (modulo 7.5 -2) -0.5) "modulo float negative divisor")
(assert-true (approx= (modulo -7.5 -2) -1.5) "modulo both negative float")
(assert-true (approx= (modulo 7 2.5) 2) "modulo mixed")
(assert-true (approx= (modulo 6.0 2) 0) "modulo integral float")

(assert-false (integer? (modulo 6.0 2)) "modulo float result is a float")
;; integer behavior unchanged
(assert-equal (modulo 5 -1) 0 "modulo by minus one")
(assert-equal (modulo 9007199254740993 2) 1 "modulo integer exactness")

(assert-error (modulo 7.5 0) "modulo float by zero")

;; error cases
(assert-error (floor 7 0) "floor by zero")
(assert-error (modulo 7 0) "modulo by zero")
