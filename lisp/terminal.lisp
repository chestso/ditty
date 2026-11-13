;; Terminal event library.
;;
;; This library is built on top of the C primitives in `docstrings/tui.md`
;; (raw mode, byte input, terminal size, resize notification, and the blocking
;; `terminal-wait`) and implements the higher-level terminal features that
;; cannot be done in pure Lisp without raw byte access: keyboard decoding,
;; mouse events, focus events, bracketed paste, terminal reply parsing
;; (capability probes), and clipboard integration via OSC 52.
;;
;; Input is decoded from UTF-8 into Unicode scalar values before parsing, so a
;; key event is one code point even when its bytes were split across reads.
;;
;; The event loop blocks in exactly one place (`terminal-wait`) and never
;; sleeps or polls: idle CPU is zero and input latency is not quantised by a
;; poll interval. An error raised by user code unwinds the loop (running its
;; cleanups) to the loop's own `condition-case`, which returns the error
;; object instead of printing or exiting.
;;
;; Usage:
;;   (require 'terminal)
;;   (use-package 'terminal)
;;   (terminal-event-loop (lambda (ev) (handle ev)) '((:mouse . #t)))
;;
;; Public API (exported from the `terminal` package):
;;   terminal-read-event, terminal-read-escape-sequence, terminal-make-decoder,
;;   terminal-key-event?, terminal-key-name, terminal-key-code,
;;   terminal-key-text, terminal-key-mods, terminal-reply-event?,
;;   terminal-parse-csi-reply, terminal-mouse-event?, terminal-focus-event?,
;;   terminal-paste-start?, terminal-paste-end?,
;;   terminal-enable-mouse, terminal-disable-mouse,
;;   terminal-enable-focus-events, terminal-disable-focus-events,
;;   terminal-enable-bracketed-paste, terminal-disable-bracketed-paste,
;;   terminal-set-clipboard, terminal-request-clipboard,
;;   terminal-enter-alt-screen, terminal-exit-alt-screen,
;;   terminal-hide-cursor, terminal-show-cursor,
;;   terminal-push-keyboard-flags, terminal-pop-keyboard-flags,
;;   terminal-query-keyboard-flags,
;;   terminal-call-with-raw, terminal-event-loop,
;;   terminal-env-capabilities, terminal-capabilities-from-replies,
;;   terminal-capabilities, terminal-query.
(in-package "terminal")

(export 'terminal-read-event
 'terminal-read-escape-sequence
 'terminal-make-decoder
 'terminal-key-event?
 'terminal-key-name
 'terminal-key-code
 'terminal-key-text
 'terminal-key-mods
 'terminal-reply-event?
 'terminal-parse-csi-reply
 'terminal-mouse-event?
 'terminal-focus-event?
 'terminal-paste-start?
 'terminal-paste-end?
 'terminal-enable-mouse
 'terminal-disable-mouse
 'terminal-enable-focus-events
 'terminal-disable-focus-events
 'terminal-enable-bracketed-paste
 'terminal-disable-bracketed-paste
 'terminal-set-clipboard
 'terminal-request-clipboard
 'terminal-enter-alt-screen
 'terminal-exit-alt-screen
 'terminal-hide-cursor
 'terminal-show-cursor
 'terminal-push-keyboard-flags
 'terminal-pop-keyboard-flags
 'terminal-query-keyboard-flags
 'terminal-call-with-raw
 'terminal-event-loop
 'terminal-env-capabilities
 'terminal-capabilities-from-replies
 'terminal-capabilities
 'terminal-query)

;; ---------------------------------------------------------------------------
;; Internal helpers
;; ---------------------------------------------------------------------------
(defun terminal-last (lst)
  "Return the last element of a non-empty list."
  (if (null? (cdr lst))
    (car lst)
    (terminal-last (cdr lst))))

(defun terminal-integer-list->string (ints)
  "Convert a list of code point integers to a string."
  (join (map (lambda (b) (char->string (code-char b))) ints) ""))

(defun terminal-opt-value (opts key default)
  "Look KEY up in the OPTS alist, returning DEFAULT when absent."
  (let ((pair (if (null? opts) nil (assoc key opts))))
    (if pair (cdr pair) default)))

;; ---------------------------------------------------------------------------
;; UTF-8 decoder
;; ---------------------------------------------------------------------------
;; The parser below works on Unicode scalar values. This decoder turns a byte
;; stream into scalar values, one at a time. It is incremental: a character
;; split across two reads leaves its state in the decoder, and "no byte now"
;; is reported as nil without losing that state. Malformed input follows the
;; Unicode "maximal subpart" rule (Table 3-7): the bytes consumed so far
;; become U+FFFD, and the byte that exposed the error is re-examined as if the
;; decoder were idle, so it is never swallowed.
(defun terminal-make-decoder ()
  "Return a new incremental UTF-8 decoder. Decoder state is mutable: pass the
   same decoder to successive `terminal-read-event` calls so a character split
   across two reads decodes correctly."
  (let ((dec (make-vector 5 nil)))
    (vector-set! dec 0 0)
    (vector-set! dec 1 0)
    (vector-set! dec 2 #x80)
    (vector-set! dec 3 #xBF)
    (vector-set! dec 4 nil)
    dec))

(defun terminal-decoder-reset! (dec)
  "Reset DEC to the idle state."
  (vector-set! dec 0 0)
  (vector-set! dec 1 0)
  (vector-set! dec 2 #x80)
  (vector-set! dec 3 #xBF))

(defun terminal-decoder-start! (dec need acc lo hi)
  "Put DEC in the middle of a character: NEED continuations, accumulator ACC,
   next continuation in [LO, HI]."
  (vector-set! dec 0 need)
  (vector-set! dec 1 acc)
  (vector-set! dec 2 lo)
  (vector-set! dec 3 hi)
  nil)

(defun terminal-decoder-feed (dec byte)
  "Feed one BYTE to DEC. Return a code point, or nil while a character is
   still incomplete. A malformed sequence yields U+FFFD and files at most one
   more code point (the re-examined byte's result) in DEC's held slot."
  (if (= (vector-ref dec 0) 0)
    (cond
      ((< byte #x80) byte)
      ((and (>= byte #xC2) (<= byte #xDF))
       (terminal-decoder-start! dec 1 (logand byte #x1F) #x80 #xBF))
      ((= byte #xE0)
       (terminal-decoder-start! dec 2 (logand byte #x0F) #xA0 #xBF))
      ((or (and (>= byte #xE1) (<= byte #xEC))
           (and (>= byte #xEE) (<= byte #xEF)))
       (terminal-decoder-start! dec 2 (logand byte #x0F) #x80 #xBF))
      ((= byte #xED)
       (terminal-decoder-start! dec 2 (logand byte #x0F) #x80 #x9F))
      ((= byte #xF0)
       (terminal-decoder-start! dec 3 (logand byte #x07) #x90 #xBF))
      ((and (>= byte #xF1) (<= byte #xF3))
       (terminal-decoder-start! dec 3 (logand byte #x07) #x80 #xBF))
      ((= byte #xF4)
       (terminal-decoder-start! dec 3 (logand byte #x07) #x80 #x8F))
      (#t #xFFFD)) ; stray continuation, overlong lead, invalid lead
    (let ((lo (vector-ref dec 2))
          (hi (vector-ref dec 3)))
      (if (and (>= byte lo) (<= byte hi))
        (let ((acc (logior (ash (vector-ref dec 1) 6) (logand byte #x3F)))
              (left (- (vector-ref dec 0) 1)))
          (if (= left 0)
            (progn (terminal-decoder-reset! dec) acc)
            (terminal-decoder-start! dec left acc #x80 #xBF)))
        ;; The maximal subpart consumed so far is malformed: replace it with
        ;; U+FFFD, then re-examine this byte as if the decoder were idle.
        (progn (terminal-decoder-reset! dec)
          (vector-set! dec 4 (terminal-decoder-feed dec byte))
          #xFFFD)))))

(defun terminal-decoder-next (dec read-byte-fn)
  "Return the next code point from DEC, reading bytes with READ-BYTE-FN, or
   nil when no byte is available right now. State survives across calls; nil
   never means EOF by itself."
  (let ((held (vector-ref dec 4)))
    (if (not (null? held))
      (progn (vector-set! dec 4 nil) held)
      (let loop ()
        (let ((byte (read-byte-fn)))
          (if (null? byte)
            nil
            (let ((cp (terminal-decoder-feed dec byte)))
              (if (null? cp) (loop) cp))))))))

;; ---------------------------------------------------------------------------
;; Escape sequence buffer
;; ---------------------------------------------------------------------------
(defun terminal-read-escape-sequence (read-byte-fn)
  "Read the units following an ESC (0x1b) and return them as a list.
   The leading ESC is not included. Handles SS3, CSI, and OSC sequences.

   A unit is a byte or a decoded Unicode scalar value; every sequence byte is
   below 0x80, so the two coincide wherever the grammar matters. READ-BYTE-FN
   must return nil when no unit is already available, and must never wait for
   one: a lone Escape is detected by the absence of a continuation unit, not by
   a timeout."
  (define b1 (read-byte-fn))
  (cond
    ((null? b1) nil)
    ((= b1 79) ; SS3: ESC O <final>
     (let ((final (read-byte-fn)))
       (if (null? final)
         '(79)
         (list 79 final))))
    ((= b1 91) ; CSI: ESC [ <body> <final>
     (let loop ((acc nil))
       (define b (read-byte-fn))
       (cond
         ((null? b) (cons 91 (reverse acc)))
         ((= b 27) (cons 91 (reverse acc))) ; ESC starts new sequence
         ((= b 126) (cons 91 (reverse (cons b acc)))) ; ~ ends CSI ~ sequences
         ((and (>= b 64) (<= b 126) (not (char-numeric? (code-char b)))
               (not (= b 59)) ; ;
               (not (= b 60)) ; <
               (not (= b 61)) ; =
               (not (= b 62)) ; >
               (not (= b 63))) ; ?
          (cons 91 (reverse (cons b acc))))
         (#t (loop (cons b acc))))))
    ((= b1 93) ; OSC: ESC ] ... BEL or ESC
     (let loop ((acc nil))
       (define b (read-byte-fn))
       (cond
         ((null? b) (cons 93 (reverse acc)))
         ((= b 7) (cons 93 (reverse (cons b acc)))) ; BEL ends OSC
         ((= b 27) (cons 93 (reverse acc))) ; ESC may start ST
         (#t (loop (cons b acc))))))
    (#t (list b1))))

;; ---------------------------------------------------------------------------
;; CSI parsing
;; ---------------------------------------------------------------------------
(defun terminal-parse-csi-field (field)
  "Parse one CSI parameter FIELD, dropping any ':' sub-parameter.
   An empty field is 0; non-numeric text also yields 0."
  (let* ((colon (string-index field ":"))
         (head (if colon (substring field 0 colon) field)))
    (if (or (null? head) (equal? head ""))
      0
      (let ((value (string->number head)))
        (if value value 0)))))

(defun terminal-parse-csi-reply (seq)
  "Parse a CSI unit list SEQ (which starts with 91, '[') into a triple
   (PREFIX PARAMS FINAL).

   PREFIX is \"?\", \">\", \"<\", \"=\" or \"\".  PARAMS is a list of
   integers: an empty field is 0 and sub-parameters after ':' are dropped.
   FINAL is a one-character string.  Returns nil for non-CSI input."
  (if (or (null? seq) (not (= (car seq) 91)) (null? (cdr seq)))
    nil
    (let* ((final (terminal-last seq))
           (body (reverse (cdr (reverse (cdr seq)))))
           (first-byte (if (null? body) nil (car body)))
           (prefix
            (cond
              ((equal? first-byte 63) "?")
              ((equal? first-byte 62) ">")
              ((equal? first-byte 60) "<")
              ((equal? first-byte 61) "=")
              (#t "")))
           (param-bytes (if (equal? prefix "") body (cdr body)))
           (param-string (terminal-integer-list->string param-bytes)))
      (list prefix
       (map terminal-parse-csi-field (split param-string ";"))
       (char->string (code-char final))))))

(defun terminal-csi-reply-seq? (seq)
  "Return #t if the CSI unit list SEQ carries a terminal reply, not a key."
  (let ((parsed (terminal-parse-csi-reply seq)))
    (if (null? parsed)
      nil
      (or (equal? (car parsed) "?") (equal? (car parsed) ">")
          (equal? (car parsed) "=") (equal? (caddr parsed) "t")))))

;; ---------------------------------------------------------------------------
;; Keyboard events
;; ---------------------------------------------------------------------------
(defun terminal-lookup-csi-key (final param)
  "Map a CSI final unit and optional numeric parameter to a key name.
   Returns nil if the sequence is not recognized."
  (if (= final 126)
    (cond
      ((= param 1) "home")
      ((= param 2) "insert")
      ((= param 3) "delete")
      ((= param 4) "end")
      ((= param 5) "pageup")
      ((= param 6) "pagedown")
      ((= param 11) "f1")
      ((= param 12) "f2")
      ((= param 13) "f3")
      ((= param 14) "f4")
      ((= param 15) "f5")
      ((= param 17) "f6")
      ((= param 18) "f7")
      ((= param 19) "f8")
      ((= param 20) "f9")
      ((= param 21) "f10")
      ((= param 23) "f11")
      ((= param 24) "f12")
      (#t nil))
    (cond
      ((= final 65) "up")
      ((= final 66) "down")
      ((= final 67) "right")
      ((= final 68) "left")
      ((= final 70) "end")
      ((= final 72) "home")
      ((= final 80) "f1")
      ((= final 81) "f2")
      ((= final 82) "f3")
      ((= final 83) "f4")
      (#t nil))))

(defun terminal-lookup-ss3-key (final)
  "Map an SS3 final unit to a key name. Returns nil if not recognized."
  (cond
    ((= final 65) "up")
    ((= final 66) "down")
    ((= final 67) "right")
    ((= final 68) "left")
    ((= final 70) "end")
    ((= final 72) "home")
    (#t nil)))

(defun terminal-csi-fields (seq)
  "Return the ';'-separated parameter fields of a CSI sequence as strings.
   SEQ starts with 91 and ends with the final unit."
  (let* ((body (reverse (cdr (reverse (cdr seq)))))
         (first-byte (if (null? body) nil (car body)))
         (stripped
          (if
            (or (equal? first-byte 63) (equal? first-byte 62)
                (equal? first-byte 60) (equal? first-byte 61))
            (cdr body)
            body)))
    (split (terminal-integer-list->string stripped) ";")))

(defun terminal-csi-event (field)
  "Return the kitty event symbol for a ':EVENT' suffix on FIELD."
  (let* ((colon (if (null? field) nil (string-index field ":")))
         (tail (if colon (substring field (+ colon 1) (length field)) nil)))
    (cond
      ((null? tail) 'press)
      ((equal? tail "2") 'repeat)
      ((equal? tail "3") 'release)
      (#t 'press))))

(defun terminal-modifier-prefix (mods)
  "Return the name prefix for the modifier bitmask MODS (ctrl, alt, super)."
  (concat
   (if (not (= 0 (logand mods 4))) "C-" "")
   (if (not (= 0 (logand mods 2))) "M-" "")
   (if (not (= 0 (logand mods 8))) "s-" "")))

(defun terminal-scalar-value? (code)
  "Return #t if CODE is a Unicode scalar value: 0..U+10FFFF, no surrogates."
  (and (>= code 0) (<= code 1114111)
       (not (and (>= code 55296) (<= code 57343)))))

(defun terminal-kitty-key-name (code)
  "Return the base key name for a kitty keyboard code point CODE."
  (cond
    ((= code 27) "escape")
    ((= code 13) "return")
    ((= code 9) "tab")
    ((= code 127) "backspace")
    ((terminal-scalar-value? code) (char->string (code-char code)))
    (#t (format nil "key-~A" code))))

(defun terminal-make-key-event (name code mods event &optional text)
  "Build a key event alist. CODE is the Unicode scalar value for character
   keys (nil for named keys); TEXT is the typed string, and is present only
   for plain printable input."
  (let* ((ev (list (cons 'type 'key) (cons 'key name)))
         (ev (if (null? code) ev (append ev (list (cons 'code code)))))
         (ev (if (null? text) ev (append ev (list (cons 'text text))))))
    (append ev
     (list (cons 'mods (if (null? mods) 0 mods))
      (cons 'event (if (null? event) 'press event))))))

(defun terminal-parse-csi-key (seq)
  "Parse a CSI keyboard sequence (SEQ starts with 91) into a key event."
  (if (< (length seq) 2)
    (list (cons 'type 'unknown) (cons 'data seq))
    (let* ((final (terminal-last seq))
           (fields (terminal-csi-fields seq))
           (first-field (if (null? fields) "" (car fields)))
           (mods-field (if (null? (cdr fields)) nil (cadr fields)))
           (code (terminal-parse-csi-field first-field))
           (mods-raw
            (if (null? mods-field) 0 (terminal-parse-csi-field mods-field)))
           (mods (if (> mods-raw 0) (- mods-raw 1) 0))
           (event (terminal-csi-event mods-field)))
      (if (= final 117) ; 'u' - kitty keyboard protocol
        (let* ((base (terminal-kitty-key-name code))
               (name (concat (terminal-modifier-prefix mods) base)))
          ;; CSI u never carries associated text: flag 1 does not request it,
          ;; and a modified key must not look like plain input.
          (terminal-make-key-event name code mods event))
        (let ((name (terminal-lookup-csi-key final code)))
          (if name
            (terminal-make-key-event name nil mods event)
            (terminal-make-key-event (format nil "csi-~A-~A" code final) nil
             mods
             event)))))))

(defun terminal-parse-keyboard-sequence (seq)
  "Parse a list of units after ESC into a keyboard event alist."
  (cond
    ((null? seq) (terminal-make-key-event "escape" nil 0 'press))
    ((= (car seq) 79) ; SS3 (ESC O ...)
     (let* ((final (if (null? (cdr seq)) nil (cadr seq)))
            (name (if (null? final) nil (terminal-lookup-ss3-key final))))
       (if name
         (terminal-make-key-event name nil 0 'press)
         (terminal-make-key-event (format nil "ss3-~A" final) nil 0 'press))))
    ((= (car seq) 91) ; CSI (ESC [ ...)
     (terminal-parse-csi-key seq))
    (#t
     (terminal-make-key-event (format nil "esc-~A" (car seq)) nil 0 'press))))

(defun terminal-key-event? (ev)
  "Return #t if EV is a keyboard event."
  (and (pair? ev) (equal? (cdr (assoc 'type ev)) 'key)))

(defun terminal-key-name (ev)
  "Return the key name of a keyboard event, or nil if not a key event."
  (and (terminal-key-event? ev) (cdr (assoc 'key ev))))

(defun terminal-key-code (ev)
  "Return the code field of a keyboard event, or nil.
   The code is a Unicode scalar value for character keys."
  (and (terminal-key-event? ev) (cdr (assoc 'code ev))))

(defun terminal-key-text (ev)
  "Return the text field of a keyboard event, or nil.
   Text is present only for plain printable input: a string safe to insert."
  (and (terminal-key-event? ev) (cdr (assoc 'text ev))))

(defun terminal-key-mods (ev)
  "Return the modifier bitmask of a keyboard event (0 when absent)."
  (if (terminal-key-event? ev)
    (let ((mods (cdr (assoc 'mods ev))))
      (if (null? mods) 0 mods))
    0))

;; ---------------------------------------------------------------------------
;; Reply events
;; ---------------------------------------------------------------------------
(defun terminal-reply-event? (ev)
  "Return #t if EV is a terminal reply event."
  (and (pair? ev) (equal? (cdr (assoc 'type ev)) 'reply)))

;; ---------------------------------------------------------------------------
;; Mouse events (SGR extended protocol)
;; ---------------------------------------------------------------------------
(defun terminal-enable-mouse ()
  "Enable mouse reporting."
  (princ "\033[?1003h\033[?1006h"))

(defun terminal-disable-mouse ()
  "Disable mouse reporting."
  (princ "\033[?1003l\033[?1006l"))

(defun terminal-parse-mouse-sequence (seq)
  "Parse an SGR mouse sequence (ESC [ < Cb ; Cx ; Cy (M|m)) into an event.
   SEQ is the units after ESC."
  (if
    (or (null? seq) (not (= (car seq) 91)) (< (length seq) 2)
        (not (= (cadr seq) 60)))
    (list (cons 'type 'unknown) (cons 'data seq))
    (let* ((str (terminal-integer-list->string (cddr seq)))
           (parts (string-split str ";"))
           (button-str (car parts))
           (x-str (cadr parts))
           (y-tail (caddr parts))
           (release-tail (string-split y-tail "m"))
           (press-tail (string-split y-tail "M"))
           (release (not (null? (cdr release-tail))))
           (tail (if release release-tail press-tail))
           (y-str (car tail))
           (button (string->number button-str))
           (x (string->number x-str))
           (y (string->number y-str)))
      (list (cons 'type 'mouse)
       (cons 'button (if release -1 button))
       (cons 'x x)
       (cons 'y y)
       (cons 'release release)))))

(defun terminal-mouse-event? (ev)
  "Return #t if EV is a mouse event."
  (and (pair? ev) (equal? (cdr (assoc 'type ev)) 'mouse)))

;; ---------------------------------------------------------------------------
;; Focus events
;; ---------------------------------------------------------------------------
(defun terminal-enable-focus-events ()
  "Enable focus in/out events."
  (princ "\033[?1004h"))

(defun terminal-disable-focus-events ()
  "Disable focus in/out events."
  (princ "\033[?1004l"))

(defun terminal-focus-sequence? (seq)
  "Return #t if SEQ is a CSI focus-in/focus-out sequence."
  (and (not (null? seq)) (= (car seq) 91) (> (length seq) 1)
       (let ((final (terminal-last seq)))
         (or (= final 73) (= final 79)))))

(defun terminal-parse-focus-sequence (seq)
  "Parse a focus event (ESC [ I or ESC [ O)."
  (if (null? seq)
    (list (cons 'type 'focus) (cons 'focus 'in))
    (let ((final (terminal-last seq)))
      (if (= final 73)
        (list (cons 'type 'focus) (cons 'focus 'in))
        (list (cons 'type 'focus) (cons 'focus 'out))))))

(defun terminal-focus-event? (ev)
  "Return #t if EV is a focus event."
  (and (pair? ev) (equal? (cdr (assoc 'type ev)) 'focus)))

;; ---------------------------------------------------------------------------
;; Bracketed paste
;; ---------------------------------------------------------------------------
(defun terminal-enable-bracketed-paste ()
  "Enable bracketed paste mode."
  (princ "\033[?2004h"))

(defun terminal-disable-bracketed-paste ()
  "Disable bracketed paste mode."
  (princ "\033[?2004l"))

(defun terminal-paste-start? (seq)
  "Return #t if SEQ is the bracketed paste start marker ESC [ 2 0 0 ~."
  (and (not (null? seq)) (= (length seq) 5) (= (car seq) 91) (= (cadr seq) 50)
       (= (caddr seq) 48) (= (cadddr seq) 48) (= (terminal-last seq) 126)))

(defun terminal-paste-end? (seq)
  "Return #t if SEQ is the bracketed paste end marker ESC [ 2 0 1 ~."
  (and (not (null? seq)) (= (length seq) 5) (= (car seq) 91) (= (cadr seq) 50)
       (= (caddr seq) 48) (= (cadddr seq) 49) (= (terminal-last seq) 126)))

;; ---------------------------------------------------------------------------
;; Clipboard integration (OSC 52)
;; ---------------------------------------------------------------------------
(defun terminal-set-clipboard (text)
  "Set the system clipboard to TEXT using OSC 52."
  (princ "\033]52;c;")
  (princ text)
  (princ "\033\\"))

(defun terminal-request-clipboard ()
  "Request the system clipboard contents via OSC 52."
  (princ "\033]52;c;?\033\\"))

;; ---------------------------------------------------------------------------
;; Mode helpers
;; ---------------------------------------------------------------------------
(defun terminal-enter-alt-screen ()
  "Switch to the alternate screen buffer."
  (princ "\033[?1049h"))

(defun terminal-exit-alt-screen ()
  "Leave the alternate screen buffer."
  (princ "\033[?1049l"))

(defun terminal-hide-cursor ()
  "Hide the cursor."
  (princ "\033[?25l"))

(defun terminal-show-cursor ()
  "Show the cursor."
  (princ "\033[?25h"))

(defun terminal-push-keyboard-flags (flags)
  "Push kitty keyboard protocol FLAGS onto the terminal's stack."
  (princ (format nil "\033[>~Au" flags)))

(defun terminal-pop-keyboard-flags ()
  "Pop the kitty keyboard protocol flags pushed last."
  (princ "\033[<u"))

(defun terminal-query-keyboard-flags ()
  "Ask the terminal for its current kitty keyboard flags."
  (princ "\033[?u"))

;; ---------------------------------------------------------------------------
;; Raw-mode sessions
;; ---------------------------------------------------------------------------
(define *terminal-raw-depth* 0)

(defun terminal-call-with-raw (thunk &optional opts)
  "Call THUNK with the terminal in raw mode, enabling the modes requested in
   the OPTS alist (:mouse, :focus, :paste, :alt-screen, :hide-cursor; all
   default off).  Cleanup runs on normal return and on error, in reverse
   order, and only the outermost call restores the terminal state.

   Raw-mode nesting is tracked here, so code that calls `set-terminal-raw`
   directly bypasses the counter."
  (let* ((mouse (terminal-opt-value opts :mouse nil))
         (focus (terminal-opt-value opts :focus nil))
         (paste (terminal-opt-value opts :paste nil))
         (alt (terminal-opt-value opts :alt-screen nil))
         (hide (terminal-opt-value opts :hide-cursor nil))
         (outer (= *terminal-raw-depth* 0)))
    (when outer (set-terminal-raw))
    (set! *terminal-raw-depth* (+ *terminal-raw-depth* 1))
    (when mouse (terminal-enable-mouse))
    (when focus (terminal-enable-focus-events))
    (when paste (terminal-enable-bracketed-paste))
    (when alt (terminal-enter-alt-screen))
    (when hide (terminal-hide-cursor))
    (unwind-protect (thunk) (when hide (terminal-show-cursor))
      (when alt (terminal-exit-alt-screen))
      (when paste (terminal-disable-bracketed-paste))
      (when focus (terminal-disable-focus-events))
      (when mouse (terminal-disable-mouse))
      (set! *terminal-raw-depth* (- *terminal-raw-depth* 1))
      (when outer (restore-terminal)))))

;; ---------------------------------------------------------------------------
;; Event reader
;; ---------------------------------------------------------------------------
(defun terminal-read-event (read-byte-fn &optional decoder)
  "Read one terminal event using READ-BYTE-FN. Returns a keyboard event for
   printable input, a mouse/focus/paste marker for special sequences, a reply
   event for a CSI terminal reply, or nil when no event can be produced from
   the input available now.

   READ-BYTE-FN returns the next byte, or nil when none is already available;
   it must never wait. DECODER is an incremental decoder from
   `terminal-make-decoder`; omitting it uses a fresh one (right for finite
   input, where a truncated character simply yields no event). Pass the same
   decoder across calls (the event loop does) so a character split across two
   reads decodes as one key.

   After ESC, the absence of a continuation unit means a lone Escape. The
   limit of that rule is that a legacy escape sequence split across two writes
   reads as `escape` plus text; terminals write a sequence in one write, and
   the kitty keyboard protocol (flag 1) removes the issue wherever it is
   supported."
  (let* ((dec (if (null? decoder) (terminal-make-decoder) decoder))
         (next-unit (lambda () (terminal-decoder-next dec read-byte-fn)))
         (b (next-unit)))
    (cond
      ((null? b) nil)
      ((= b 27) ; ESC
       (let ((seq (terminal-read-escape-sequence next-unit)))
         (cond
           ((terminal-paste-start? seq) (list (cons 'type 'paste-start)))
           ((terminal-paste-end? seq) (list (cons 'type 'paste-end)))
           ((and (not (null? seq)) (= (car seq) 91) (> (length seq) 1)
                 (= (cadr seq) 60))
            (terminal-parse-mouse-sequence seq))
           ((and (not (null? seq)) (= (car seq) 91) (> (length seq) 1)
                 (terminal-csi-reply-seq? seq))
            (let ((reply (terminal-parse-csi-reply seq)))
              (list (cons 'type 'reply)
               (cons 'prefix (car reply))
               (cons 'params (cadr reply))
               (cons 'final (caddr reply)))))
           ((terminal-focus-sequence? seq) (terminal-parse-focus-sequence seq))
           (#t (terminal-parse-keyboard-sequence seq)))))
      ((= b 13) (terminal-make-key-event "return" nil 0 'press)) ; CR -> return
      ((= b 10) (terminal-make-key-event "newline" nil 0 'press)) ; LF
      ((= b 9) (terminal-make-key-event "tab" nil 0 'press))
      ((= b 127) (terminal-make-key-event "backspace" nil 0 'press))
      ((< b 32) ; control character
       (terminal-make-key-event
        (concat "C-" (char->string (char-downcase (code-char (+ b 64))))) b 0
        'press))
      ((and (>= b 128) (<= b 159)) ; C1 control: named, never text
       (terminal-make-key-event (format nil "c1-~A" b) b 0 'press))
      (#t ; printable character -- name and text are the character itself
       (terminal-make-key-event (char->string (code-char b)) b 0 'press
        (char->string (code-char b)))))))

;; ---------------------------------------------------------------------------
;; Capabilities
;; ---------------------------------------------------------------------------
(defconst *terminal-probe* "\033[?u\033[16t\033[14t\033[c"
  "Capability probe: kitty keyboard flags, cell size, pixel size, then the
   Device Attributes sentinel.  Every terminal answers DA1 and replies arrive
   in query order, so DA1 marks the end of the answers.")

(defun terminal-env-capabilities (&optional getenv-fn)
  "Return graphics capabilities inferred from the environment as an alist
   with the keys sixel, kitty-graphics, and iterm2-images.  GETENV-FN
   defaults to `getenv`; this function is pure."
  (let* ((env-fn (if (null? getenv-fn) getenv getenv-fn))
         (portty (env-fn "PORTTY"))
         (term (env-fn "TERM"))
         (kitty (env-fn "KITTY_WINDOW_ID"))
         (term-program (env-fn "TERM_PROGRAM")))
    (list
     (cons 'sixel
      (if
        (or (equal? portty "1")
            (and term (string-contains? (string-downcase term) "sixel")))
        #t
        #f))
     (cons 'kitty-graphics (if kitty #t #f))
     (cons 'iterm2-images (if (equal? term-program "iTerm.app") #t #f)))))

(defun terminal-normalise-reply (reply)
  "Return REPLY as a (PREFIX PARAMS FINAL) triple, or nil."
  (cond
    ((null? reply) nil)
    ((and (pair? reply) (pair? (car reply)))
     (list (alist-get 'prefix reply) (alist-get 'params reply)
      (alist-get 'final reply)))
    ((and (list? reply) (= (length reply) 3)) reply)
    (#t nil)))

(defun terminal-normalise-replies (replies)
  "Normalise a list of replies (triples or reply events), dropping junk."
  (filter (lambda (r) (not (null? r)))
   (map terminal-normalise-reply (if (null? replies) nil replies))))

(defun terminal-find-reply (replies prefix final &optional first-param)
  "Find the first (PREFIX PARAMS FINAL) triple in REPLIES matching PREFIX and
   FINAL (and, when FIRST-PARAM is given, its first parameter)."
  (let loop ((rest replies))
    (cond
      ((null? rest) nil)
      ((and (equal? (car (car rest)) prefix) (equal? (caddr (car rest)) final)
            (or (null? first-param)
                (and (not (null? (cadr (car rest))))
                     (= (car (cadr (car rest))) first-param))))
       (car rest))
      (#t (loop (cdr rest))))))

(defun terminal-reply-dims (reply)
  "Return (width . height) from a 't' reply triple, or nil.
   Reply parameters are (KIND HEIGHT WIDTH)."
  (if (null? reply)
    nil
    (let ((params (cadr reply)))
      (if (or (null? params) (null? (cdr params)) (null? (cddr params)))
        nil
        (cons (caddr params) (cadr params))))))

(defun terminal-compute-cell (pixel-size)
  "Return the cell size (width . height) from PIXEL-SIZE and terminal-size."
  (let ((size (terminal-size)))
    (if (and pixel-size size (> (cdr size) 0) (> (car size) 0))
      (cons (floor (/ (car pixel-size) (cdr size)))
       (floor (/ (cdr pixel-size) (car size))))
      nil)))

(defun terminal-capabilities-from-replies (replies &optional getenv-fn)
  "Fold parsed REPLIES (triples or reply events) and the environment into an
   alist with all of: sixel, kitty-graphics, iterm2-images, kitty-keyboard,
   pixel-size, cell-size, and source.  This function is pure."
  (let* ((env (terminal-env-capabilities getenv-fn))
         (parsed (terminal-normalise-replies replies))
         (da1 (terminal-find-reply parsed "?" "c"))
         (keyboard-reply (terminal-find-reply parsed "?" "u"))
         (pixel-reply (terminal-find-reply parsed "" "t" 4))
         (cell-reply (terminal-find-reply parsed "" "t" 6))
         (env-sixel (alist-get 'sixel env))
         (da-sixel (if (null? da1) nil (if (member 4 (cadr da1)) #t nil)))
         (sixel (if (or da-sixel env-sixel) #t nil))
         (pixel
          (let ((direct (terminal-pixel-size)))
            (if direct direct (terminal-reply-dims pixel-reply))))
         (cell
          (let ((direct (terminal-reply-dims cell-reply)))
            (if direct direct (terminal-compute-cell pixel))))
         (source
          (cond
            ((not (null? da1)) "query")
            ((or env-sixel (alist-get 'kitty-graphics env)
                 (alist-get 'iterm2-images env))
             "env")
            (#t "none"))))
    (list
     (cons 'sixel sixel)
     (cons 'kitty-graphics (alist-get 'kitty-graphics env))
     (cons 'iterm2-images (alist-get 'iterm2-images env))
     (cons 'kitty-keyboard
      (if (null? keyboard-reply)
        nil
        (if (null? (cadr keyboard-reply))
          0
          (car (cadr keyboard-reply)))))
     (cons 'pixel-size pixel)
     (cons 'cell-size cell)
     (cons 'source source))))

(defun terminal-query (query &optional timeout-ms)
  "Write QUERY followed by the DA1 sentinel, then collect the CSI replies.
   Returns the parsed replies in arrival order (DA1 last), or nil when nothing
   arrived.  Blocking: not for use inside `terminal-event-loop`.

   TIMEOUT-MS is the per-byte wait (default 300).  Bytes arriving before the
   first ESC are discarded (typeahead)."
  (let ((timeout (if (null? timeout-ms) 300 timeout-ms)))
    (if (not (terminal-tty-p))
      nil
      (terminal-call-with-raw
       (lambda () (princ query)
         (princ "\033[c")
         (let ((reader (lambda () (read-byte-timeout timeout))))
           (let loop ((acc nil))
             (let ((b (reader)))
               (cond
                 ((null? b) (reverse acc))
                 ((= b 27)
                  (let* ((seq (terminal-read-escape-sequence reader))
                         (parsed (terminal-parse-csi-reply seq)))
                    (cond
                      ((null? parsed) (loop acc))
                      ((and (equal? (car parsed) "?")
                            (equal? (caddr parsed) "c"))
                       (reverse (cons parsed acc)))
                      (#t (loop (cons parsed acc))))))
                 (#t (loop acc)))))))))))

(defun terminal-capabilities (&optional opts)
  "Return the capabilities alist of `terminal-capabilities-from-replies`,
   probing the terminal when possible.  OPTS supports :query (default #t).
   Blocking: for scripts, not for the event loop."
  (let ((query (terminal-opt-value opts :query #t)))
    (if (or (null? query) (not (terminal-tty-p)))
      (terminal-capabilities-from-replies nil)
      (terminal-capabilities-from-replies (terminal-query *terminal-probe*)))))

;; ---------------------------------------------------------------------------
;; Event loop
;; ---------------------------------------------------------------------------
(defun terminal-event-loop (handler &optional opts)
  "Run an event loop, calling HANDLER with one event per call.

   OPTS keys: the session keys of `terminal-call-with-raw` (:mouse, :focus,
   :paste, :alt-screen, :hide-cursor), plus:

   :tick-ms     integer; deliver a ((type . tick)) event that often
   :on-start    thunk called once after setup, before the first wait
   :probe       #t (default) to probe the terminal at start
   :keyboard    #t (default) to push kitty keyboard flags 1 once the probe
                shows support (needs :probe, which is on by default)
   :wait-fn     override for `terminal-wait`
   :read-byte-fn, :available-fn, :tty-p   test/embedding seams

   HANDLER returns the symbol `stop` to end the loop.  The loop returns nil
   after a clean stop or stdin EOF, or the error object; it never prints an
   error and never exits."
  (let* ((tick-ms (terminal-opt-value opts :tick-ms nil))
         (on-start (terminal-opt-value opts :on-start nil))
         (probe (terminal-opt-value opts :probe #t))
         (keyboard (terminal-opt-value opts :keyboard #t))
         (wait-fn (terminal-opt-value opts :wait-fn terminal-wait))
         (read-byte-fn (terminal-opt-value opts :read-byte-fn read-byte))
         (available-fn
          (terminal-opt-value opts :available-fn terminal-input-available-p))
         (tty-p (terminal-opt-value opts :tty-p (terminal-tty-p)))
         (stop #f)
         (eof #f)
         (probe-pending (if (and probe tty-p) #t nil))
         (replies nil)
         (flags-pushed #f)
         (last-tick (current-time-ms)))
    ;; A signal from the handler, the reader, the wait function or `:on-start'
    ;; unwinds out of `body' (running the `unwind-protect' cleanups) to the
    ;; `condition-case' at the bottom, which returns the error object.
    (define (deliver ev) (when (equal? (handler ev) 'stop) (set! stop #t)))
    (define (deliver-tick) (set! last-tick (current-time-ms))
      (deliver (list (cons 'type 'tick))))
    (define (tick-due?)
      (and (not (null? tick-ms)) (>= (current-time-ms) (+ last-tick tick-ms))))
    (define (deliver-maybe-tick) (when (tick-due?) (deliver-tick)))
    (define (deliver-resize)
      (let* ((size (terminal-size))
             (rows (if (null? size) 0 (car size)))
             (cols (if (null? size) 0 (cdr size))))
        (deliver
         (list (cons 'type 'resize) (cons 'rows rows) (cons 'cols cols)))))
    (define (finish-probe)
      (let* ((caps (terminal-capabilities-from-replies (reverse replies)))
             (flags (alist-get 'kitty-keyboard caps)))
        (when (and keyboard (not (null? flags)) (not flags-pushed))
          (terminal-push-keyboard-flags 1)
          (set! flags-pushed #t))
        (set! probe-pending nil)
        (deliver (list (cons 'type 'capabilities) (cons 'caps caps)))))
    (define (dispatch ev)
      (cond
        ((and probe-pending (terminal-reply-event? ev))
         (set! replies (cons ev replies))
         (if
           (and (equal? (cdr (assoc 'prefix ev)) "?")
                (equal? (cdr (assoc 'final ev)) "c"))
           (finish-probe)
           nil))
        (#t (deliver ev))))
    ;; One decoder per loop, so a character split across two wake-ups decodes
    ;; as one key. The first read of a wake-up is unconditional so that EOF is
    ;; observed; a nil read is EOF, and is recorded here at the source.
    (define decoder (terminal-make-decoder))
    (define (make-reader forced) (define first forced)
      (lambda ()
        (if (or first (available-fn))
          (progn (set! first #f)
            (let ((b (read-byte-fn)))
              (when (null? b) (set! eof #t))
              b))
          nil)))
    ;; Drain until the reader has no event now. A decoder holding a partial
    ;; character also makes terminal-read-event return nil; that is not EOF,
    ;; which only the byte reader above can report.
    (define (drain-input)
      (let loop ((forced #t))
        (let ((ev (terminal-read-event (make-reader forced) decoder)))
          (when ev (dispatch ev)
            (deliver-maybe-tick)
            (when (and (not stop) (not eof)) (loop #f))))))
    (define (next-timeout)
      (if (null? tick-ms)
        nil
        (let ((left (- (+ last-tick tick-ms) (current-time-ms))))
          (if (< left 0) 0 left))))
    (define (loop)
      (if stop
        nil
        (let ((why (wait-fn (next-timeout))))
          (cond
            ((equal? why 'resize) (deliver-resize) (deliver-maybe-tick))
            ((equal? why 'timeout) (deliver-maybe-tick))
            ((equal? why 'input) (drain-input))
            (#t nil))
          (when (and (not stop) (not eof)) (loop)))))
    (define (body) (when (not (null? on-start)) (on-start))
      (when probe
        (if tty-p
          (princ *terminal-probe*)
          (deliver
           (list (cons 'type 'capabilities)
            (cons 'caps (terminal-capabilities-from-replies nil))))))
      (unless stop (loop)))
    (condition-case err
      (terminal-call-with-raw
       (lambda ()
         (unwind-protect (body)
           (when flags-pushed (terminal-pop-keyboard-flags)
             (set! flags-pushed #f))))
       opts)
      (error err))))

(provide 'terminal)
