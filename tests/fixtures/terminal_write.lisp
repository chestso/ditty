;; Mix princ and terminal-write. Used by regression/terminal_output_pipe.sh to
;; check that the bytes arrive complete and in order: a princ before a
;; terminal-write must not be overtaken, a long string and a string with
;; newlines must not be cut or reordered, and UTF-8 and escapes pass through.
(princ "a")

(terminal-write "b")

(princ "c")

(terminal-write (make-string 5000 #\x))
(terminal-write (concat "l1\nl2" "é" "\033[0m"))
(terminal-write "")

(princ "d")
