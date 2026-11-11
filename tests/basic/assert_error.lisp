(load "tests/test-helpers.lisp")

;; passes when EXPR raises, however it raises
(assert-error (floor 7 0) "builtin error")

(define bad (lambda nil undefined-var))

(assert-error (bad) "user function error")

(define deep (lambda nil (bad)))

(assert-error (deep) "nested user function error")
(assert-error (error "boom") "explicit error")

;; fails when EXPR succeeds. The outer condition-case must hold the
;; assert-error form directly (not inside a progn), see the note above.
(assert-equal 'failed
 (condition-case e (assert-error (+ 1 2) "x") (error 'failed))
 "assert-error fails on a succeeding expression")
(assert-equal 'failed
 (condition-case e (assert-error (lambda (x) x) "x") (error 'failed))
 "assert-error fails when a lambda is merely created")
(assert-equal nil
 (condition-case e (assert-error (floor 7 0) "x") (error 'failed))
 "assert-error passes quietly when EXPR raises")
