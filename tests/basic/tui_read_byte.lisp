(load "tests/test-helpers.lisp")

;; read-byte and read-byte-timeout are hard to test against a real stdin in a
;; test runner, so we only test their existence and error behavior here. The
;; actual blocking behavior is covered by the tui-events parser unit tests.

;; Functions exist and are callable
(assert-true (callable? read-byte) "read-byte should be callable")
(assert-true (callable? read-byte-timeout)
 "read-byte-timeout should be callable")

;; Timeout with no input returns nil quickly
(define byte (read-byte-timeout 10))

(assert-true (or (null? byte) (integer? byte))
 "read-byte-timeout should return nil or integer")

(when byte
  (assert-true (and (>= byte 0) (<= byte 255))
   "read-byte-timeout should return a byte"))

;; Invalid timeout argument
(assert-error (read-byte-timeout -1)
 "read-byte-timeout should reject negative timeout")
(assert-error (read-byte-timeout "foo")
 "read-byte-timeout should reject non-integer timeout")
