(load "tests/test-helpers.lisp")

(require 'terminal)

(use-package 'terminal)

;; ---------------------------------------------------------------------------
;; CSI reply parsing
;; ---------------------------------------------------------------------------
(define da1-reply
  (terminal-parse-csi-reply
   '(91 63 54 52 59 49 59 52 59 57 99))) ; ESC [ ? 6 4 ; 1 ; 4 ; 9 c

(assert-equal '("?" (64 1 4 9) "c") da1-reply "DA1 reply is parsed")

(define size-reply (terminal-parse-csi-reply '(91 54 59 50 48 59 49 48 116))) ; ESC [ 6 ; 2 0 ; 1 0 t

(assert-equal '("" (6 20 10) "t") size-reply "cell size reply is parsed")
(assert-equal '(">" (1) "u")
 (terminal-parse-csi-reply '(91 62 49 117))
 "push-flags style reply keeps the > prefix")
(assert-equal '("?" (1) "u")
 (terminal-parse-csi-reply '(91 63 49 117))
 "keyboard flags reply keeps the ? prefix")
(assert-equal '("" (0 0) "t")
 (terminal-parse-csi-reply '(91 59 116))
 "empty parameter fields are zero")

;; Intermediate bytes (0x20-0x2F) follow the parameters and are reported
;; separately instead of corrupting the last parameter.
(assert-equal '("?" (2026 2) "y" "$")
 (terminal-parse-csi-reply '(91 63 50 48 50 54 59 50 36 121))
 "DECRQM reply keeps its status and reports the $ intermediate")
(assert-equal '("?" (2026 0) "y" "$")
 (terminal-parse-csi-reply '(91 63 50 48 50 54 59 48 36 121))
 "DECRQM status 0 is a real status")
(assert-equal '("" (1) "q" " ")
 (terminal-parse-csi-reply '(91 49 32 113))
 "a space intermediate is reported")
(assert-equal '("" () "p" "$")
 (terminal-parse-csi-reply '(91 36 112))
 "intermediates without parameters leave the parameter list empty")
(assert-equal 3 (length (terminal-parse-csi-reply '(91 63 49 117)))
 "a reply without intermediates stays a triple")

(assert-nil (terminal-parse-csi-reply '())
 "an empty byte list is not a CSI reply")
(assert-nil (terminal-parse-csi-reply '(79 65))
 "an SS3 sequence is not a CSI reply")
(assert-nil (terminal-parse-csi-reply '(91))
 "a bare CSI introducer is not a reply")

;; ---------------------------------------------------------------------------
;; Environment heuristics
;; ---------------------------------------------------------------------------
(defun fake-getenv (env)
  (lambda (name)
    (let ((pair (assoc name env)))
      (if (null? pair) nil (cdr pair)))))

(define portty-env
  (terminal-env-capabilities (fake-getenv '(("PORTTY" . "1")))))

(assert-true (cdr (assoc 'sixel portty-env)) "PORTTY=1 advertises sixel")

(assert-false (cdr (assoc 'kitty-graphics portty-env))
 "no KITTY_WINDOW_ID means no kitty graphics")
(assert-false (cdr (assoc 'iterm2-images portty-env))
 "no TERM_PROGRAM means no iTerm2 images")

(define term-env
  (terminal-env-capabilities (fake-getenv '(("TERM" . "xterm-sixel")))))

(assert-true (cdr (assoc 'sixel term-env))
 "TERM containing sixel advertises sixel")

(define kitty-env
  (terminal-env-capabilities (fake-getenv '(("KITTY_WINDOW_ID" . "1")))))

(assert-true (cdr (assoc 'kitty-graphics kitty-env))
 "KITTY_WINDOW_ID advertises kitty graphics")

(define iterm-env
  (terminal-env-capabilities (fake-getenv '(("TERM_PROGRAM" . "iTerm.app")))))

(assert-true (cdr (assoc 'iterm2-images iterm-env))
 "TERM_PROGRAM=iTerm.app advertises iTerm2 images")

(define empty-env (terminal-env-capabilities (fake-getenv '())))

(assert-false (cdr (assoc 'sixel empty-env))
 "an empty environment has no sixel")
(assert-false (cdr (assoc 'kitty-graphics empty-env))
 "an empty environment has no kitty graphics")
(assert-false (cdr (assoc 'iterm2-images empty-env))
 "an empty environment has no iTerm2 images")

;; ---------------------------------------------------------------------------
;; Capability folding
;; ---------------------------------------------------------------------------
(define no-env (fake-getenv '()))

;; sixel comes from DA1 parameter 4.
(define da1-caps
  (terminal-capabilities-from-replies (list '("?" (64 4 29) "c")) no-env))

(assert-true (cdr (assoc 'sixel da1-caps)) "DA1 parameter 4 means sixel")

(assert-equal "query" (cdr (assoc 'source da1-caps))
 "a DA1 reply makes source query")

;; kitty keyboard flags, including the "supported but no flags" zero.
(define flags0
  (terminal-capabilities-from-replies (list '("?" (0) "u")) no-env))

(assert-equal 0 (cdr (assoc 'kitty-keyboard flags0))
 "CSI ?0u reports keyboard flags 0")

(define flags1
  (terminal-capabilities-from-replies (list '("?" (1) "u")) no-env))

(assert-equal 1 (cdr (assoc 'kitty-keyboard flags1))
 "CSI ?1u reports keyboard flags 1")

(define flags-absent
  (terminal-capabilities-from-replies (list '("?" (64 4) "c")) no-env))

(assert-nil (cdr (assoc 'kitty-keyboard flags-absent))
 "no keyboard reply leaves kitty-keyboard nil")

;; The env answer for sixel is never downgraded by a DA1 without parameter 4.
(define env-sixel
  (terminal-capabilities-from-replies (list '("?" (64 29) "c"))
   (fake-getenv '(("PORTTY" . "1")))))

(assert-true (cdr (assoc 'sixel env-sixel))
 "an env sixel capability survives a DA1 without parameter 4")

(assert-equal "query" (cdr (assoc 'source env-sixel))
 "a DA1 reply makes source query even when sixel came from the env")

(define env-source
  (terminal-capabilities-from-replies nil (fake-getenv '(("PORTTY" . "1")))))

(assert-equal "env" (cdr (assoc 'source env-source))
 "without a DA1 reply the source is env")

(define none-source (terminal-capabilities-from-replies nil no-env))

(assert-equal "none" (cdr (assoc 'source none-source))
 "without replies or env capabilities the source is none")

;; Cell size comes from the CSI 6;H;W t reply.
(define cell-caps
  (terminal-capabilities-from-replies (list '("" (6 20 10) "t")) no-env))

(assert-equal (cons 10 20) (cdr (assoc 'cell-size cell-caps))
 "CSI 6;H;W t gives a (width . height) cell size")

;; Pixel size comes from CSI 4;H;W t unless the ioctl already knows it.
(define pixel-caps
  (terminal-capabilities-from-replies (list '("" (4 400 800) "t")) no-env))

(when (null? (terminal-pixel-size))
  (assert-equal (cons 800 400) (cdr (assoc 'pixel-size pixel-caps))
   "CSI 4;H;W t gives a (width . height) pixel size"))

;; Reply events work as inputs to the folder too.
(define event-caps
  (terminal-capabilities-from-replies
   (list
    (list (cons 'type 'reply) (cons 'prefix "?") (cons 'params '(64 4))
     (cons 'final "c")))
   no-env))

(assert-true (cdr (assoc 'sixel event-caps)) "reply events are accepted")

(assert-equal "query" (cdr (assoc 'source event-caps))
 "reply events count as a query")

;; Synchronized output (DECRQM mode 2026): statuses 1, 2 and 3 mean the
;; terminal implements it; 0 (unknown), 4 (permanently reset) and silence do
;; not.
(defun sync-caps (reply)
  (terminal-capabilities-from-replies (if (null? reply) nil (list reply))
   no-env))

(assert-true (cdr (assoc 'sync-output (sync-caps '("?" (2026 1) "y" "$"))))
 "DECRQM 2026 status 1 (set) means sync-output")
(assert-true (cdr (assoc 'sync-output (sync-caps '("?" (2026 2) "y" "$"))))
 "DECRQM 2026 status 2 (reset) means sync-output")
(assert-true (cdr (assoc 'sync-output (sync-caps '("?" (2026 3) "y" "$"))))
 "DECRQM 2026 status 3 (permanently set) means sync-output")

(assert-false (cdr (assoc 'sync-output (sync-caps '("?" (2026 0) "y" "$"))))
 "DECRQM 2026 status 0 (not recognised) is not sync-output")
(assert-false (cdr (assoc 'sync-output (sync-caps '("?" (2026 4) "y" "$"))))
 "DECRQM 2026 status 4 (permanently reset) is not sync-output")
(assert-false (cdr (assoc 'sync-output (sync-caps '("?" (2026) "y" "$"))))
 "a DECRQM reply without a status is not sync-output")
(assert-false (cdr (assoc 'sync-output (sync-caps '("?" (2027 1) "y" "$"))))
 "a DECRQM reply for another mode is not sync-output")
(assert-false (cdr (assoc 'sync-output (sync-caps nil)))
 "no reply means no sync-output")

;; Reply events (with an intermediates field) work as inputs too.
(assert-true
 (cdr
  (assoc 'sync-output
   (terminal-capabilities-from-replies
    (list
     (list (cons 'type 'reply) (cons 'prefix "?") (cons 'params '(2026 2))
      (cons 'final "y") (cons 'intermediates "$")))
    no-env)))
 "a DECRQM reply event is accepted")

;; The probe asks for it, ahead of the DA1 sentinel.
(assert-true (string-contains? terminal:*terminal-probe* "\033[?2026$p")
 "the probe sends the synchronized-output DECRQM")
(assert-true
 (< (string-index terminal:*terminal-probe* "\033[?2026$p")
  (string-index terminal:*terminal-probe* "\033[c"))
 "the DECRQM query comes before the DA1 sentinel")

;; `terminal-query` appends the DA1 sentinel itself, so what
;; `terminal-capabilities` hands it must not already end in one: a second DA1
;; makes the terminal send a second reply that nobody reads.
(assert-false
 (string-contains? terminal:*terminal-probe-queries* "\033[c")
 "the probe queries carry no DA1 sentinel")

(assert-equal (concat terminal:*terminal-probe-queries* "\033[c")
 terminal:*terminal-probe*
 "the event-loop probe is the queries plus one DA1 sentinel")
(assert-equal (- (length terminal:*terminal-probe*) 3)
 (string-index terminal:*terminal-probe* "\033[c")
 "the event-loop probe has its only DA1 sentinel at the end")

;; Every capability alist contains all eight keys.
(assert-equal 8 (length da1-caps) "the capabilities alist has eight keys")
(assert-equal 8 (length (sync-caps '("?" (2026 2) "y" "$")))
 "the capabilities alist has eight keys with a DECRQM reply")

;; ---------------------------------------------------------------------------
;; Blocking helpers
;; ---------------------------------------------------------------------------
(when (not (terminal-tty-p))
  (assert-nil (terminal-query "\033[?u")
   "terminal-query writes nothing and returns nil without a tty"))

(define off-caps (terminal-capabilities '((:query))))

(assert-equal 8 (length off-caps)
 "terminal-capabilities with :query nil returns eight keys")

(assert-true
 (null?
  (filter
   (lambda (key) (null? (assoc key off-caps)))
   '(sixel kitty-graphics iterm2-images kitty-keyboard pixel-size cell-size
     sync-output source)))
 "terminal-capabilities fills every documented key")
(assert-true
 (or (equal? (cdr (assoc 'source off-caps)) "env")
     (equal? (cdr (assoc 'source off-caps)) "none"))
 "a non-querying terminal-capabilities reports env or none")
