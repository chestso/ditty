;; Terminal event library.
;;
;; This library is built on top of the C primitives in `docstrings/tui.md`
;; (raw mode, byte input, terminal size, resize notification, and the blocking
;; `terminal-wait`) and implements the higher-level terminal features that
;; cannot be done in pure Lisp without raw byte access: keyboard decoding,
;; mouse events, focus events, bracketed paste, terminal reply parsing
;; (capability probes), and clipboard integration via OSC 52.
;;
;; The event loop blocks in exactly one place (`terminal-wait`) and never
;; sleeps or polls: idle CPU is zero and input latency is not quantised by a
;; poll interval. Errors are values: the loop returns the error object instead
;; of printing or exiting.
;;
;; Usage:
;;   (require 'terminal)
;;   (use-package 'terminal)
;;   (terminal-event-loop (lambda (ev) (handle ev)) '((:mouse . #t)))
;;
;; Public API (exported from the `terminal` package):
;;   terminal-read-event, terminal-read-escape-sequence,
;;   terminal-key-event?, terminal-key-name, terminal-key-byte,
;;   terminal-key-mods, terminal-reply-event?, terminal-parse-csi-reply,
;;   terminal-mouse-event?, terminal-focus-event?,
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
 'terminal-key-event?
 'terminal-key-name
 'terminal-key-byte
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
  "Convert a list of byte integers to a string."
  (join (map (lambda (b) (char->string (code-char b))) ints) ""))

(defun terminal-opt-value (opts key default)
  "Look KEY up in the OPTS alist, returning DEFAULT when absent."
  (let ((pair (if (null? opts) nil (assoc key opts))))
    (if pair (cdr pair) default)))

;; ---------------------------------------------------------------------------
;; Escape sequence buffer
;; ---------------------------------------------------------------------------
(defun terminal-read-escape-sequence (read-byte-fn)
  "Read bytes following an ESC (0x1b) and return them as a list of integers.
   The leading ESC is not included. Handles SS3, CSI, and OSC sequences.

   READ-BYTE-FN must return nil when no byte is already available, and must
   never wait for one: a lone Escape is detected by the absence of a
   continuation byte, not by a timeout."
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
  "Parse a CSI byte list SEQ (which starts with 91, '[') into a triple
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
  "Return #t if the CSI byte list SEQ carries a terminal reply, not a key."
  (let ((parsed (terminal-parse-csi-reply seq)))
    (if (null? parsed)
      nil
      (or (equal? (car parsed) "?") (equal? (car parsed) ">")
          (equal? (car parsed) "=") (equal? (caddr parsed) "t")))))

;; ---------------------------------------------------------------------------
;; Keyboard events
;; ---------------------------------------------------------------------------
(defun terminal-lookup-csi-key (final param)
  "Map a CSI final byte and optional numeric parameter to a key name.
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
  "Map an SS3 final byte to a key name. Returns nil if not recognized."
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
   SEQ starts with 91 and ends with the final byte."
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

(defun terminal-kitty-key-name (code)
  "Return the base key name for a kitty keyboard code point CODE."
  (cond
    ((= code 27) "escape")
    ((= code 13) "return")
    ((= code 9) "tab")
    ((= code 127) "backspace")
    ((and (>= code 0) (<= code 1114111)) (char->string (code-char code)))
    (#t (format nil "key-~A" code))))

(defun terminal-make-key-event (name byte mods event)
  "Build a key event alist."
  (let* ((ev (list (cons 'type 'key) (cons 'key name)))
         (ev (if (null? byte) ev (append ev (list (cons 'byte byte))))))
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
               (name (concat (terminal-modifier-prefix mods) base))
               (byte (if (< code 128) code nil)))
          (terminal-make-key-event name byte mods event))
        (let ((name (terminal-lookup-csi-key final code)))
          (if name
            (terminal-make-key-event name nil mods event)
            (terminal-make-key-event (format nil "csi-~A-~A" code final) nil
             mods
             event)))))))

(defun terminal-parse-keyboard-sequence (seq)
  "Parse a list of bytes after ESC into a keyboard event alist."
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

(defun terminal-key-byte (ev)
  "Return the byte field of a keyboard event, or nil."
  (and (terminal-key-event? ev) (cdr (assoc 'byte ev))))

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
   SEQ is the bytes after ESC."
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
(defun terminal-read-event (read-byte-fn)
  "Read one terminal event using READ-BYTE-FN. Returns a keyboard event for
   printable input, a mouse/focus/paste marker for special sequences, a reply
   event for a CSI terminal reply, or nil on timeout/EOF.

   READ-BYTE-FN must return nil when no byte is already available and must
   never wait: after ESC, the absence of a continuation byte means a lone
   Escape.  The limit of that rule is that a legacy escape sequence split
   across two writes reads as `escape` plus text; terminals write a sequence
   in one write, and the kitty keyboard protocol (flag 1) removes the issue
   wherever it is supported."
  (define b (read-byte-fn))
  (cond
    ((null? b) nil)
    ((= b 27) ; ESC
     (let ((seq (terminal-read-escape-sequence read-byte-fn)))
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
    (#t ; printable ASCII/Unicode byte -- return the byte as key
     (terminal-make-key-event (char->string (code-char b)) b 0 'press))))

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
         (loop-error nil)
         (probe-pending (if (and probe tty-p) #t nil))
         (replies nil)
         (flags-pushed #f)
         (last-tick (current-time-ms)))
    ;; Ditty only propagates an error raised in a tail position; elsewhere the
    ;; error object is returned as a value.  Every call into user code is
    ;; therefore bound first and checked for `error?`, so a failing handler
    ;; stops the loop and comes back as the loop's return value.
    (define (deliver ev)
      (let ((outcome (handler ev)))
        (cond
          ((error? outcome) (set! loop-error outcome) (set! stop #t))
          ((equal? outcome 'stop) (set! stop #t))
          (#t nil))))
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
        ((null? ev) (set! eof #t))
        ((error? ev) (set! loop-error ev) (set! stop #t))
        ((and probe-pending (terminal-reply-event? ev))
         (set! replies (cons ev replies))
         (if
           (and (equal? (cdr (assoc 'prefix ev)) "?")
                (equal? (cdr (assoc 'final ev)) "c"))
           (finish-probe)
           nil))
        (#t (deliver ev))))
    (define (make-reader) (define first #t)
      (lambda ()
        (if first
          (progn (set! first #f) (read-byte-fn))
          (if (available-fn) (read-byte-fn) nil))))
    (define (drain-input)
      (let loop ()
        (let ((ev (terminal-read-event (make-reader))))
          (cond
            ((null? ev) (set! eof #t))
            ((error? ev) (set! loop-error ev) (set! stop #t))
            (#t
             (dispatch ev)
             (deliver-maybe-tick)
             (when (and (not stop) (not eof) (available-fn)) (loop)))))))
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
            ((error? why) (set! loop-error why) (set! stop #t))
            ((equal? why 'resize) (deliver-resize) (deliver-maybe-tick))
            ((equal? why 'timeout) (deliver-maybe-tick))
            ((equal? why 'input) (drain-input))
            (#t nil))
          (when (and (not stop) (not eof)) (loop)))))
    (define (body)
      (let ((started (if (null? on-start) nil (on-start))))
        (when (error? started) (set! loop-error started)
          (set! stop #t)))
      (when probe
        (if tty-p
          (princ *terminal-probe*)
          (deliver
           (list (cons 'type 'capabilities)
            (cons 'caps (terminal-capabilities-from-replies nil))))))
      (unless stop (loop)))
    (let ((result
           (condition-case err
             (terminal-call-with-raw
              (lambda ()
                (unwind-protect (body)
                  (when flags-pushed (terminal-pop-keyboard-flags)
                    (set! flags-pushed #f))))
              opts)
             (error err))))
      (cond
        ((error? result) result)
        ((error? loop-error) loop-error)
        (#t nil)))))

(provide 'terminal)
