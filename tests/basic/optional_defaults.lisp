(load "tests/test-helpers.lisp")

;; Simple optional default
(define f1 (lambda (x &optional (y 10)) (+ x y)))

(assert-equal 13 (f1 3) "optional default used")
(assert-equal 7 (f1 3 4) "optional default overridden")

;; Default expression referencing earlier param
(define f2 (lambda (a &optional (b (* a 2))) (+ a b)))

(assert-equal 9 (f2 3) "default references earlier param")
(assert-equal 8 (f2 3 5) "default overridden")

;; Multiple optional defaults
(define f3 (lambda (x &optional (y 10) (z 20)) (+ x y z)))

(assert-equal 33 (f3 3) "multiple defaults")
(assert-equal 28 (f3 3 5) "second default overridden")
(assert-equal 15 (f3 3 5 7) "all defaults overridden")

;; Required + optional + rest
(define f4 (lambda (a &optional (b 2) &rest rest) (list a b rest)))

(assert-equal (list 1 2 nil) (f4 1) "rest empty")
(assert-equal (list 1 3 (list 4 5)) (f4 1 3 4 5) "rest collected")

;; Error: non-list default pair
(assert-error (lambda (x &optional y z) x) "unparenthesized optional params")
