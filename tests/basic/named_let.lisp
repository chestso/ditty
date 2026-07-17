(load "tests/test-helpers.lisp")

;; Summation via named let
(assert-equal 10
 (let loop ((i 0)
            (sum 0))
   (if (< i 5)
     (loop (+ i 1) (+ sum i))
     sum))
 "named let sum")

;; Factorial via named let
(assert-equal 120
 (let fact ((n 5)
            (acc 1))
   (if (= n 0)
     acc
     (fact (- n 1) (* acc n))))
 "named let factorial")

;; No recursive call, just binding
(assert-equal 3
 (let no-recursion ((x 3))
   x)
 "named let no recursion")

;; Regular let still works
(assert-equal 5
 (let ((x 3)
       (y 2))
   (+ x y))
 "regular let unchanged")
