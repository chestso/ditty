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

;; error cases
(assert-error (floor 7 0) "floor by zero")
(assert-error (modulo 7 0) "modulo by zero")
(assert-error (floor 7.5 3) "floor with non-integer")
