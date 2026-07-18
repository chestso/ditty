(load "tests/test-helpers.lisp")

;; Test that sleep exists and returns truthy
(assert-true (sleep 1) "sleep should return truthy")

;; Test that sleep actually waits (allow 100ms tolerance)
(define start (current-time-ms))

(sleep 50)

(define elapsed (- (current-time-ms) start))

(assert-true (>= elapsed 40) "sleep 50 should wait at least 40ms")
(assert-true (< elapsed 500) "sleep 50 should not take 500ms")

;; Test zero sleep
(assert-true (sleep 0) "sleep 0 should return truthy")

;; Test invalid arguments
(assert-error (sleep -1) "sleep should reject negative duration")
(assert-error (sleep "foo") "sleep should reject non-integer")
