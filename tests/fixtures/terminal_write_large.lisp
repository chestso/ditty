;; One terminal-write of a million bytes. Used by
;; regression/terminal_output_pipe.sh with a reader that starts late, so the
;; write has to wait for the pipe to drain.
(terminal-write (make-string 1000000 #\y))
