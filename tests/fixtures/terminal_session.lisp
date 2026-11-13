;; Run terminal sessions over a pipe and print a marker inside each. Used by
;; regression/terminal_output_pipe.sh to check the order of the mode
;; sequences, including :no-wrap. Over a pipe the terminal is never put in
;; raw mode, so only the escape sequences and the markers are written.
(require 'terminal)

(terminal:terminal-call-with-raw (lambda () (princ "A"))
 '((:alt-screen . #t) (:hide-cursor . #t) (:no-wrap . #t)))
(terminal:terminal-call-with-raw (lambda () (princ "B")) '((:no-wrap . #t)))
(terminal:terminal-call-with-raw (lambda () (princ "C")))

(condition-case err
  (terminal:terminal-call-with-raw (lambda () (princ "D") (error "boom"))
   '((:hide-cursor . #t) (:no-wrap . #t)))
  (error (princ "E")))
