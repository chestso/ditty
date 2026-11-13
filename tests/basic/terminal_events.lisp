(load "tests/test-helpers.lisp")

(require 'terminal)

(use-package 'terminal)

(assert-true (provided? 'terminal) "require provides terminal feature")

;; Verify terminal exports the public API (order is implementation-defined)
(define expected-exports
  '(terminal-read-event terminal-read-escape-sequence terminal-key-event?
    terminal-key-name terminal-key-code terminal-key-text terminal-key-mods
    terminal-reply-event? terminal-parse-csi-reply terminal-mouse-event?
    terminal-focus-event? terminal-paste-start? terminal-paste-end?
    terminal-enable-mouse terminal-disable-mouse terminal-enable-focus-events
    terminal-disable-focus-events terminal-enable-bracketed-paste
    terminal-disable-bracketed-paste terminal-set-clipboard
    terminal-request-clipboard terminal-enter-alt-screen
    terminal-exit-alt-screen terminal-hide-cursor terminal-show-cursor
    terminal-enable-autowrap terminal-disable-autowrap
    terminal-push-keyboard-flags terminal-pop-keyboard-flags
    terminal-query-keyboard-flags terminal-call-with-raw terminal-event-loop
    terminal-make-decoder terminal-env-capabilities
    terminal-capabilities-from-replies terminal-capabilities terminal-query))

(define actual-exports (package-exports 'terminal))

(assert-equal 37 (length expected-exports)
 "expected terminal export list has 37 names")
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
 (terminal-key-code (terminal-read-event (make-fake-reader '(120))))
 "plain x key code")
(assert-equal "x"
 (terminal-key-text (terminal-read-event (make-fake-reader '(120))))
 "plain x key text")
(assert-equal 0
 (terminal-key-mods (terminal-read-event (make-fake-reader '(120))))
 "plain x key has no modifiers")
(assert-equal 'press
 (cdr (assoc 'event (terminal-read-event (make-fake-reader '(120)))))
 "plain x key press event")
(assert-equal " "
 (terminal-key-name (terminal-read-event (make-fake-reader '(32))))
 "space key")

;; UTF-8 input: one code point is one key event
(define aring (terminal-read-event (make-fake-reader '(195 165))))

(assert-equal "å" (terminal-key-name aring) "UTF-8 å key name")
(assert-equal 229 (terminal-key-code aring) "UTF-8 å key code")
(assert-equal "å" (terminal-key-text aring) "UTF-8 å key text")
(assert-equal 0 (terminal-key-mods aring) "UTF-8 å has no modifiers")
(assert-equal 1 (length (terminal-key-name aring)) "å name is one character")

(define emoji (terminal-read-event (make-fake-reader '(240 159 152 128))))

(assert-equal "😀" (terminal-key-name emoji) "four-byte emoji key name")
(assert-equal 128512 (terminal-key-code emoji) "four-byte emoji key code")
(assert-equal "😀" (terminal-key-text emoji) "four-byte emoji key text")
(assert-equal 1 (length (terminal-key-name emoji))
 "emoji name is one character")

;; Malformed input becomes U+FFFD, which is visible text.
(define replacement (terminal-read-event (make-fake-reader '(255))))

(assert-equal (char->string (code-char #xFFFD)) (terminal-key-name replacement)
 "a stray byte becomes U+FFFD")
(assert-equal #xFFFD (terminal-key-code replacement) "replacement key code")
(assert-equal (char->string (code-char #xFFFD)) (terminal-key-text replacement)
 "U+FFFD is typed text")

;; C1 controls are not text.
(define c1 (terminal-read-event (make-fake-reader '(194 133))))

(assert-equal "c1-133" (terminal-key-name c1) "C2 85 is a C1 control")
(assert-equal 133 (terminal-key-code c1) "C1 control code")

(assert-nil (terminal-key-text c1) "C1 control is not text")

;; Control characters
(define ctrl-a (terminal-read-event (make-fake-reader '(1))))

(assert-equal "C-a" (terminal-key-name ctrl-a) "C-a key")
(assert-equal 1 (terminal-key-code ctrl-a) "C-a key code")

(assert-nil (terminal-key-text ctrl-a) "C-a is not text")

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

(assert-nil
 (terminal-key-code (terminal-read-event (make-fake-reader '(27 91 65))))
 "a named key has no code")
(assert-nil
 (terminal-key-text (terminal-read-event (make-fake-reader '(27 91 65))))
 "a named key is not text")

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

;; A multi-byte character after ESC is one unit, not a stray text event.
(define esc-reader (make-fake-reader '(27 195 165)))

(define esc-aring (terminal-read-event esc-reader))

(assert-equal "esc-229" (terminal-key-name esc-aring)
 "ESC + UTF-8 å is one escape-sequence unit")

(assert-nil (terminal-read-event esc-reader)
 "ESC + UTF-8 å leaves no stray text event")

;; OSC payloads decode to code points (title/clipboard replies).
(define osc-reader (make-fake-reader '(93 53 50 59 99 59 195 165 7)))

(define osc-dec (terminal-make-decoder))

(define osc-seq
  (terminal-read-escape-sequence
   (lambda () (terminal:terminal-decoder-next osc-dec osc-reader))))

(assert-equal '(93 53 50 59 99 59 229 7) osc-seq
 "an OSC payload decodes its UTF-8 bytes")

;; Kitty keyboard protocol: CSI 27 u is Escape with flag 1 enabled.
(assert-equal "escape"
 (terminal-key-name (terminal-read-event (make-fake-reader '(27 91 50 55 117))))
 "kitty CSI 27 u is escape")
(assert-equal 0
 (terminal-key-mods (terminal-read-event (make-fake-reader '(27 91 50 55 117))))
 "kitty escape has no modifiers")

;; Kitty modified keys get a prefix so they never look like typed text.
(define kitty-ctrl-a
  (terminal-read-event (make-fake-reader '(27 91 57 55 59 53 117))))

(assert-equal "C-a" (terminal-key-name kitty-ctrl-a) "kitty CSI 97;5u is C-a")
(assert-equal 4 (terminal-key-mods kitty-ctrl-a) "kitty CSI 97;5u has ctrl")
(assert-equal 97 (terminal-key-code kitty-ctrl-a)
 "kitty CSI 97;5u carries its code")

(assert-nil (terminal-key-text kitty-ctrl-a)
 "kitty CSI 97;5u is not typed text")

;; Kitty unmodified character keys carry a code but never associated text.
(define kitty-aring
  (terminal-read-event (make-fake-reader '(27 91 50 50 56 117))))

(assert-equal "ä" (terminal-key-name kitty-aring) "kitty CSI 228 u is ä")
(assert-equal 228 (terminal-key-code kitty-aring) "kitty CSI 228 u code")

(assert-nil (terminal-key-text kitty-aring)
 "kitty CSI 228 u has no text")

;; Surrogates and out-of-range code points are not scalar values.
(assert-equal "key-55296"
 (terminal-key-name
  (terminal-read-event (make-fake-reader '(27 91 53 53 50 57 54 117))))
 "kitty CSI 55296 u is named, not minted")
(assert-equal "key-1114112"
 (terminal-key-name
  (terminal-read-event (make-fake-reader '(27 91 49 49 49 52 49 49 50 117))))
 "kitty CSI 1114112 u is named, not minted")

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

(assert-false (assoc 'intermediates da1)
 "a reply without intermediate bytes has no intermediates field")

;; DECRQM reply ESC [ ? 2026 ; 2 $ y: the $ is an intermediate byte, not part
;; of the second parameter.
(define decrqm-reply
  (terminal-read-event
   (make-fake-reader '(27 91 63 50 48 50 54 59 50 36 121))))

(assert-true (terminal-reply-event? decrqm-reply) "DECRQM reply is a reply")

(assert-equal "?" (cdr (assoc 'prefix decrqm-reply)) "DECRQM prefix")
(assert-equal '(2026 2) (cdr (assoc 'params decrqm-reply))
 "DECRQM params keep the mode status")
(assert-equal "y" (cdr (assoc 'final decrqm-reply)) "DECRQM final")
(assert-equal "$" (cdr (assoc 'intermediates decrqm-reply))
 "DECRQM intermediates")

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

;; Accessors on non-key events
(assert-equal 0 (terminal-key-mods mouse-ev) "mouse has no key modifiers")

(assert-nil (terminal-key-code mouse-ev) "mouse has no key code")
(assert-nil (terminal-key-text mouse-ev) "mouse has no key text")
