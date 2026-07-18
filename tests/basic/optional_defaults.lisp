(load "tests/test-helpers.lisp")

;; Simple optional default
(define f1 (lambda (x &optional (y 10)) (+ x y)))

(assert-equal (f1 3) 13 "optional default used")
(assert-equal (f1 3 4) 7 "optional default overridden")

;; Default expression referencing earlier param
(define f2 (lambda (a &optional (b (* a 2))) (+ a b)))

(assert-equal (f2 3) 9 "default references earlier param")
(assert-equal (f2 3 5) 8 "default overridden")

;; Multiple optional defaults
(define f3 (lambda (x &optional (y 10) (z 20)) (+ x y z)))

(assert-equal (f3 3) 33 "multiple defaults")
(assert-equal (f3 3 5) 28 "second default overridden")
(assert-equal (f3 3 5 7) 15 "all defaults overridden")

;; Required + optional + rest
(define f4 (lambda (a &optional (b 2) &rest rest) (list a b rest)))

(assert-equal (f4 1) (list 1 2 nil) "rest empty")
(assert-equal (f4 1 3 4 5) (list 1 3 (list 4 5)) "rest collected")

;; Error: non-list default pair
(assert-error (lambda (x &optional y z) x) "unparenthesized optional params")
