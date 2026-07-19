;; Tail Recursion and Tail Call Optimization Tests
;; Tests that tail-recursive functions don't exhaust the stack
(load "tests/test-helpers.lisp")

;; Test 1: Tail-recursive factorial
;; Traditional recursive factorial would stack overflow at large N
(define factorial-tail
  (lambda (n acc) (if (<= n 1) acc (factorial-tail (- n 1) (* n acc)))))

(assert-equal 120 (factorial-tail 5 1) "tail-recursive factorial 5")
(assert-equal 3628800 (factorial-tail 10 1) "tail-recursive factorial 10")

;; Test 2: Very large tail recursion (would stack overflow without TCO)
;; Recurses 100000 deep but keeps the accumulator small (bounded by N,
;; never multiplies), so it exercises deep TCO without integer overflow.
(define count-down-tail
  (lambda (n acc) (if (= n 0) acc (count-down-tail (- n 1) (+ acc 1)))))

(assert-equal 100000 (count-down-tail 100000 0)
 "tail-recursive count 100000 (deep TCO)")

;; Test 3: Tail-recursive sum (count down)
(define sum-to-n (lambda (n acc) (if (= n 0) acc (sum-to-n (- n 1) (+ n acc)))))

(assert-equal 55 (sum-to-n 10 0) "tail-recursive sum 10")
(assert-equal 5050 (sum-to-n 100 0) "tail-recursive sum 100")
(assert-equal 50005000 (sum-to-n 10000 0) "tail-recursive sum 10000")

;; Test 4: Tail-recursive length
(define length-tail
  (lambda (lst acc) (if (null? lst) acc (length-tail (cdr lst) (+ acc 1)))))

(assert-equal 5 (length-tail (list 1 2 3 4 5) 0) "tail-recursive length")

;; Test 5: Mutual recursion (even?/odd?) - tests general tail call optimization
(define is-even? (lambda (n) (if (= n 0) #t (is-odd? (- n 1)))))

(define is-odd? (lambda (n) (if (= n 0) nil (is-even? (- n 1)))))

(assert-true (is-even? 10) "is-even? 10")

(assert-false (is-odd? 10) "is-odd? 10")
(assert-false (is-even? 99) "is-even? 99")

(assert-true (is-odd? 99) "is-odd? 99")
;; Large mutual recursion (would stack overflow without TCO)
(assert-true (is-even? 10000) "is-even? 10000 (large)")
(assert-true (is-odd? 10001) "is-odd? 10001 (large)")

;; Test 6: Tail recursion with multiple returns
(define find-first
  (lambda (predicate lst)
    (if (null? lst)
      nil
      (if (predicate (car lst)) (car lst) (find-first predicate (cdr lst))))))

(assert-equal 6 (find-first (lambda (x) (> x 5)) (list 1 2 3 6 7 8))
 "find-first tail recursion")

;; Test 7: Tail recursion in let body
;; SKIPPED: Recursive let bindings not supported (requires letrec)
;; (define test-let-tail
;;   (lambda (n)
;;     (let ((helper (lambda (x acc)
;;                     (if (<= x 0)
;;                       acc
;;                       (helper (- x 1) (+ acc x))))))
;;       (helper n 0))))
;;
;; (assert-equal  5050 (test-let-tail 100)  "tail recursion in let body")
;; TODO: Add letrec support or rewrite test without recursive let bindings
;; Test 8: Tail recursion through cond
(define collatz-length
  (lambda (n acc)
    (cond
      ((= n 1) acc)
      ((= (remainder n 2) 0) (collatz-length (quotient n 2) (+ acc 1)))
      (else (collatz-length (+ (* 3 n) 1) (+ acc 1))))))

(assert-equal 6 (collatz-length 10 0) "collatz length 10")
(assert-equal 111 (collatz-length 27 0) "collatz length 27")

;; Test 9: Tail recursion through case
(define count-down-case
  (lambda (n acc) (case n ((0) acc) (else (count-down-case (- n 1) (+ acc 1))))))

(assert-equal 10 (count-down-case 10 0) "tail recursion through case")

;; Test 10: Tail recursion through progn (last expression)
(define progn-tail
  (lambda (n acc)
    (progn (define temp (+ n 1))
      (if (<= n 0) acc (progn-tail (- n 1) (+ acc n))))))

(assert-equal 15 (progn-tail 5 0) "tail recursion through progn")
