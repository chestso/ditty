(load "tests/test-helpers.lisp")

(require 'terminal)

(use-package 'terminal)

;; The loop takes seams (:wait-fn, :read-byte-fn, :available-fn, :tty-p) so
;; these tests never touch the real terminal.
(defun fake-wait (responses)
  "Return a :wait-fn that yields RESPONSES in order, then timeout forever."
  (let ((rest responses))
    (lambda (timeout)
      (if (null? rest)
        'timeout
        (let ((response (car rest)))
          (set! rest (cdr rest))
          response)))))

(defun fake-input (bytes)
  "Return (READ-BYTE-FN AVAILABLE-FN) over BYTES."
  (let ((remaining bytes))
    (list
     (lambda ()
       (if (null? remaining)
         nil
         (let ((b (car remaining)))
           (set! remaining (cdr remaining))
           b)))
     (lambda () (not (null? remaining))))))

(defun loop-opts (wait read available)
  (list (cons :probe nil) (cons :tty-p nil) (cons :wait-fn wait)
   (cons :read-byte-fn read) (cons :available-fn available)))

;; ---------------------------------------------------------------------------
;; Ticks: a timeout with a due tick delivers one tick event.
;; ---------------------------------------------------------------------------
(define tick-count 0)

(define tick-result
  (terminal-event-loop
   (lambda (ev)
     (when (equal? (cdr (assoc 'type ev)) 'tick)
       (set! tick-count (+ tick-count 1))
       (when (>= tick-count 3) 'stop)))
   (list (cons :tick-ms 0) (cons :probe nil) (cons :tty-p nil)
    (cons :wait-fn (fake-wait '(timeout timeout timeout timeout)))
    (cons :read-byte-fn (lambda () nil))
    (cons :available-fn (lambda () nil)))))

(assert-nil tick-result "a stopped loop returns nil")

(assert-equal 3 tick-count "the handler saw exactly three ticks")

;; ---------------------------------------------------------------------------
;; Input burst: bytes, then a resize.
;; ---------------------------------------------------------------------------
(define seen nil)

(define burst-input (fake-input '(120 27 91 65)))

(define burst-result
  (terminal-event-loop
   (lambda (ev) (set! seen (cons ev seen))
     (when (>= (length seen) 3) 'stop))
   (list (cons :probe nil) (cons :tty-p nil)
    (cons :wait-fn (fake-wait '(input resize)))
    (cons :read-byte-fn (car burst-input))
    (cons :available-fn (cadr burst-input)))))

(assert-nil burst-result "the burst loop stops cleanly")

(assert-equal 3 (length seen) "x, up, and one resize arrived")
(assert-equal "x" (terminal-key-name (caddr seen)) "first event is x")
(assert-equal "up" (terminal-key-name (cadr seen)) "second event is up")
(assert-equal 'resize (cdr (assoc 'type (car seen))) "third event is a resize")

(assert-true (integer? (cdr (assoc 'rows (car seen)))) "resize carries rows")
(assert-true (integer? (cdr (assoc 'cols (car seen)))) "resize carries cols")

;; ---------------------------------------------------------------------------
;; A lone ESC arrives as escape with no byte after it.
;; ---------------------------------------------------------------------------
(define escape-seen nil)

(define lone-input (fake-input '(27)))

(terminal-event-loop
 (lambda (ev) (set! escape-seen (cons ev escape-seen))
   'stop)
 (list (cons :probe nil) (cons :tty-p nil)
  (cons :wait-fn (fake-wait '(input)))
  (cons :read-byte-fn (car lone-input))
  (cons :available-fn (cadr lone-input))))

(assert-equal 1 (length escape-seen) "a lone escape is one event")
(assert-equal "escape" (terminal-key-name (car escape-seen))
 "a lone escape is the escape key")
(assert-equal 0 (terminal-key-mods (car escape-seen))
 "a lone escape has no modifiers")

;; ---------------------------------------------------------------------------
;; EOF: wait says input but no byte can be read.
;; ---------------------------------------------------------------------------
(define eof-calls 0)

(define eof-result
  (terminal-event-loop
   (lambda (ev) (set! eof-calls (+ eof-calls 1))
     nil)
   (list (cons :probe nil) (cons :tty-p nil)
    (cons :wait-fn (fake-wait '(input)))
    (cons :read-byte-fn (lambda () nil))
    (cons :available-fn (lambda () nil)))))

(assert-nil eof-result "EOF ends the loop with nil")

(assert-equal 0 eof-calls "EOF delivers no event")

;; ---------------------------------------------------------------------------
;; A handler error is caught by the loop's condition-case.
;; ---------------------------------------------------------------------------
(define error-result
  (terminal-event-loop (lambda (ev) (error "handler exploded"))
   (list (cons :tick-ms 0) (cons :probe nil) (cons :tty-p nil)
    (cons :wait-fn (fake-wait '(timeout)))
    (cons :read-byte-fn (lambda () nil))
    (cons :available-fn (lambda () nil)))))

(assert-true (error? error-result)
 "a handler error is caught by the loop's condition-case and returned as its value")

;; ... and the session depth is restored.
(assert-equal 0 terminal:*terminal-raw-depth*
 "raw depth is restored after a handler error")

;; ---------------------------------------------------------------------------
;; :on-start runs once, before any event.
;; ---------------------------------------------------------------------------
(define order nil)

(terminal-event-loop
 (lambda (ev) (set! order (cons 'event order))
   'stop)
 (list (cons :tick-ms 0) (cons :probe nil) (cons :tty-p nil)
  (cons :on-start (lambda () (set! order (cons 'start order))))
  (cons :wait-fn (fake-wait '(timeout)))
  (cons :read-byte-fn (lambda () nil))
  (cons :available-fn (lambda () nil))))

(assert-equal '(event start) order ":on-start runs once before the first event")

;; ---------------------------------------------------------------------------
;; :probe with :tty-p nil: one capabilities event, from the environment.
;; ---------------------------------------------------------------------------
(define capability-events nil)

(terminal-event-loop
 (lambda (ev) (set! capability-events (cons ev capability-events))
   'stop)
 (list (cons :probe #t) (cons :tty-p nil)
  (cons :wait-fn (fake-wait '(timeout)))
  (cons :read-byte-fn (lambda () nil))
  (cons :available-fn (lambda () nil))))

(assert-equal 1 (length capability-events) "exactly one capabilities event")
(assert-equal 'capabilities
 (cdr (assoc 'type (car capability-events)))
 "the first event is capabilities")

(define env-caps (cdr (assoc 'caps (car capability-events))))

(assert-equal 8 (length env-caps) "the capabilities alist has eight keys")

(assert-true
 (null?
  (filter
   (lambda (key) (null? (assoc key env-caps)))
   '(sixel kitty-graphics iterm2-images kitty-keyboard pixel-size cell-size
     sync-output source)))
 "the capabilities alist carries every documented key")
(assert-true
 (or (equal? (cdr (assoc 'source env-caps)) "env")
     (equal? (cdr (assoc 'source env-caps)) "none"))
 "source is env or none without a query")

;; ---------------------------------------------------------------------------
;; :probe with replies: DA1 ends the probe, kitty keyboard flags are pushed.
;; ---------------------------------------------------------------------------
(define probe-seen nil)

(define probe-input (fake-input '(27 91 63 49 117 27 91 63 54 52 59 52 99)))

(terminal-event-loop
 (lambda (ev) (set! probe-seen (cons ev probe-seen))
   (when (equal? (cdr (assoc 'type ev)) 'capabilities) 'stop))
 (list (cons :probe #t) (cons :keyboard #t) (cons :tty-p #t)
  (cons :wait-fn (fake-wait '(input)))
  (cons :read-byte-fn (car probe-input))
  (cons :available-fn (cadr probe-input))))

(assert-equal 1 (length probe-seen) "the probe delivers one capabilities event")

(define query-caps (cdr (assoc 'caps (car probe-seen))))

(assert-equal "query" (cdr (assoc 'source query-caps))
 "a DA1 reply makes the source query")
(assert-equal 1 (cdr (assoc 'kitty-keyboard query-caps))
 "CSI ?1u reports keyboard flag 1")

(assert-true (cdr (assoc 'sixel query-caps))
 "DA1 parameter 4 advertises sixel")

;; ---------------------------------------------------------------------------
;; :probe with a DECRQM reply: sync-output reaches the capabilities event.
;; ---------------------------------------------------------------------------
(defun probe-caps (bytes)
  "Run the probe loop over BYTES and return the capabilities alist."
  (let ((input (fake-input bytes))
        (found nil))
    (terminal-event-loop
     (lambda (ev)
       (when (equal? (cdr (assoc 'type ev)) 'capabilities)
         (set! found (cdr (assoc 'caps ev)))
         'stop))
     (list (cons :probe #t) (cons :keyboard #f) (cons :tty-p #t)
      (cons :wait-fn (fake-wait '(input)))
      (cons :read-byte-fn (car input))
      (cons :available-fn (cadr input))))
    found))

;; ESC [ ? 2026 ; 2 $ y, then DA1 ESC [ ? 6 4 ; 4 c
(define sync-caps
  (probe-caps
   '(27 91 63 50 48 50 54 59 50 36 121 27 91 63 54 52 59 52 99)))

(assert-true (cdr (assoc 'sync-output sync-caps))
 "a DECRQM 2026 reply (status 2) reaches the capabilities as sync-output")
(assert-true (cdr (assoc 'sixel sync-caps))
 "the DA1 reply after a DECRQM reply still ends the probe")

;; ESC [ ? 2026 ; 0 $ y: the terminal does not know the mode.
(define unsync-caps
  (probe-caps
   '(27 91 63 50 48 50 54 59 48 36 121 27 91 63 54 52 59 52 99)))

(assert-false (cdr (assoc 'sync-output unsync-caps))
 "a DECRQM 2026 reply with status 0 is not sync-output")

;; A terminal that ignores DECRQM answers only DA1.
(define silent-caps (probe-caps '(27 91 63 54 52 59 52 99)))

(assert-false (cdr (assoc 'sync-output silent-caps))
 "no DECRQM reply means no sync-output")

;; ---------------------------------------------------------------------------
;; :on-idle runs each time the loop is about to block.
;; ---------------------------------------------------------------------------
;; The harness keeps its state in top-level variables so each helper stays
;; small. One log records idle calls, waits and handled events in order.
(define idle-log nil)

(define idle-current nil)

(define idle-pending nil)

(define idle-responses nil)

(define idle-stop-on nil)

(defun idle-note (entry) (set! idle-log (cons entry idle-log)))

(defun idle-load-chunk ()
  "Make the next chunk of bytes readable, as an input wake-up does."
  (when (not (null? idle-pending)) (set! idle-current (car idle-pending))
    (set! idle-pending (cdr idle-pending))))

(defun idle-wait (timeout)
  "A :wait-fn that replays the responses, then reports EOF-style input."
  (idle-note 'wait)
  (if (null? idle-responses)
    'input
    (let ((response (car idle-responses)))
      (set! idle-responses (cdr idle-responses))
      (when (equal? response 'input) (idle-load-chunk))
      response)))

(defun idle-read-byte ()
  (if (null? idle-current)
    nil
    (let ((b (car idle-current)))
      (set! idle-current (cdr idle-current))
      b)))

(defun idle-handler (ev)
  "Log a key's name (or another event's type); stop on IDLE-STOP-ON."
  (let ((name
         (if (terminal-key-event? ev)
           (terminal-key-name ev)
           (cdr (assoc 'type ev)))))
    (idle-note name)
    (when (equal? name idle-stop-on) 'stop)))

(defun idle-opts ()
  (list (cons :probe nil) (cons :tty-p nil)
   (cons :on-idle (lambda () (idle-note 'idle)))
   (cons :wait-fn idle-wait)
   (cons :read-byte-fn idle-read-byte)
   (cons :available-fn (lambda () (not (null? idle-current))))))

(defun run-idle-loop (chunks responses stop-on)
  "Run the loop over CHUNKS (one per input wake-up in RESPONSES) and return
   (LOG . RESULT), LOG in order."
  (set! idle-log nil)
  (set! idle-current nil)
  (set! idle-pending chunks)
  (set! idle-responses responses)
  (set! idle-stop-on stop-on)
  (let ((result (terminal-event-loop idle-handler (idle-opts))))
    (cons (reverse idle-log) result)))

;; A burst is drained before idle: x y z arrive in one wake-up, so idle runs
;; once for them, not three times; and it runs before the first wait.
(define burst-run (run-idle-loop '((120 121 122) (119)) '(input input) "w"))

(assert-nil (cdr burst-run) "the idle loop stops cleanly")

(assert-equal '(idle wait "x" "y" "z" idle wait "w") (car burst-run)
 "on-idle runs before each wait, once per burst, and not after stop")

;; Nothing to deliver: idle runs once before the first wait, and not again
;; once EOF ends the loop.
(define eof-run (run-idle-loop '() '(input) "never"))

(assert-nil (cdr eof-run) "an idle loop ends at EOF")

(assert-equal '(idle wait) (car eof-run)
 "on-idle runs before the first wait and not after EOF")

;; Resize and timeout wake-ups are wake-ups too.
(define resize-run (run-idle-loop '() '(resize timeout resize) "never"))

(assert-equal '(idle wait resize idle wait idle wait resize idle wait)
 (car resize-run) "on-idle runs after resize and timeout wake-ups")

;; The capabilities event of a non-terminal stdin comes before the first idle.
(define caps-log nil)

(terminal-event-loop
 (lambda (ev) (set! caps-log (cons (cdr (assoc 'type ev)) caps-log))
   nil)
 (list (cons :probe #t) (cons :tty-p nil)
  (cons :on-idle (lambda () (set! caps-log (cons 'idle caps-log))))
  (cons :wait-fn
   (lambda (timeout) (set! caps-log (cons 'wait caps-log))
     'input))
  (cons :read-byte-fn (lambda () nil))
  (cons :available-fn (lambda () nil))))

(assert-equal '(capabilities idle wait) (reverse caps-log)
 "on-idle runs after the capabilities event and before the first wait")

;; :on-idle runs after :on-start.
(define start-log nil)

(terminal-event-loop (lambda (ev) nil)
 (list (cons :probe nil) (cons :tty-p nil)
  (cons :on-start (lambda () (set! start-log (cons 'start start-log))))
  (cons :on-idle (lambda () (set! start-log (cons 'idle start-log))))
  (cons :wait-fn
   (lambda (timeout) (set! start-log (cons 'wait start-log))
     'input))
  (cons :read-byte-fn (lambda () nil))
  (cons :available-fn (lambda () nil))))

(assert-equal '(start idle wait) (reverse start-log)
 "on-idle runs after on-start")

;; The tick timeout is computed after on-idle, so time spent in the thunk
;; counts against the tick interval instead of delaying the tick: with a
;; 50 ms interval and a 30 ms thunk, the first wait gets about 20 ms.
(define seen-timeouts nil)

(terminal-event-loop
 (lambda (ev) (when (equal? (cdr (assoc 'type ev)) 'tick) 'stop))
 (list (cons :tick-ms 50) (cons :probe nil) (cons :tty-p nil)
  (cons :on-idle (lambda () (sleep 30)))
  (cons :wait-fn
   (lambda (timeout) (set! seen-timeouts (cons timeout seen-timeouts))
     'timeout))
  (cons :read-byte-fn (lambda () nil))
  (cons :available-fn (lambda () nil))))

(assert-true (< (car (reverse seen-timeouts)) 45)
 "the tick timeout is computed after a slow on-idle thunk")

;; An error from the thunk unwinds like a handler error.
(define idle-error
  (terminal-event-loop (lambda (ev) nil)
   (list (cons :probe nil) (cons :tty-p nil)
    (cons :on-idle (lambda () (error "idle exploded")))
    (cons :wait-fn (fake-wait '(timeout)))
    (cons :read-byte-fn (lambda () nil))
    (cons :available-fn (lambda () nil)))))

(assert-true (error? idle-error)
 "an error from :on-idle is returned by the loop")

(assert-equal 0 terminal:*terminal-raw-depth*
 "raw depth is restored after an :on-idle error")

;; ---------------------------------------------------------------------------
;; UTF-8 across wake-ups: a character split over two reads is one key event.
;; ---------------------------------------------------------------------------
(defun run-chunked (chunks max-events)
  "Run the loop over CHUNKS, one chunk per wake-up. Stop after MAX-EVENTS
   events (nil = never stop); return the events in arrival order."
  (let ((current nil)
        (pending chunks)
        (seen nil))
    (terminal-event-loop
     (lambda (ev) (set! seen (cons ev seen))
       (when (and max-events (>= (length seen) max-events)) 'stop))
     (loop-opts
      (lambda (timeout)
        (if (null? pending)
          'input ; EOF: a real terminal reports it as readable, forever
          (progn (set! current (car pending)) (set! pending (cdr pending))
            'input)))
      (lambda ()
        (if (null? current)
          nil
          (let ((b (car current)))
            (set! current (cdr current))
            b)))
      (lambda () (not (null? current)))))
    (reverse seen)))

(define split-events (run-chunked '((195) (165)) 1))

(assert-equal 1 (length split-events) "a split character is one event")
(assert-equal "å" (terminal-key-name (car split-events)) "split character name")
(assert-equal 229 (terminal-key-code (car split-events)) "split character code")
(assert-equal "å" (terminal-key-text (car split-events)) "split character text")

;; EOF in the middle of a character: no event, and the loop ends.
(define partial-eof-events (run-chunked '((195)) nil))

(assert-equal '() partial-eof-events
 "a partial character at EOF produces no event")

;; A malformed byte does not swallow the ASCII that exposed the error.
(define malformed-events (run-chunked '((226 130 65)) 2))

(assert-equal 2 (length malformed-events) "malformed input yields two events")
(assert-equal (char->string (code-char #xFFFD))
 (terminal-key-name (car malformed-events))
 "the malformed subpart becomes U+FFFD")
(assert-equal "A" (terminal-key-name (cadr malformed-events))
 "the exposing byte is delivered as text")
(assert-equal "A" (terminal-key-text (cadr malformed-events))
 "the exposing byte is typed text")

;; ... including when the split falls between the malformed byte and the
;; exposing byte.
(define split-malformed-events (run-chunked '((226 130) (65)) 2))

(assert-equal (map terminal-key-name malformed-events)
 (map terminal-key-name split-malformed-events)
 "a split before the exposing byte gives the same events")
