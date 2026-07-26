;; Test: unwind-protect with tail-recursive calls
;; Regression test for bug where cleanup forms don't run when errors occur
;; in tail-recursive loops. The trampoline-based TCO bypasses cleanup.
(load "tests/test-helpers.lisp")

;; ---- Case 1: Error in tail-recursive loop (FAILS before fix) ----
(define cleanup-case1 #f)

(condition-case err
  (unwind-protect (let loop ()
                    (signal 'test-error "in tail call")
                    (loop))
    (set! cleanup-case1 #t))
  (error nil))

(assert-true cleanup-case1 "Error in tail-recursive loop: cleanup should run")

;; ---- Case 2: Non-tail error (should always work) ----
(define cleanup-case2 #f)

(condition-case err
  (unwind-protect (signal 'test-error "non-tail") (set! cleanup-case2 #t))
  (error nil))

(assert-true cleanup-case2 "Non-tail error: cleanup should run")

;; ---- Case 3: Successful tail recursion (should work) ----
(define cleanup-case3 #f)

(define result-case3
  (unwind-protect (let loop ((i 0))
                    (if (< i 5)
                      (loop (+ i 1))
                      'done))
    (set! cleanup-case3 #t)))

(assert-true cleanup-case3 "Successful tail recursion: cleanup should run")

(assert-equal 'done result-case3
 "Successful tail recursion: should return result")

;; ---- Case 4: Nested unwind-protect with error in tail call ----
(define outer-cleanup #f)

(define inner-cleanup #f)

(condition-case err
  (unwind-protect
    (unwind-protect (let loop ()
                      (signal 'nested-error "nested")
                      (loop))
      (set! inner-cleanup #t))
    (set! outer-cleanup #t))
  (error nil))

(assert-true inner-cleanup "Nested: inner cleanup should run")
(assert-true outer-cleanup "Nested: outer cleanup should run")

;; ---- Case 5: Multiple iterations before error ----
(define cleanup-case5 #f)

(define iterations-case5 0)

(condition-case err
  (unwind-protect
    (let loop ((i 0))
      (set! iterations-case5 (+ iterations-case5 1))
      (if (< i 10)
        (loop (+ i 1)) ; tail call, no error
        (signal 'final-error "done iterating")))
    (set! cleanup-case5 #t))
  (error nil))

(assert-true cleanup-case5 "Multiple iterations: cleanup should run")

(assert-equal 11 iterations-case5 "Multiple iterations: ran 11 times")

;; ---- Case 6: unwind-protect + condition-case interaction ----
(define cleanup-case6 #f)

(define handler-ran #f)

(condition-case err
  (unwind-protect (let loop ()
                    (signal 'interaction-error "test")
                    (loop))
    (set! cleanup-case6 #t))
  (error (set! handler-ran #t)))

(assert-true cleanup-case6 "Mixed: cleanup should run before handler")
(assert-true handler-ran "Mixed: handler should run after cleanup")

;; ---- Case 7: Error in cleanup form (check behavior) ----
;; Current behavior: error in cleanup aborts remaining cleanups
;; This test documents that behavior
(define cleanup-case7a #f)

(define cleanup-case7b #f)

(condition-case err
  (unwind-protect
    (unwind-protect (signal 'body-error "body") (set! cleanup-case7a #t)
      (signal 'cleanup-error "inner cleanup"))
    (set! cleanup-case7b #t))
  (error nil))

(assert-true cleanup-case7a "Error in cleanup: inner cleanup started")
;; cleanup-case7b may or may not run depending on error-in-cleanup handling
;; We document current behavior: if inner cleanup errors, outer may not run

;; ---- Case 8: Multiple cleanups run in correct order ----
;; Cleanups run from innermost to outermost
;; Since we cons onto the list, the result is (outer middle inner)
(define order '())

(condition-case err
  (unwind-protect
    (unwind-protect
      (unwind-protect (let loop ()
                        (signal 'order-error "test")
                        (loop))
        (set! order (cons 'inner order)))
      (set! order (cons 'middle order)))
    (set! order (cons 'outer order)))
  (error nil))

(assert-equal '(outer middle inner) order "Cleanups run innermost-to-outermost")
