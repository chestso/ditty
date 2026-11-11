;; Recursive call benchmark: function application and arithmetic.
;; Not part of `make check`; run via tests/bench/run.sh.
(define (fib n) (if (< n 2) n (+ (fib (- n 1)) (fib (- n 2)))))

(fib 27)
