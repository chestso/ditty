;;; Test short-circuiting and/or operators and list? predicate
;; Load test helper macros
(load "tests/test-helpers.lisp")

;;; Basic and tests
(assert-true (and) "and with no arguments returns true")
(assert-true (and #t) "and with single true argument returns true")

(assert-false (and nil) "and with single false argument returns false")

(assert-equal 3 (and 1 2 3) "and returns last value when all truthy")

(assert-false (and 1 nil 3) "and returns false when middle value is false")
(assert-false (and nil 1 3) "and returns false when first value is false")

;;; Basic or tests
(assert-nil (or) "or with no arguments returns nil")

(assert-false (or nil) "or with single false argument returns false")

(assert-true (or #t) "or with single true argument returns true")

(assert-equal 3 (or nil nil 3) "or returns first truthy value")
(assert-equal 1 (or 1 2 3) "or returns first value when truthy")
(assert-equal 1 (or nil 1 3) "or skips false and returns first truthy")

;;; Short-circuit tests for and
;;; These would cause errors if not short-circuited
(assert-false (and nil (/ 1 0))
 "and short-circuits on false, avoiding division by zero")
(assert-false (and nil (error "should not evaluate"))
 "and short-circuits on false, avoiding error")

;;; Short-circuit tests for or
(assert-true (or #t (/ 1 0))
 "or short-circuits on true, avoiding division by zero")

(assert-equal 1 (or 1 (error "should not evaluate"))
 "or short-circuits on truthy, avoiding error")
;;; Test that and/or return the actual value (not just #t/#f)
(assert-equal "foo" (and 1 2 "foo")
 "and returns actual last value, not just true")
(assert-equal "bar" (or nil nil "bar") "or returns actual first truthy value")
(assert-equal 0 (or 0 "baz") "or returns 0 (truthy in this Lisp)")
;;; Truthy/falsy semantics (Note: 0 and "" are TRUTHY, but '() is FALSY (same as nil))
(assert-equal 5 (and 0 5) "and with 0 (truthy) returns second value")
(assert-equal 5 (and "" 5)
 "and with empty string (truthy) returns second value")

(assert-nil (and 'nil 5) "and with empty list (falsy) returns nil")

;;; list? predicate tests
(assert-true (list? 'nil) "empty list is a list")
(assert-true (list? nil) "nil is a list")
(assert-true (list? '(1 2 3)) "proper list is a list")
(assert-true (list? (cons 1 2)) "cons cell is a list")

(assert-nil (list? 1) "integer is not a list")
(assert-nil (list? "string") "string is not a list")
(assert-nil (list? #(1 2 3)) "vector is not a list")
(assert-nil (list? #t) "boolean true is not a list")

(assert-true (list? nil) "boolean false (#f = nil) is a list (the empty list)")
;;; pair? predicate tests
(assert-true (pair? '(1 . 2)) "dotted pair is a pair")
(assert-true (pair? (cons 1 2)) "cons cell is a pair")
(assert-true (pair? '(a b c)) "proper list is a pair (cons cell)")
(assert-true (pair? '(1 2)) "list with elements is a pair")

(assert-nil (pair? nil) "nil is not a pair")
(assert-nil (pair? 'nil) "empty list is not a pair")
(assert-nil (pair? 42) "integer is not a pair")
(assert-nil (pair? "string") "string is not a pair")
(assert-nil (pair? #(1 2 3)) "vector is not a pair")
(assert-nil (pair? #t) "boolean true is not a pair")
(assert-nil (pair? nil) "boolean false is not a pair")

(assert-true (pair? (cons 'a 'b)) "cons of symbols is a pair")
(assert-true (pair? (cons 1 nil)) "cons with nil cdr is a pair")

;;; Nested and/or
(assert-equal 2 (and (or nil 1) (or 2 nil)) "nested and/or evaluates correctly")
(assert-equal 3 (or (and nil 1) (and 2 3)) "nested or/and evaluates correctly")

;;; Complex short-circuit example
(define x 0)

(assert-equal 20 (and #t (set! x 10) (set! x 20))
 "and evaluates all expressions when all truthy")
(assert-equal 20 x "x was set to 20 by and expression")

(set! x 0)

(assert-false (and nil (set! x 10) (set! x 20)) "and stops on false")

(assert-equal 0 x "x was not modified by short-circuited expressions")

(set! x 0)

(assert-equal 10 (or nil (set! x 10) (set! x 20))
 "or returns first truthy value")
(assert-equal 10 x "x was set to 10 by or expression")

(set! x 0)

(assert-true (or #t (set! x 10) (set! x 20))
 "or returns first truthy value (true)")

(assert-equal 0 x "x was not modified by short-circuited expressions")
