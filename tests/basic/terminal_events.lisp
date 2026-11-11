(load "tests/test-helpers.lisp")

(require 'terminal)

(use-package 'terminal)

(assert-true (provided? 'terminal) "require provides terminal feature")

;; Verify terminal exports the public API (order is implementation-defined)
(define expected-exports
  '(terminal-read-event terminal-read-escape-sequence terminal-key-event?
    terminal-key-name terminal-key-byte terminal-key-mods terminal-reply-event?
    terminal-parse-csi-reply terminal-mouse-event? terminal-focus-event?
    terminal-paste-start? terminal-paste-end? terminal-enable-mouse
    terminal-disable-mouse terminal-enable-focus-events
    terminal-disable-focus-events terminal-enable-bracketed-paste
    terminal-disable-bracketed-paste terminal-set-clipboard
    terminal-request-clipboard terminal-enter-alt-screen
    terminal-exit-alt-screen terminal-hide-cursor terminal-show-cursor
    terminal-push-keyboard-flags terminal-pop-keyboard-flags
    terminal-query-keyboard-flags terminal-call-with-raw terminal-event-loop
    terminal-env-capabilities terminal-capabilities-from-replies
    terminal-capabilities terminal-query))

(define actual-exports (package-exports 'terminal))

(assert-equal 33 (length expected-exports)
 "expected terminal export list has 33 names")
(assert-equal (length expected-exports) (length actual-exports)
 "terminal exports correct number of symbols")

(assert-true
 (null?
  (filter (lambda (sym) (not (memq sym actual-exports))) expected-exports))
 "terminal exports all expected public symbols")

(defun make-fake-reader (bytes)
  (let ((remaining bytes))
    (lambda ()
      (if (null? remaining)
        nil
        (let ((b (car remaining)))
          (set! remaining (cdr remaining))
          b)))))

;; Printable ASCII keys
(assert-equal "x"
 (terminal-key-name (terminal-read-event (make-fake-reader '(120))))
 "plain x key")
(assert-equal 120
 (terminal-key-byte (terminal-read-event (make-fake-reader '(120))))
 "plain x key byte")
(assert-equal 0
 (terminal-key-mods (terminal-read-event (make-fake-reader '(120))))
 "plain x key has no modifiers")
(assert-equal 'press
 (cdr (assoc 'event (terminal-read-event (make-fake-reader '(120)))))
 "plain x key press event")
(assert-equal " "
 (terminal-key-name (terminal-read-event (make-fake-reader '(32))))
 "space key")

;; Control characters
(assert-equal "C-a"
 (terminal-key-name (terminal-read-event (make-fake-reader '(1))))
 "C-a key")
(assert-equal "C-d"
 (terminal-key-name (terminal-read-event (make-fake-reader '(4))))
 "C-d key")

;; Special keys via CSI
(assert-equal "up"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 91 65))))
 "arrow up")
(assert-equal "down"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 91 66))))
 "arrow down")
(assert-equal "right"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 91 67))))
 "arrow right")
(assert-equal "left"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 91 68))))
 "arrow left")
(assert-equal "home"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 91 72))))
 "home")
(assert-equal "end"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 91 70)))) "end")
(assert-equal "delete"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 91 51 126))))
 "delete")

;; Special keys via SS3
(assert-equal "up"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 79 65))))
 "SS3 up")

;; Escape alone, no timer: the reader simply has nothing more to give.
(assert-equal "escape"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27))))
 "escape key")
(assert-equal 0
 (terminal-key-mods (terminal-read-event (make-fake-reader '(27))))
 "lone escape has no modifiers")

;; Kitty keyboard protocol: CSI 27 u is Escape with flag 1 enabled.
(assert-equal "escape"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 91 50 55 117))))
 "kitty CSI 27 u is escape")
(assert-equal 0
 (terminal-key-mods (terminal-read-event (make-fake-reader '(27 91 50 55 117))))
 "kitty escape has no modifiers")

;; Kitty modified keys get a prefix so they never look like typed text.
(define ctrl-a
  (terminal-read-event (make-fake-reader '(27 91 57 55 59 53 117))))

(assert-equal "C-a" (terminal-key-name ctrl-a) "kitty CSI 97;5u is C-a")
(assert-equal 4 (terminal-key-mods ctrl-a) "kitty CSI 97;5u has ctrl")
(assert-equal 97 (terminal-key-byte ctrl-a) "kitty CSI 97;5u carries its byte")

(define alt-x
  (terminal-read-event (make-fake-reader '(27 91 49 50 48 59 51 117))))

(assert-equal "M-x" (terminal-key-name alt-x) "kitty CSI 120;3u is M-x")
(assert-equal 2 (terminal-key-mods alt-x) "kitty CSI 120;3u has alt")

(define ctrl-alt-a
  (terminal-read-event (make-fake-reader '(27 91 57 55 59 55 117))))

(assert-equal "C-M-a" (terminal-key-name ctrl-alt-a)
 "kitty CSI 97;7u is C-M-a")

;; Event type suffix: CSI 97;5:3u is a key release.
(define released
  (terminal-read-event (make-fake-reader '(27 91 57 55 59 53 58 51 117))))

(assert-equal 'release (cdr (assoc 'event released)) "kitty key release event")
(assert-equal "C-a" (terminal-key-name released)
 "kitty key release keeps the name")

;; Legacy functional keys with modifiers.
(define shift-up (terminal-read-event (make-fake-reader '(27 91 49 59 53 65))))

(assert-equal "up" (terminal-key-name shift-up) "CSI 1;5A is up")
(assert-equal 4 (terminal-key-mods shift-up) "CSI 1;5A has ctrl")

(define shift-delete
  (terminal-read-event (make-fake-reader '(27 91 51 59 50 126))))

(assert-equal "delete" (terminal-key-name shift-delete) "CSI 3;2~ is delete")
(assert-equal 1 (terminal-key-mods shift-delete) "CSI 3;2~ has shift")

;; Replies are not keys.
(define da1
  (terminal-read-event (make-fake-reader '(27 91 63 54 52 59 49 59 52 99))))

(assert-true (terminal-reply-event? da1) "CSI ?64;1;4c is a reply")

(assert-false (terminal-key-event? da1) "reply is not a key event")

(assert-equal "?" (cdr (assoc 'prefix da1)) "DA1 prefix")
(assert-equal '(64 1 4) (cdr (assoc 'params da1)) "DA1 params")
(assert-equal "c" (cdr (assoc 'final da1)) "DA1 final")

(define keyboard-reply
  (terminal-read-event (make-fake-reader '(27 91 63 49 117))))

(assert-true (terminal-reply-event? keyboard-reply) "CSI ?1u is a reply")

(assert-equal "u" (cdr (assoc 'final keyboard-reply)) "keyboard reply final")
(assert-equal '(1) (cdr (assoc 'params keyboard-reply)) "keyboard reply params")

(define size-reply
  (terminal-read-event (make-fake-reader '(27 91 54 59 50 48 59 49 48 116))))

(assert-true (terminal-reply-event? size-reply) "CSI 6;20;10t is a reply")

(assert-equal "" (cdr (assoc 'prefix size-reply)) "size reply prefix")
(assert-equal '(6 20 10) (cdr (assoc 'params size-reply)) "size reply params")
(assert-equal "t" (cdr (assoc 'final size-reply)) "size reply final")

;; Focus events
(assert-equal 'in
 (cdr (assoc 'focus (terminal-read-event (make-fake-reader '(27 91 73)))))
 "focus in")
(assert-equal 'out
 (cdr (assoc 'focus (terminal-read-event (make-fake-reader '(27 91 79)))))
 "focus out")

(assert-true
 (terminal-focus-event? (terminal-read-event (make-fake-reader '(27 91 73))))
 "focus event predicate")

;; Bracketed paste markers
(assert-equal 'paste-start
 (cdr
  (assoc 'type (terminal-read-event (make-fake-reader '(27 91 50 48 48 126)))))
 "paste start")
(assert-equal 'paste-end
 (cdr
  (assoc 'type (terminal-read-event (make-fake-reader '(27 91 50 48 49 126)))))
 "paste end")

;; Mouse event (SGR)
(define mouse-ev
  (terminal-read-event (make-fake-reader '(27 91 60 48 59 49 59 49 77))))

(assert-true (terminal-mouse-event? mouse-ev) "mouse event")

(assert-equal 0 (cdr (assoc 'button mouse-ev)) "mouse button 0")
(assert-equal 1 (cdr (assoc 'x mouse-ev)) "mouse x 1")
(assert-equal 1 (cdr (assoc 'y mouse-ev)) "mouse y 1")

(assert-false (cdr (assoc 'release mouse-ev)) "mouse press")

(define mouse-release
  (terminal-read-event (make-fake-reader '(27 91 60 48 59 50 59 51 109))))

(assert-true (cdr (assoc 'release mouse-release)) "mouse release")

(assert-equal -1 (cdr (assoc 'button mouse-release)) "release button -1")

;; Timeout / EOF
(assert-nil (terminal-read-event (make-fake-reader '()))
 "empty reader returns nil")

;; Helpers for typed predicates
(assert-true
 (terminal-key-event? (terminal-read-event (make-fake-reader '(97))))
 "key event predicate")

(assert-false (terminal-key-event? mouse-ev) "mouse is not key event")
(assert-false (terminal-reply-event? mouse-ev) "mouse is not a reply")

;; Modifier accessors on non-key events
(assert-equal 0 (terminal-key-mods mouse-ev) "mouse has no key modifiers")

(assert-nil (terminal-key-byte mouse-ev) "mouse has no key byte")
