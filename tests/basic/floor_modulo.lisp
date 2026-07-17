(load "tests/test-helpers.lisp")

;; floor: division rounded toward negative infinity
(assert-equal 2 (floor 7 3) "floor positive")
(assert-equal -3 (floor -7 3) "floor negative dividend")
(assert-equal -3 (floor 7 -3) "floor negative divisor")
(assert-equal 2 (floor -7 -3) "floor both negative")

;; modulo: result has same sign as divisor
(assert-equal 1 (modulo 7 3) "modulo positive")
(assert-equal 2 (modulo -7 3) "modulo negative dividend")
(assert-equal -2 (modulo 7 -3) "modulo negative divisor")
(assert-equal -1 (modulo -7 -3) "modulo both negative")

;; error cases
(assert-error (floor 7 0) "floor by zero")
(assert-error (modulo 7 0) "modulo by zero")
(assert-error (floor 7.5 3) "floor with non-integer")
