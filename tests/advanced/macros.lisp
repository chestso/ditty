;; Test basic macro functionality
(load "tests/test-helpers.lisp")

;; Test 1: Simple macro that returns a quoted expression
(defmacro simple-test nil '(+ 1 2))

(assert-equal 3 (simple-test) "simple macro")

;; Test 2: Macro with parameters
(defmacro double (x) (list '+ x x))

(assert-equal 10 (double 5) "macro with parameters")
(assert-equal 10 (double (+ 2 3)) "macro with expression parameter")

;; Test 3: Test defun macro - single body expression
(defun add-one (x) (+ x 1))

(assert-equal 6 (add-one 5) "defun with single body expression")

;; Test 4: Test defun macro - multiple body expressions
(defun test-multi (x) (+ x 1) (+ x 2) (+ x 3))

(assert-equal 13 (test-multi 10) "defun with multiple body expressions")

;; Test 5: Test defun with multiple parameters
(defun add-three (a b c) (+ a b c))

(assert-equal 6 (add-three 1 2 3) "defun with multiple parameters")

;; Test 6: Test defun creates named functions
(defun factorial (n) (if (= n 0) 1 (* n (factorial (- n 1)))))

(assert-equal 120 (factorial 5) "defun creates recursive functions")

;; Test 7: Macro that creates a conditional
(defmacro when (condition body) (list 'if condition body nil))

(assert-equal 42 (when #t 42) "when macro with true condition")

(assert-nil (when nil 42) "when macro with false condition")

;; Test 8: Verify macro doesn't evaluate arguments initially (but expansion is evaluated)
;; This macro receives (+ 1 2) unevaluated, returns it, then it gets evaluated
(defmacro pass-through (x) x)

(assert-equal 3 (pass-through (+ 1 2)) "pass-through macro evaluates expansion")

;; To return unevaluated form, macro must quote it
(defmacro quote-form (x) (list 'quote x))

(assert-equal '(+ 1 2) (quote-form (+ 1 2)) "macro that quotes form")
