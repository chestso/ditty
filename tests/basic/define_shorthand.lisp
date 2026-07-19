;; Tests for the (define (name args...) body...) function shorthand
;; and the parallel (defmacro (name args...) body...) macro shorthand.
;; These forms are equivalent to (define name (lambda args... body...))
;; and (defmacro name args... body...) respectively, and must work in
;; every package, not just user/core.
(load "tests/test-helpers.lisp")

;; ===========================================
;; Basic function shorthand
;; ===========================================
(define (square x) (* x x))

(assert-equal 25 (square 5) "basic single-arg shorthand")

;; ===========================================
;; Multi-arg function shorthand
;; ===========================================
(define (add a b) (+ a b))

(assert-equal 5 (add 2 3) "multi-arg shorthand")

;; ===========================================
;; Zero-arg function shorthand
;; ===========================================
(define (answer) 42)

(assert-equal 42 (answer) "zero-arg shorthand")

;; ===========================================
;; Multi-expression body: returns last expression
;; ===========================================
(define (debug-double x) (* x 2))

(assert-equal 10 (debug-double 5) "multi-expression body returns last expr")

;; ===========================================
;; Docstring: first body expr is a string, more exprs follow
;; ===========================================
(define (g x) "Double the argument." (* x 2))

(assert-equal 20 (g 10) "docstring shorthand still computes value")

(assert-true (string? (documentation 'g)) "documentation returns the docstring")

(assert-equal "Double the argument." (documentation 'g)
 "documentation content matches docstring")

;; ===========================================
;; Lambda name is attached for debugging
;; ===========================================
(define (named-fn x) x)

(assert-equal "named-fn" (function-name named-fn)
 "shorthand attaches the function name to the lambda")

;; ===========================================
;; Recursion works (function can call itself by name)
;; ===========================================
(define (fact n) (if (= n 0) 1 (* n (fact (- n 1)))))

(assert-equal 120 (fact 5) "shorthand supports recursion")

;; ===========================================
;; Optional and rest parameters work (delegates to lambda parser)
;; ===========================================
(define (vararg-fn a &rest rest) (list a rest))

(assert-equal '(1 (2 3 4)) (vararg-fn 1 2 3 4) "shorthand supports &rest")

(define (opt-fn a b &optional (c 100)) (+ a b c))

(assert-equal 103 (opt-fn 1 2) "shorthand supports &optional with default")
(assert-equal 6 (opt-fn 1 2 3) "shorthand &optional default can be overridden")

;; ===========================================
;; Closure over surrounding variables
;; ===========================================
(define (make-adder n) (lambda (x) (+ x n)))

(define add5 (make-adder 5))

(assert-equal 15 (add5 10) "shorthand can return closures")

;; ===========================================
;; Function shorthand inside a non-user/core package
;; ===========================================
(in-package 'shorthand-test-pkg)

(define (pkg-fn x) (* x 10))

(assert-equal 30 (pkg-fn 3) "shorthand works inside a custom package")
(assert-equal 'shorthand-test-pkg (current-package)
 "package is correctly set during shorthand definition")

;; Cross-package access from user using the pkg:name form
(in-package 'user)

(assert-equal 30 (shorthand-test-pkg:pkg-fn 3)
 "shorthand-defined fn is reachable cross-package via pkg:name")

;; ===========================================
;; defmacro shorthand: (defmacro (name args...) body...)
;; ===========================================
(defmacro (swap! a b) `(let ((tmp ,a)) (set! ,a ,b) (set! ,b tmp)))

(define sx 1)

(define sy 2)

(swap! sx sy)

(assert-equal 2 sx "defmacro shorthand swap! swaps first var")
(assert-equal 1 sy "defmacro shorthand swap! swaps second var")

;; defmacro shorthand with docstring
(defmacro (when2 cond . body) "Execute body when cond is true."
  `(if ,cond (progn ,@body) nil))

(assert-true (string? (documentation 'when2))
 "defmacro shorthand attaches docstring")

(assert-equal "Execute body when cond is true." (documentation 'when2)
 "defmacro shorthand docstring content matches")
(assert-equal 42 (when2 #t 42) "defmacro shorthand expands and evaluates")

(assert-nil (when2 nil 42) "defmacro shorthand returns nil on falsy condition")

;; defmacro shorthand inside a non-user/core package
(in-package 'macro-test-pkg)

(defmacro (double-it x) `(* 2 ,x))

(assert-equal 14 (double-it 7) "defmacro shorthand works in a custom package")

(in-package 'user)

;; ===========================================
;; Error cases
;; ===========================================
;; Non-symbol function name in the shorthand position
(assert-error (eval (read "(define (123 x) x)"))
 "define shorthand rejects non-symbol function name")

;; Empty body
(assert-error (eval (read "(define (no-body x))"))
 "define shorthand rejects empty body")

;; Non-symbol macro name in defmacro shorthand
(assert-error (eval (read "(defmacro (456 x) x)"))
 "defmacro shorthand rejects non-symbol macro name")

;; ===========================================
;; Simple (define name value) form still works (regression)
;; ===========================================
(define plain-value 99)

(assert-equal 99 plain-value "plain define still works")

(define plain-fn (lambda (x) (* x 3)))

(assert-equal 9 (plain-fn 3) "plain define with lambda still works")

(print "All define/defmacro shorthand tests passed.")
