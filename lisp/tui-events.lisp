;; TUI event parsing library.
;;
;; This library is built on top of the C primitives in `docstrings/tui.md` and
;; implements the higher-level terminal features that cannot be done in pure
;; Lisp without raw byte access: keyboard decoding, mouse events, focus
;; events, bracketed paste, and clipboard integration via OSC 52.
;;
;; Usage:
;;   (require 'tui-events)
;;   (use-package 'tui-events)
;;   (tui-read-event read-byte-fn)
;;
;; Public API (exported from the `tui-events` package):
;;   tui-read-event, tui-read-escape-sequence,
;;   tui-key-event?, tui-key-name,
;;   tui-mouse-event?, tui-enable-mouse, tui-disable-mouse,
;;   tui-focus-event?, tui-enable-focus-events, tui-disable-focus-events,
;;   tui-enable-bracketed-paste, tui-disable-bracketed-paste,
;;   tui-paste-start?, tui-paste-end?,
;;   tui-set-clipboard, tui-request-clipboard.
(in-package "tui-events")

(export 'tui-read-event
 'tui-read-escape-sequence
 'tui-key-event?
 'tui-key-name
 'tui-mouse-event?
 'tui-enable-mouse
 'tui-disable-mouse
 'tui-focus-event?
 'tui-enable-focus-events
 'tui-disable-focus-events
 'tui-enable-bracketed-paste
 'tui-disable-bracketed-paste
 'tui-paste-start?
 'tui-paste-end?
 'tui-set-clipboard
 'tui-request-clipboard)

;; ---------------------------------------------------------------------------
;; Internal helpers
;; ---------------------------------------------------------------------------
(defun tui-last (lst)
  "Return the last element of a non-empty list."
  (if (null? (cdr lst))
    (car lst)
    (tui-last (cdr lst))))

(defun tui-integer-list->string (ints)
  "Convert a list of byte integers to a string."
  (join (map (lambda (b) (char->string (code-char b))) ints) ""))

;; ---------------------------------------------------------------------------
;; Escape sequence buffer
;; ---------------------------------------------------------------------------
(defun tui-read-escape-sequence (read-byte-fn)
  "Read bytes following an ESC (0x1b) and return them as a list of integers.
   The leading ESC is not included. Handles SS3, CSI, and OSC sequences."
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
;; Keyboard events
;; ---------------------------------------------------------------------------
(defun tui-lookup-csi-key (final param)
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

(defun tui-lookup-ss3-key (final)
  "Map an SS3 final byte to a key name. Returns nil if not recognized."
  (cond
    ((= final 65) "up")
    ((= final 66) "down")
    ((= final 67) "right")
    ((= final 68) "left")
    ((= final 70) "end")
    ((= final 72) "home")
    (#t nil)))

(defun tui-parse-keyboard-sequence (seq)
  "Parse a list of bytes after ESC into a keyboard event alist."
  (cond
    ((null? seq) (list (cons 'type 'key) (cons 'key "escape")))
    ((= (car seq) 79) ; SS3 (ESC O ...)
     (let* ((final (cadr seq))
            (name (tui-lookup-ss3-key final)))
       (if name
         (list (cons 'type 'key) (cons 'key name))
         (list (cons 'type 'key) (cons 'key (format nil "ss3-~A" final))))))
    ((= (car seq) 91) ; CSI (ESC [ ...)
     (let* ((body (cdr seq))
            (final (tui-last body))
            (digits (reverse (cdr (reverse body))))
            (param
             (if (null? digits)
               0
               (string->number (tui-integer-list->string digits))))
            (name (tui-lookup-csi-key final param)))
       (if name
         (list (cons 'type 'key) (cons 'key name))
         (list (cons 'type 'key)
          (cons 'key (format nil "csi-~A-~A" param final))))))
    (#t (list (cons 'type 'key) (cons 'key (format nil "esc-~A" (car seq)))))))

(defun tui-key-event? (ev)
  "Return #t if EV is a keyboard event."
  (and (pair? ev) (equal? (cdr (assoc 'type ev)) 'key)))

(defun tui-key-name (ev)
  "Return the key name of a keyboard event, or nil if not a key event."
  (and (tui-key-event? ev) (cdr (assoc 'key ev))))

;; ---------------------------------------------------------------------------
;; Mouse events (SGR extended protocol)
;; ---------------------------------------------------------------------------
(defun tui-enable-mouse ()
  "Enable mouse reporting."
  (princ "\x1b[?1003h\x1b[?1006h"))

(defun tui-disable-mouse ()
  "Disable mouse reporting."
  (princ "\x1b[?1003l\x1b[?1006l"))

(defun tui-parse-mouse-sequence (seq)
  "Parse an SGR mouse sequence (ESC [ < Cb ; Cx ; Cy (M|m)) into an event.
   SEQ is the bytes after ESC."
  (if
    (or (null? seq) (not (= (car seq) 91)) (< (length seq) 2)
        (not (= (cadr seq) 60)))
    (list (cons 'type 'unknown) (cons 'data seq))
    (let* ((str (tui-integer-list->string (cddr seq)))
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

(defun tui-mouse-event? (ev)
  "Return #t if EV is a mouse event."
  (and (pair? ev) (equal? (cdr (assoc 'type ev)) 'mouse)))

;; ---------------------------------------------------------------------------
;; Focus events
;; ---------------------------------------------------------------------------
(defun tui-enable-focus-events ()
  "Enable focus in/out events."
  (princ "\x1b[?1004h"))

(defun tui-disable-focus-events ()
  "Disable focus in/out events."
  (princ "\x1b[?1004l"))

(defun tui-parse-focus-sequence (seq)
  "Parse a focus event (ESC [ I or ESC [ O)."
  (if (null? seq)
    (list (cons 'type 'focus) (cons 'focus 'in))
    (let ((final (tui-last seq)))
      (if (= final 73)
        (list (cons 'type 'focus) (cons 'focus 'in))
        (list (cons 'type 'focus) (cons 'focus 'out))))))

(defun tui-focus-event? (ev)
  "Return #t if EV is a focus event."
  (and (pair? ev) (equal? (cdr (assoc 'type ev)) 'focus)))

;; ---------------------------------------------------------------------------
;; Bracketed paste
;; ---------------------------------------------------------------------------
(defun tui-enable-bracketed-paste ()
  "Enable bracketed paste mode."
  (princ "\x1b[?2004h"))

(defun tui-disable-bracketed-paste ()
  "Disable bracketed paste mode."
  (princ "\x1b[?2004l"))

(defun tui-paste-start? (seq)
  "Return #t if SEQ is the bracketed paste start marker ESC [ 2 0 0 ~."
  (and (not (null? seq)) (= (length seq) 5) (= (car seq) 91) (= (cadr seq) 50)
       (= (caddr seq) 48) (= (cadddr seq) 48) (= (tui-last seq) 126)))

(defun tui-paste-end? (seq)
  "Return #t if SEQ is the bracketed paste end marker ESC [ 2 0 1 ~."
  (and (not (null? seq)) (= (length seq) 5) (= (car seq) 91) (= (cadr seq) 50)
       (= (caddr seq) 48) (= (cadddr seq) 49) (= (tui-last seq) 126)))

;; ---------------------------------------------------------------------------
;; Clipboard integration (OSC 52)
;; ---------------------------------------------------------------------------
(defun tui-set-clipboard (text)
  "Set the system clipboard to TEXT using OSC 52."
  (princ "\x1b]52;c;")
  (princ text)
  (princ "\x1b\\"))

(defun tui-request-clipboard ()
  "Request the system clipboard contents via OSC 52."
  (princ "\x1b]52;c;?\x1b\\"))

;; ---------------------------------------------------------------------------
;; Main event reader
;; ---------------------------------------------------------------------------
(defun tui-read-event (read-byte-fn)
  "Read one terminal event using READ-BYTE-FN. Returns a keyboard event for
   printable input, a mouse/focus/paste marker for special sequences, or nil
   on timeout/EOF."
  (define b (read-byte-fn))
  (cond
    ((null? b) nil)
    ((= b 27) ; ESC
     (let ((seq (tui-read-escape-sequence read-byte-fn)))
       (cond
         ((tui-paste-start? seq) (list (cons 'type 'paste-start)))
         ((tui-paste-end? seq) (list (cons 'type 'paste-end)))
         ((and (not (null? seq)) (= (car seq) 91) (> (length seq) 1)
               (= (cadr seq) 60))
          (tui-parse-mouse-sequence seq))
         ((and (not (null? seq)) (= (car seq) 91) (> (length seq) 1))
          (let ((final (tui-last seq)))
            (if (or (= final 73) (= final 79))
              (tui-parse-focus-sequence seq)
              (tui-parse-keyboard-sequence seq))))
         (#t (tui-parse-keyboard-sequence seq)))))
    ((= b 13) (list (cons 'type 'key) (cons 'key "return"))) ; CR -> return
    ((= b 10) (list (cons 'type 'key) (cons 'key "newline"))) ; LF
    ((= b 9) (list (cons 'type 'key) (cons 'key "tab")))
    ((= b 127) (list (cons 'type 'key) (cons 'key "backspace")))
    ((< b 32) ; control character
     (list (cons 'type 'key)
      (cons 'key
       (concat "C-" (char->string (char-downcase (code-char (+ b 64)))))) (cons 'byte b)))
    (#t ; printable ASCII/Unicode byte -- return the byte as key
     (list (cons 'type 'key) (cons 'key (char->string (code-char b)))
      (cons 'byte b)))))

(provide 'tui-events)
