;; progn special form examples
;; progn evaluates multiple expressions sequentially and returns the last value
(load "tests/test-helpers.lisp")

;; Basic progn - returns last expression
(assert-equal 15 (+ (progn (+ 1 1) (+ 2 2) 10) 5) "progn returns last value")
;; progn with define operations
(assert-equal 30 (progn (define x 10) (define y 20) (+ x y))
 "progn with define operations")
;; progn with mixed operations building up values sequentially
(assert-equal 16 (let ((a 5)) (progn (define b (+ a 3)) (define c (* b 2)) c))
 "progn with sequential definitions")

;; Empty progn returns NIL (null value)
(assert-nil (progn) "empty progn returns nil")

;; Single expression progn returns that expression
(assert-equal 42 (progn 42) "single expression progn")
;; progn with conditional logic - evaluates all but returns last
(assert-equal 99 (progn (if (> 5 3) 1 0) (if (< 10 5) 2 0) 99)
 "progn evaluates all expressions but returns last")
