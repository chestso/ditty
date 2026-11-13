(load "tests/test-helpers.lisp")

;; terminal-write writes straight to the file descriptor. The bytes it emits
;; are checked end to end by regression/terminal_output_pipe.sh; this file
;; covers the return value and the argument checks. The empty string writes
;; nothing, so the test output stays clean.
(assert-equal "" (terminal-write "")
 "terminal-write returns the string it was given")

(assert-error (terminal-write) "terminal-write requires an argument")
(assert-error (terminal-write 42) "terminal-write rejects a number")
(assert-error (terminal-write 'text) "terminal-write rejects a symbol")
(assert-error (terminal-write nil) "terminal-write rejects nil")
(assert-error (terminal-write "" "") "terminal-write rejects extra arguments")
