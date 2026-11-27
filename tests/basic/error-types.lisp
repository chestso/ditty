;; Runtime condition taxonomy and hierarchy matching.
;; The type set is a small Common Lisp-leaning subset (see ERROR_TYPES in
;; include/lisp.h). condition-case matches a handler against the signal
;; type's whole ancestor chain, first matching clause wins, and `error` is
;; the root that catches everything.
(load "tests/test-helpers.lisp")

;;; Generic error keeps the simple-error type; `error` catches it.
(define e nil)
(condition-case err (error "boom") (error (set! e err)))
(assert-equal 'simple-error (error-type e) "error function signals simple-error")

;;; A child type is caught by its parent handler.
(assert-equal 'caught
  (condition-case err (signal 'division-by-zero "x")
    (arithmetic-error 'caught))
  "arithmetic-error handler catches division-by-zero")

(assert-equal 'caught
  (condition-case err (signal 'division-by-zero "x")
    (error 'caught))
  "error root catches division-by-zero")

(assert-equal 'caught
  (condition-case err (signal 'unclosed-input "x")
    (reader-error 'caught))
  "reader-error handler catches unclosed-input")

;;; A parent is NOT caught by a child handler (no downward matching).
(assert-equal 'propagated
  (condition-case outer
      (condition-case inner (signal 'reader-error "x")
        (unclosed-input 'wrong))
    (error 'propagated))
  "child handler does not catch a parent type")

;;; Unregistered user types: exact match and error catch-all both work.
(assert-equal 'exact
  (condition-case err (signal 'my-error "x")
    (my-error 'exact))
  "unregistered type matches its exact handler")

(assert-equal 'catchall
  (condition-case err (signal 'my-error "x")
    (error 'catchall))
  "error catches an unregistered type")

;;; An unregistered type is not caught by an unrelated registered handler.
(assert-equal 'propagated
  (condition-case outer
      (condition-case inner (signal 'my-error "x")
        (type-error 'wrong))
    (error 'propagated))
  "registered handler does not catch an unrelated unregistered type")

;;; First matching clause wins (CL/Elisp order semantics).
(assert-equal 'first
  (condition-case err (signal 'simple-error "x")
    (error 'first)
    (simple-error 'second))
  "first matching clause wins over a later exact match")

;;; Specific clause before the catch-all still wins when it matches.
(assert-equal 'specific
  (condition-case err (signal 'division-by-zero "x")
    (division-by-zero 'specific)
    (error 'catchall))
  "matching specific clause beats a later catch-all")

;;; Data payload survives signaling and is ordinary data once caught.
(define d nil)
(condition-case err (signal 'type-error '(42 integer))
  (type-error (set! d (error-data err))))
(assert-equal '(42 integer) d "error-data carries the payload")

;;; Re-signaling a caught error keeps its type and message.
(assert-equal "kept"
  (condition-case outer
      (condition-case inner (signal 'file-error "kept")
        (file-error (signal inner)))
    (error (error-message outer)))
  "re-signal preserves type and message")
