;; Named Functions Examples
;; Demonstrate how function names appear in stack traces and REPL output
(load "tests/test-helpers.lisp")

;; ===========================================
;; Basic Named Functions
;; ===========================================
;; Define a simple named function
(define add (lambda (x y) (+ x y)))

;; add should print #<lambda:add>
(assert-equal 8 (add 3 5) "named function add")

;; Define factorial
(define factorial (lambda (n) (if (<= n 1) 1 (* n (factorial (- n 1))))))

;; factorial should print #<lambda:factorial>
(assert-equal 120 (factorial 5) "named factorial function")
;; ===========================================
;; Anonymous Lambdas (no name)
;; ===========================================
;; Anonymous lambda still works
(assert-equal 25 ((lambda (x) (* x x)) 5) "anonymous lambda")

;; Store anonymous lambda in variable
(define anon (lambda (x) (+ x 1)))

;; anon should print #<lambda:anon> (gets named from define)
;; ===========================================
;; Error Testing with Named Functions
;; ===========================================
;; Function that calls undefined variable
(define bad-func (lambda nil undefined-var))

;; Should show "bad-func" in stack trace
(assert-error (bad-func) "named function with undefined variable")

;; ===========================================
;; Nested Named Function Calls
;; ===========================================
(define outer (lambda nil (middle)))

(define middle (lambda nil (inner)))

(define inner (lambda nil (deepest)))

(define deepest (lambda nil unknown-symbol))

;; Should show full call chain: outer -> middle -> inner -> deepest
(assert-error (outer) "nested named function calls with error")

;; ===========================================
;; Named Functions with Parameters
;; ===========================================
(define greet
  (lambda (name) (define greeting (lambda nil (concat "Hello, " name)))
    (greeting)))

(assert-equal "Hello, World" (greet "World") "named function with closure")

;; ===========================================
;; Reassignment
;; ===========================================
(define func1 (lambda nil "first"))

;; func1 => #<lambda:func1>
(assert-equal "first" (func1) "initial function definition")

;; Redefine with new lambda
(define func1 (lambda nil "second"))

;; func1 => #<lambda:func1>
(assert-equal "second" (func1) "function redefinition")
;; ===========================================
;; Let bindings (lambdas stay anonymous)
;; ===========================================
(assert-equal 20 (let ((f (lambda (x) (* x 2)))) (f 10))
 "lambda in let binding")

;; The lambda in let stays anonymous
;; ===========================================
;; Higher-order functions
;; ===========================================
(define make-adder (lambda (x) (lambda (y) (+ x y))))

(define add5 (make-adder 5))

;; add5 => #<lambda:add5>
(assert-equal 15 (add5 10) "higher-order function")

;; The inner lambda from make-adder is anonymous
;; But when we define it as add5, it gets that name
;; ===========================================
;; Recursive named functions
;; ===========================================
(define fib (lambda (n) (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2))))))

(assert-equal 55 (fib 10) "recursive named function")
