;; Signal/catch benchmark: raise, stack trace construction, handler dispatch.
;; Not part of `make check`; run via tests/bench/run.sh.
(do ((i 0 (+ i 1))) ((= i 100000) 'done)
  (condition-case e (error "x") (error nil)))
