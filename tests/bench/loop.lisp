;; Tail-recursive loop benchmark: TCO trampoline, environments, arithmetic.
;; Not part of `make check`; run via tests/bench/run.sh.
(define (count n acc) (if (= n 0) acc (count (- n 1) (+ acc 1))))

(count 3000000 0)
