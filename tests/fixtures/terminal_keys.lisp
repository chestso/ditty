;; Print one "name|" per key event. Used by regression/terminal_utf8_pipe.sh
;; to exercise the real read-byte/terminal-wait path over a pipe.
(require 'terminal)

(terminal-event-loop
 (lambda (ev)
   (when (terminal:terminal-key-event? ev)
     (princ (terminal:terminal-key-name ev))
     (princ "|")))
 '((:probe)))
