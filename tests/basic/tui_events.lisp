(load "tests/test-helpers.lisp")

(require 'tui-events)

(use-package 'tui-events)

(assert-true (provided? 'tui-events) "require provides tui-events feature")

;; Verify tui-events exports the public API (order is implementation-defined)
(define expected-exports
  '(tui-read-event tui-read-escape-sequence tui-key-event? tui-key-name
    tui-mouse-event? tui-enable-mouse tui-disable-mouse
    tui-focus-event? tui-enable-focus-events tui-disable-focus-events
    tui-enable-bracketed-paste tui-disable-bracketed-paste
    tui-paste-start? tui-paste-end?
    tui-set-clipboard tui-request-clipboard))

(define actual-exports (package-exports 'tui-events))

(assert-equal (length expected-exports) (length actual-exports)
 "tui-events exports correct number of symbols")

(assert-true
 (null?
  (filter (lambda (sym) (not (memq sym actual-exports))) expected-exports))
 "tui-events exports all expected public symbols")

(defun make-fake-reader (bytes)
  (let ((remaining bytes))
    (lambda ()
      (if (null? remaining)
        nil
        (let ((b (car remaining)))
          (set! remaining (cdr remaining))
          b)))))

;; Printable ASCII keys
(assert-equal "x" (tui-key-name (tui-read-event (make-fake-reader '(120))))
 "plain x key")
(assert-equal " " (tui-key-name (tui-read-event (make-fake-reader '(32))))
 "space key")

;; Control characters
(assert-equal "C-a" (tui-key-name (tui-read-event (make-fake-reader '(1))))
 "C-a key")
(assert-equal "C-d" (tui-key-name (tui-read-event (make-fake-reader '(4))))
 "C-d key")

;; Special keys via CSI
(assert-equal "up"
 (tui-key-name (tui-read-event (make-fake-reader '(27 91 65)))) "arrow up")
(assert-equal "down"
 (tui-key-name (tui-read-event (make-fake-reader '(27 91 66)))) "arrow down")
(assert-equal "right"
 (tui-key-name (tui-read-event (make-fake-reader '(27 91 67)))) "arrow right")
(assert-equal "left"
 (tui-key-name (tui-read-event (make-fake-reader '(27 91 68)))) "arrow left")
(assert-equal "home"
 (tui-key-name (tui-read-event (make-fake-reader '(27 91 72)))) "home")
(assert-equal "end"
 (tui-key-name (tui-read-event (make-fake-reader '(27 91 70)))) "end")
(assert-equal "delete"
 (tui-key-name (tui-read-event (make-fake-reader '(27 91 51 126)))) "delete")

;; Special keys via SS3
(assert-equal "up"
 (tui-key-name (tui-read-event (make-fake-reader '(27 79 65)))) "SS3 up")

;; Escape alone
(assert-equal "escape" (tui-key-name (tui-read-event (make-fake-reader '(27))))
 "escape key")

;; Focus events
(assert-equal 'in
 (cdr (assoc 'focus (tui-read-event (make-fake-reader '(27 91 73)))))
 "focus in")
(assert-equal 'out
 (cdr (assoc 'focus (tui-read-event (make-fake-reader '(27 91 79)))))
 "focus out")

(assert-true (tui-focus-event? (tui-read-event (make-fake-reader '(27 91 73))))
 "focus event predicate")

;; Bracketed paste markers
(assert-equal 'paste-start
 (cdr (assoc 'type (tui-read-event (make-fake-reader '(27 91 50 48 48 126)))))
 "paste start")
(assert-equal 'paste-end
 (cdr (assoc 'type (tui-read-event (make-fake-reader '(27 91 50 48 49 126)))))
 "paste end")

;; Mouse event (SGR)
(define mouse-ev
  (tui-read-event (make-fake-reader '(27 91 60 48 59 49 59 49 77))))

(assert-true (tui-mouse-event? mouse-ev) "mouse event")

(assert-equal 0 (cdr (assoc 'button mouse-ev)) "mouse button 0")
(assert-equal 1 (cdr (assoc 'x mouse-ev)) "mouse x 1")
(assert-equal 1 (cdr (assoc 'y mouse-ev)) "mouse y 1")

(assert-false (cdr (assoc 'release mouse-ev)) "mouse press")

(define mouse-release
  (tui-read-event (make-fake-reader '(27 91 60 48 59 50 59 51 109))))

(assert-true (cdr (assoc 'release mouse-release)) "mouse release")

(assert-equal -1 (cdr (assoc 'button mouse-release)) "release button -1")

;; Timeout / EOF
(assert-nil (tui-read-event (make-fake-reader '())) "empty reader returns nil")

;; Helpers for typed predicates
(assert-true (tui-key-event? (tui-read-event (make-fake-reader '(97))))
 "key event predicate")

(assert-false (tui-key-event? mouse-ev) "mouse is not key event")
