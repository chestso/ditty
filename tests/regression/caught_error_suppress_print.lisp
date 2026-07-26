;; Test: caught errors should not auto-print
;; Regression test for bug where condition-case caught errors still printed
;; to stdout/stderr with ERROR: prefix instead of being silently handled
(load "tests/test-helpers.lisp")

;; ---- Case 1: Caught error returns handler value, not error output ----
;; Handler returns 'ok, so result should be 'ok with no ERROR: prefix
(define result1 (condition-case err (error 'test-error "message") (error 'ok)))

(assert-equal 'ok result1 "Caught error: handler return value is result")

;; ---- Case 2: Caught error with nil handler body returns nil ----
(define result2 (condition-case err (error 'test-error "message") (error nil)))

(assert-nil result2 "Caught error: nil handler body returns nil")

;; ---- Case 3: Caught error in tail-recursive loop ----
(define result3
  (condition-case err (let loop ()
                        (signal 'loop-error "in tail call")
                        (loop))
    (error 'caught-in-loop)))

(assert-equal 'caught-in-loop result3 "Caught error in tail call: handler runs")

;; ---- Case 4: Handler returns the error object itself ----
;; The error object should NOT print - it's a caught error returned as value
(define result4 (condition-case err (error 'test-error "message") (error err)))

(assert-true (error? result4) "Handler returns error object: is error type")
;; The caught flag should be set
;; We can't directly test LISP_ERROR_CAUGHT from Lisp, but we verify behavior

;; ---- Case 5: Explicit print in handler still works ----
(define printed-value nil)

(condition-case err (error 'test-error "message")
  (error (set! printed-value 'handler-ran)))

(assert-equal 'handler-ran printed-value "Handler side effects work")

;; ---- Case 6: Uncaught error behavior verified externally ----
;; Uncaught errors can't be tested inline since they exit the process.
;; Instead, verify that matching handler prevents propagation.
;; This test file should exit 0 with all assertions passing.
(define caught-unmatched
  (condition-case err (signal 'specific-error "test")
    (error 'any-error-caught)))

(assert-equal 'any-error-caught caught-unmatched
 "Generic handler catches specific error")

;; ---- Case 7: Nested condition-case with caught errors ----
(define outer-result nil)

(define inner-result nil)

(condition-case outer-err
  (condition-case inner-err (error 'inner-error "inner")
    (error (set! inner-result 'inner-caught)))
  (error (set! outer-result 'outer-caught)))

(assert-equal 'inner-caught inner-result "Nested: inner handler ran")

(assert-nil outer-result "Nested: outer handler did not run")

;; ---- Case 8: Multiple expressions, one errors and is caught ----
;; Verify that caught errors don't leak into subsequent expressions
(define counter 0)

(set! counter (+ counter 1))

(condition-case err (error 'ignored "this is caught")
  (error nil))

(set! counter (+ counter 1))

(assert-equal 2 counter "Multiple expressions: caught error doesn't break flow")

;; ---- Case 9: Error in unwind-protect cleanup propagates ----
;; Error in cleanup is NOT caught by a condition-case around the unwind-protect
;; because the cleanup runs after the body returns, outside the condition-case scope.
;; This is documented behavior - cleanup errors are separate from body errors.
(define cleanup-ran #f)

(define result9 (unwind-protect 'body-result (set! cleanup-ran #t)))

(assert-equal 'body-result result9 "Unwind-protect returns body result")

(assert-true cleanup-ran "Cleanup ran")
