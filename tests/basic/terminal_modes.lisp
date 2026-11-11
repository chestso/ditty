(load "tests/test-helpers.lisp")

(require 'terminal)

(use-package 'terminal)

;; The mode helpers return the string they print (`princ` returns its
;; argument), so the exact escape sequences can be checked without capturing
;; output.  Each helper is immediately undone by its counterpart so that a
;; manual run in a terminal ends in the state it started in.
(assert-equal "\033[?1049h" (terminal-enter-alt-screen)
 "enter alt screen writes ESC[?1049h")
(assert-equal "\033[?1049l" (terminal-exit-alt-screen)
 "exit alt screen writes ESC[?1049l")
(assert-equal "\033[?25l" (terminal-hide-cursor)
 "hide cursor writes ESC[?25l")
(assert-equal "\033[?25h" (terminal-show-cursor)
 "show cursor writes ESC[?25h")
(assert-equal "\033[?1003h\033[?1006h" (terminal-enable-mouse)
 "enable mouse writes button + SGR tracking")
(assert-equal "\033[?1003l\033[?1006l" (terminal-disable-mouse)
 "disable mouse writes button + SGR tracking off")
(assert-equal "\033[?1004h" (terminal-enable-focus-events)
 "enable focus events writes ESC[?1004h")
(assert-equal "\033[?1004l" (terminal-disable-focus-events)
 "disable focus events writes ESC[?1004l")
(assert-equal "\033[?2004h" (terminal-enable-bracketed-paste)
 "enable bracketed paste writes ESC[?2004h")
(assert-equal "\033[?2004l" (terminal-disable-bracketed-paste)
 "disable bracketed paste writes ESC[?2004l")
(assert-equal "\033[>1u" (terminal-push-keyboard-flags 1)
 "push keyboard flags writes ESC[>1u")
(assert-equal "\033[<u" (terminal-pop-keyboard-flags)
 "pop keyboard flags writes ESC[<u")
(assert-equal "\033[?u" (terminal-query-keyboard-flags)
 "query keyboard flags writes ESC[?u")

;; Clipboard helpers write several pieces; only their callability is checked.
(assert-true (callable? terminal-set-clipboard)
 "terminal-set-clipboard is callable")
(assert-true (callable? terminal-request-clipboard)
 "terminal-request-clipboard is callable")

;; A session returns the thunk's value.
(assert-equal 42 (terminal-call-with-raw (lambda () 42))
 "terminal-call-with-raw returns the thunk value")
(assert-equal 0 terminal:*terminal-raw-depth*
 "depth is zero before any nested session")

;; A session is re-entrant: the inner call must not end the outer one.
(define nested
  (terminal-call-with-raw
   (lambda ()
     (let ((inner (terminal-call-with-raw (lambda () 7))))
       (list inner terminal:*terminal-raw-depth*)))))

(assert-equal '(7 1) nested
 "nested sessions return the inner value and keep the outer depth")
(assert-equal 0 terminal:*terminal-raw-depth*
 "depth is zero after nested sessions")

;; An error inside the thunk propagates, but cleanup still runs.
(define failure
  (condition-case err
    (terminal-call-with-raw
     (lambda () (error "boom"))
     '((:mouse . #t)))
    (error err)))

(assert-true (error? failure) "an error inside the thunk is propagated")

(assert-equal 0 terminal:*terminal-raw-depth*
 "depth is restored after an error")

;; ... and a following session still works.
(assert-equal 'ok (terminal-call-with-raw (lambda () 'ok))
 "a session after an error still runs")

;; Session modes are requested from the opts alist and do not leak.
(assert-equal 5
 (terminal-call-with-raw
  (lambda ()
    (terminal-call-with-raw (lambda () 5) '((:paste . #t) (:focus . #t))))
  '((:hide-cursor . #t)))
 "sessions with mode options return the thunk value")
(assert-equal 0 terminal:*terminal-raw-depth*
 "depth is zero after sessions with mode options")
