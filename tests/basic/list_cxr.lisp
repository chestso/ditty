(load "tests/test-helpers.lisp")

;; The c*r accessors: each is the composition its name spells, applied
;; right to left, and (like car and cdr) gives nil past the end of a list.
(define nested '((1 2) (3 4) 5))

(assert-equal 1 (caar nested) "caar is the car of the car")
(assert-equal '(3 4) (cadr nested) "cadr is the car of the cdr")
(assert-equal '(2) (cdar nested) "cdar is the cdr of the car")
(assert-equal '(5) (cddr nested) "cddr is the cdr of the cdr")
(assert-equal 5 (caddr nested) "caddr is the third element")

(define five '(1 2 3 4 5))

(assert-equal 4 (cadddr five) "cadddr is the fourth element")
(assert-equal '(4 5) (cdddr five) "cdddr drops the first three elements")
(assert-equal '(4 5) (cdr (cddr five)) "cdddr is (cdr (cddr x))")
(assert-equal '(3 4 5) (cddr five) "cddr drops the first two elements")

;; cdddr stops at the end of a list instead of failing.
(assert-nil (cdddr '(a b c)) "cdddr of a three-element list is nil")
(assert-nil (cdddr '(a b)) "cdddr of a two-element list is nil")
(assert-nil (cdddr '()) "cdddr of the empty list is nil")

(assert-equal 4 (cdddr '(1 2 3 . 4)) "cdddr of an improper list is its tail")

;; The same argument rules as the other c*r accessors.
(assert-error (cdddr) "cdddr requires an argument")
