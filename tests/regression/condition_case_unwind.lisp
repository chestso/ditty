;; Test: condition-case and unwind-protect really unwind.
;; Regression tests for error-as-value propagation: a signal must transfer
;; control to the innermost matching handler, and nothing between the signal
;; and the handler may run.
(load "tests/test-helpers.lisp")

(define (boom) (error "boom"))

;; --- Nothing after the signal runs --------------------------------------
(define reached #f)

(assert-equal 'caught
 (condition-case e (progn (boom) (set! reached #t) 'more) (error 'caught))
 "progn stops at the signal")

(assert-false reached "code after the signal is skipped")

(assert-equal 'caught (condition-case e (if (boom) 'yes 'no) (error 'caught))
 "an if test cannot turn a signal into a branch")
(assert-equal 'caught (condition-case e (list 1 (boom) 2) (error 'caught))
 "arguments are not built; list never runs")
(assert-equal 'caught (condition-case e (let ((r (boom))) r) (error 'caught))
 "let bindings abort")

(define iterations 0)

(assert-equal 'caught
 (condition-case e
   (do ((i 0 (+ i 1))) ((= i 5) 'done)
     (set! iterations (+ iterations 1))
     (when (= i 2) (boom)))
   (error 'caught))
 "do loop is abandoned")
(assert-equal 3 iterations "the loop body ran for i = 0,1,2 only")

(define seen nil)

(assert-equal 'caught
 (condition-case e
   (map (lambda (x) (set! seen (cons x seen)) (if (= x 2) (boom) x))
    '(1 2 3))
   (error 'caught))
 "map is abandoned")
(assert-equal '(2 1) seen "map stops calling the callback at the first error")

(define (deep n) (if (= n 0) (boom) (+ 1 (deep (- n 1)))))

(assert-equal 'caught (condition-case e (deep 1000) (error 'caught))
 "one raise skips a thousand frames")

;; assert-error now sees errors in any body shape.
(assert-error (progn (boom) 'unreached) "errors in a non-final progn form")

;; --- Handlers that raise -------------------------------------------------
;; Regression: this used to SIGSEGV.
(assert-equal 'outer
 (condition-case outer (condition-case inner (boom) (error (boom)))
   (error 'outer))
 "a handler that raises through a call reaches the outer handler")
(assert-equal 'outer
 (condition-case outer
   (condition-case inner (boom) (error (error "from handler")))
   (error 'outer))
 "a handler that raises directly reaches the outer handler")

;; --- Error objects are data, never signals --------------------------------
(define captured (condition-case e (boom) (error e)))

(assert-true (error? captured) "handler returns the error object")
(assert-true (error? (car (list captured)))
 "car returns an error object as data")
(assert-true (error? (cdr (cons 1 captured)))
 "cdr returns an error object as data")
(assert-true (error? (apply (lambda (x) x) (list captured)))
 "apply returns an error object as data")
(assert-true (error? (car (map (lambda (x) x) (list captured))))
 "a map callback may return an error object")

(assert-equal 1 (length (filter (lambda (x) #t) (list captured)))
 "a filter predicate input may be an error object")

(assert-true (error? (eval 'captured)) "eval returns an error object as data")

(assert-equal "#<error error \"boom\">" (format nil "~S" captured)
 "an error value prints as data, not as the ERROR: diagnostic")

;; --- unwind-protect --------------------------------------------------------
(define trail nil)

(assert-equal 'caught
 (condition-case e
   (unwind-protect
     (progn (unwind-protect (boom) (set! trail (append trail '(inner))))
       (set! trail (append trail '(unreached))))
     (set! trail (append trail '(outer))))
   (error 'caught))
 "cleanups run on the way out")
(assert-equal '(inner outer) trail
 "innermost first, once, skipped forms stay skipped")
(assert-equal 'cleanup-error
 (condition-case e
   (unwind-protect (boom) (signal 'cleanup-error "cleanup failed"))
   (error (error-type e)))
 "a raising cleanup replaces the pending error")

;; Regression for the pending-error clobber: a cleanup that raises and catches
;; its own error must not disturb the error being re-raised.
(assert-equal "boom"
 (condition-case e
   (unwind-protect (boom) (condition-case x (error "inner") (error nil)))
   (error (error-message e)))
 "the original error survives a cleanup that catches internally")

(define n 0)

(assert-equal 'caught
 (unwind-protect (condition-case e (boom) (error (set! n (+ n 1)) 'caught))
   (set! n (+ n 10)))
 "condition-case inside unwind-protect")
(assert-equal 11 n "handler ran once, cleanup ran once on the normal path")

;; --- Matching --------------------------------------------------------------
(assert-equal 'outer
 (condition-case outer
   (condition-case inner (signal 'specific "x") (other 'nope))
   (specific 'outer))
 "no matching handler keeps raising")

;; --- Re-signal an error object --------------------------------------------
(define inner-obj nil)

(define outer-obj
  (condition-case o
    (condition-case i (boom) (error (set! inner-obj i) (signal i)))
    (error o)))

(assert-true (eq? inner-obj outer-obj) "(signal ERR) re-raises the same object")

(assert-equal "boom" (error-message outer-obj) "message survives a re-signal")

;; --- Handler variable is local ---------------------------------------------
(define ev 'outer-value)

(condition-case ev (boom) (error nil))

(assert-equal 'outer-value ev
 "the handler variable does not clobber an outer binding")

(condition-case o (condition-case ev (boom) (error (error "h"))) (error nil))

(assert-equal 'outer-value ev
 "a raising handler leaves the outer binding intact")

(define get-msg (condition-case e (boom) (error (lambda () (error-message e)))))

(assert-equal "boom" (get-msg)
 "a closure created in a handler keeps the variable")

;; --- Dynamic state is restored by the catching frame -------------------------
(define (trace-length thunk)
  (condition-case e (thunk) (error (length (error-stack e)))))

(define baseline (trace-length boom))

(do ((i 0 (+ i 1))) ((= i 30) 'done)
  (condition-case e (map (lambda (x) (boom)) '(1)) (error nil)))

(assert-equal baseline (trace-length boom)
 "errors that crossed a builtin leave no phantom call-stack frames")
(assert-equal 'caught (condition-case e (deep 50) (error 'caught))
 "still works")
(assert-equal baseline (trace-length boom)
 "and a deep unwind leaves none either")

;; --- Tail calls --------------------------------------------------------------
(define calls 0)

(assert-equal 'caught
 (condition-case e
   (let loop ((i 0))
     (set! calls (+ calls 1))
     (if (= i 100000) (boom) (loop (+ i 1))))
   (error 'caught))
 "a raise from the end of a tail-recursive loop is caught once")
(assert-equal 100001 calls "the loop ran to the signal and stopped")
