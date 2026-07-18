(load "tests/test-helpers.lisp")

;; Summation via named let
(assert-equal
 (let loop ((i 0)
            (sum 0))
   (if (< i 5)
     (loop (+ i 1) (+ sum i))
     sum)) 10
 "named let sum")

;; Factorial via named let
(assert-equal
 (let fact ((n 5)
            (acc 1))
   (if (= n 0)
     acc
     (fact (- n 1) (* acc n)))) 120
 "named let factorial")

;; No recursive call, just binding
(assert-equal
 (let no-recursion ((x 3))
   x) 3
 "named let no recursion")

;; Regular let still works
(assert-equal
 (let ((x 3)
       (y 2))
   (+ x y)) 5
 "regular let unchanged")
