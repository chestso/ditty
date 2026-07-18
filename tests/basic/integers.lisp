;; Integer and mixed arithmetic examples
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; Integer arithmetic
(assert-equal 6 (+ 1 2 3) "Integer addition")
(assert-equal 7 (- 10 3) "Integer subtraction")
(assert-equal 24 (* 2 3 4) "Integer multiplication")
;; Mixed integer/float
(assert-equal 3.5 (+ 1 2.5) "Mixed int+float addition")
(assert-equal 12.0 (* 3 4.0) "Mixed int*float multiplication")
(assert-equal 3.5 (+ 1.5 2) "Mixed float+int addition")
;; Division
(assert-equal 5.0 (/ 10 2) "Division always returns float")

(assert-true (< (- (/ 10 3) 3.333333) 0.00001) "Division with remainder")

(assert-equal 3 (quotient 10 3) "Integer quotient truncates")

;; Type coercion in operations
(define a 5)

(define b 2.5)

(assert-equal 7.5 (+ a b) "Integer promoted to float in mixed operation")
(assert-equal 50 (* a 10) "Integer multiplication stays integer")

;; Comparisons work with both types
(assert-true (> 5 3) "Greater than with integers")
(assert-true (> 5.0 3) "Greater than with float and integer")
(assert-true (< 2 3.5) "Less than with int and float")
(assert-true (= 5 5.0) "Equality between int and float (same value)")
(assert-true (= 5.0 5) "Equality between float and int (same value)")

;; Truthy/falsy behavior (note: 0 is truthy in this Lisp, only nil is falsy)
(define x 0)

(assert-equal 0 x "Variable holds integer 0")

(define y 5)

(assert-equal 5 y "Variable holds integer 5")
(assert-equal "yes" (if 0 "yes" "no")
 "0 is truthy in conditional (only nil is falsy)")
(assert-equal "yes" (if 1 "yes" "no") "Non-zero integer is truthy")
