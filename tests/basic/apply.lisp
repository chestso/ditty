;; Tests for (apply fn args-list)
(load "tests/test-helpers.lisp")

;; Apply with builtins
(assert-equal 6 (apply + (list 1 2 3)) "apply builtin +")
(assert-equal 24 (apply * (list 2 3 4)) "apply builtin *")
(assert-equal 1 (apply car (list (list 1 2))) "apply builtin car")
(assert-equal '(1 2 3) (apply list (list 1 2 3)) "apply builtin list")
;; Apply with lambda
(assert-equal 7 (apply (lambda (a b) (+ a b)) (list 3 4)) "apply lambda")
(assert-equal 25 (apply (lambda (x) (* x x)) (list 5)) "apply lambda unary")
;; Apply with no args
(assert-equal 42 (apply (lambda () 42) nil) "apply lambda no args")
;; Apply with optional parameters
(assert-equal '(1 nil) (apply (lambda (a &optional b) (list a b)) (list 1))
 "apply lambda optional not provided")
(assert-equal '(1 2) (apply (lambda (a &optional b) (list a b)) (list 1 2))
 "apply lambda optional provided")
;; Apply with rest parameters
(assert-equal
 '(1 (2 3)) (apply (lambda (a &rest more) (list a more)) (list 1 2 3))
 "apply lambda rest")
(assert-equal '(1 2 3) (apply (lambda (&rest all) all) (list 1 2 3))
 "apply lambda rest only")

;; Apply with named function
(define double (lambda (x) (* x 2)))

(assert-equal 14 (apply double (list 7)) "apply named function")

;; Error cases
(assert-error (apply 42 (list 1)) "apply non-function")
(assert-error (apply + 5) "apply non-list args")
