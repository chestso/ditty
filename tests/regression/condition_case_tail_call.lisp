;; Test: condition-case with tail-recursive calls
;; Regression test for bug where condition-case failed to catch errors in tail-recursive loops
;; The trampoline-based TCO bypassed the error handler's scope
(load "tests/test-helpers.lisp")

;; ---- Case 1: Named let loop with error in tail position (FAILS before fix) ----
;; The error occurs in tail position inside the loop, which gets TCO'd
(define caught-case1 #f)

(condition-case err (let loop ()
                      (error 'test-error)
                      (loop)) ; tail call
  (error (set! caught-case1 #t)))

(assert-true caught-case1 "Named let: error in tail position should be caught")

;; ---- Case 2: Non-tail position (should always work) ----
(define caught-case2 #f)

(condition-case err (progn (error 'test-error) 42)
  (error (set! caught-case2 #t)))

(assert-true caught-case2 "Non-tail position error should be caught")

;; ---- Case 3: do loop with error in body (not in tail position) ----
(define caught-case3 #f)

(define iterations 0)

(condition-case err
  (do ((i 0 (+ i 1))) ((>= i 3))
    (set! iterations (+ iterations 1))
    (when (= i 2) (error 'test-error)))
  (error (set! caught-case3 #t)))

(assert-true caught-case3 "do loop: error in body should be caught")

(assert-equal 3 iterations "do loop: ran 3 iterations before error")

;; ---- Case 4: Named let with error before tail call ----
(define caught-case4 #f)

(define case4-iters 0)

(condition-case err
  (let loop ((i 0))
    (set! case4-iters (+ case4-iters 1))
    (when (< i 2) (error 'test-error) ; error BEFORE tail call
      (loop (+ i 1))))
  (error (set! caught-case4 #t)))

(assert-true caught-case4 "Named let: error before tail call should be caught")

(assert-equal 1 case4-iters "Named let: stopped at first iteration")

;; ---- Case 5: Multiple tail calls before error ----
(define caught-case5 #f)

(define case5-iters 0)

(condition-case err
  (let loop ((i 0))
    (set! case5-iters (+ case5-iters 1))
    (if (< i 3)
      (loop (+ i 1)) ; tail call, no error
      (error 'final-error))) ; error after iterations
  (error (set! caught-case5 #t)))

(assert-true caught-case5
 "Named let: error after multiple tail calls should be caught")

(assert-equal 4 case5-iters "Named let: ran 4 iterations")

;; ---- Case 6: Nested condition-case inside tail call ----
(define outer-caught #f)

(define inner-caught #f)

(condition-case err
  (let loop ()
    (condition-case inner-err (error 'inner-error)
      (error (set! inner-caught #t)))
    (error 'outer-error) ; after inner handler returns
    (loop))
  (error (set! outer-caught #t)))

(assert-true inner-caught
 "Nested: inner error should be caught by inner handler")
(assert-true outer-caught
 "Nested: outer error should be caught by outer handler")

;; ---- Case 7: Handler returns value from tail position ----
(define result-case7
  (condition-case err (let loop ((i 0))
                        (if (< i 5)
                          (loop (+ i 1))
                          'success))
    (error 'handler-error)))

(assert-equal 'success result-case7
 "Tail recursive loop should return final value")

;; ---- Case 8: Specific error type matching in tail call ----
(define caught-specific #f)

(define caught-generic #f)

(condition-case err (let loop ()
                      (signal 'division-by-zero "test")
                      (loop))
  (division-by-zero (set! caught-specific #t))
  (error (set! caught-generic #t)))

(assert-true caught-specific "Specific error type should match in tail call")

(assert-false caught-generic
 "Generic handler should not run if specific matched")

;; ---- Case 9: Error var binding in tail call handler ----
(define error-var-value nil)

(condition-case err (let loop ()
                      (signal 'my-error "error message")
                      (loop))
  (error (set! error-var-value err)))

(assert-true (error? error-var-value)
 "Error var should be bound to error object")

(assert-equal 'my-error (error-type error-var-value) "Error type should match")
(assert-equal "error message" (error-message error-var-value)
 "Error message should match")
