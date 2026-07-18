(load "tests/test-helpers.lisp")

;; These tests exercise the TUI primitives on a real terminal. They are safe to
;; run in CI because they do not block: input-available-p returns immediately.

;; Terminal size should be available when running under a tty.
(define size (terminal-size))

(assert-true
 (or (null? size)
     (and (pair? size) (integer? (car size)) (integer? (cdr size))))
 "terminal-size should return nil or a pair of integers")

(when size (assert-true (> (car size) 0) "terminal rows should be positive")
  (assert-true (> (cdr size) 0) "terminal columns should be positive"))

;; Input should not be available in a test runner without a real stdin.
(assert-true (boolean? (terminal-input-available-p))
 "terminal-input-available-p should return a boolean")

;; Raw mode toggling should return truthy and be idempotent.
(assert-true (set-terminal-raw) "set-terminal-raw should return truthy")
(assert-true (restore-terminal) "restore-terminal should return truthy")

;; Restoring an already-restored terminal is a no-op.
(assert-true (restore-terminal) "restore-terminal should be idempotent")

;; Resized flag is a boolean. It starts false and flips true on SIGWINCH.
(assert-true (boolean? (terminal-resized-p))
 "terminal-resized-p should return a boolean")

(assert-false (terminal-resized-p)
 "terminal-resized-p should be false initially")
(assert-false (terminal-resized-p)
 "terminal-resized-p should stay false without a resize")
