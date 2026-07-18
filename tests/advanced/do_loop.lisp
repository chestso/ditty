;; do loop examples
;; Demonstrate the do loop construct for efficient iteration
(load "tests/test-helpers.lisp")

;; Simple counter from 0 to 9
(assert-equal 10 (do ((i 0 (+ i 1))) ((>= i 10) i))
 "simple counter with do loop")
;; Count up with side effects
(assert-equal "done" (do ((i 1 (+ i 1))) ((> i 5) "done") i)
 "do loop with side effects")
;; Multiple variables - count and accumulate sum
(assert-equal
 55 (do ((i 1 (+ i 1)) (sum 0)) ((> i 10) sum) (set! sum (+ sum i)))
 "do loop with multiple variables")

;; Compute factorial using do loop
(define factorial-do
  (lambda (n) (do ((i n (- i 1)) (acc 1)) ((<= i 0) acc) (set! acc (* acc i)))))

(assert-equal 120 (factorial-do 5) "factorial with do loop")
;; Count down from 10 to 1 with side effect
(assert-equal "blastoff" (do ((i 10 (- i 1))) ((<= i 0) "blastoff") i)
 "countdown with do loop")
;; Generate powers of 2 until > 1000
(assert-equal 1024 (do ((x 1 (* x 2))) ((> x 1000) x) x)
 "powers of 2 with do loop")
;; Pair-wise variables tracking in opposite directions
(assert-equal
 (do ((a 0 (+ a 1)) (b 10 (- b 1))) ((>= a 10) (list a b)) (progn a b)) '(10 0)
 "pair-wise variables in do loop")
;; Body is empty - just increment count until condition
(assert-equal 5 (do ((count 0 (+ count 1))) ((>= count 5) count))
 "do loop with empty body")

;; Sum of squares with multiple accumulators
(define sum-squares
  (lambda (n)
    (do ((i 1 (+ i 1)) (sum 0)) ((> i n) sum) (set! sum (+ sum (* i i))))))

(assert-equal 55 (sum-squares 5) "sum of squares with do loop")
;; Early exit when square equals 25
;; Note: i is incremented to 6 after found is set to #t (do loop semantics)
(assert-equal
 (do ((i 1 (+ i 1)) (found nil)) ((or (> i 10) found) (if found i nil))
   (if (= (* i i) 25) (set! found #t))) 6
 "early exit with do loop")
;; Immediate return (no increment, just test)
(assert-equal 5 (do ((x 5)) ((= x 5) x)) "immediate return from do loop")
;; Multiple result expressions in test clause (should behave like progn)
(assert-equal 3 (do ((i 0 (+ i 1))) ((= i 3) 1 2 3))
 "do returns last of multiple result expressions")

(define side-effect-tracker nil)

(assert-equal
 (do ((i 0 (+ i 1))) ((= i 2) (set! side-effect-tracker "ran") 42)) 42
 "do evaluates all result expressions for side effects")
(assert-equal "ran" side-effect-tracker
 "do result expression side effects are visible")
;; Single result expression still works
(assert-equal "only" (do ((i 0 (+ i 1))) ((= i 1) "only"))
 "do with single result expression")
;; No result expression returns nil
(assert-equal nil (do ((i 0 (+ i 1))) ((= i 1)))
 "do with no result expression returns nil")
