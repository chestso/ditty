;; Integer operations: remainder, even?, odd?
;; Demonstrate the new integer utility functions
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; Remainder (modulo) operation
(assert-equal 2 (remainder 17 5) "remainder of 17 divided by 5")
(assert-equal 1 (remainder 10 3) "remainder of 10 divided by 3")
(assert-equal 0 (remainder 20 5) "remainder of 20 divided by 5 is 0")

;; Even numbers
(assert-true (even? 0) "0 is even")
(assert-true (even? 2) "2 is even")
(assert-true (even? 4) "4 is even")
(assert-true (even? 10) "10 is even")
;; Odd numbers
(assert-true (odd? 1) "1 is odd")
(assert-true (odd? 3) "3 is odd")
(assert-true (odd? 5) "5 is odd")
(assert-true (odd? 11) "11 is odd")
;; Negative numbers
(assert-true (even? -2) "-2 is even")
(assert-true (odd? -1) "-1 is odd")
(assert-true (odd? -3) "-3 is odd")

;; Combined in logic
(assert-equal "yes" (if (even? 6) "yes" "no") "even? 6 in conditional")
(assert-equal "yes" (if (odd? 7) "yes" "no") "odd? 7 in conditional")

;; Practical example: Filter even numbers
(define test_even (lambda (n) (if (even? n) "even" "odd")))

(assert-equal "even" (test_even 8) "test_even identifies 8 as even")
(assert-equal "odd" (test_even 9) "test_even identifies 9 as odd")
;; Modular arithmetic patterns
(assert-equal 1 (remainder 100 3) "100 mod 3 equals 1")
(assert-equal 0 (remainder 100 4) "100 mod 4 equals 0")
(assert-equal 2 (remainder 100 7) "100 mod 7 equals 2")
