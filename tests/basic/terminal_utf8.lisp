(load "tests/test-helpers.lisp")

(require 'terminal)

(use-package 'terminal)

;; ---------------------------------------------------------------------------
;; Decoder test helpers
;; ---------------------------------------------------------------------------
;; terminal-decoder-next is internal, reached through the package prefix like
;; the other terminal internals.  It returns a code point, or nil when no
;; byte is available right now (the decoder keeps its state).
(defun decode-with (dec bytes)
  "Feed BYTES to DEC and return the code points produced, in order."
  (let ((remaining bytes)
        (out nil))
    (let loop ()
      (let ((cp
             (terminal:terminal-decoder-next dec
              (lambda ()
                (if (null? remaining)
                  nil
                  (let ((b (car remaining)))
                    (set! remaining (cdr remaining))
                    b))))))
        (if (null? cp)
          (reverse out)
          (progn (set! out (cons cp out)) (loop)))))))

(defun decode-all (bytes)
  (decode-with (terminal-make-decoder) bytes))

;; ---------------------------------------------------------------------------
;; Unicode Table 3-7 vectors
;; ---------------------------------------------------------------------------
(define fffd #xFFFD)

(define decoder-vectors
  (list
   (list (quote ()) '())
   (list '(#x61) '(#x61))
   (list '(#xC3 #xA4) '(#xE4))
   (list '(#xE2 #x82 #xAC) '(#x20AC))
   (list '(#xF0 #x9F #x98 #x80) '(#x1F600))
   (list '(#xC2 #x80) '(#x80))
   (list '(#xDF #xBF) '(#x7FF))
   (list '(#xE0 #xA0 #x80) '(#x800))
   (list '(#xEF #xBF #xBF) '(#xFFFF))
   (list '(#xF0 #x90 #x80 #x80) '(#x10000))
   (list '(#xF4 #x8F #xBF #xBF) '(#x10FFFF))
   (list '(#x80) (list fffd))
   (list '(#xC0 #x80) (list fffd fffd))
   (list '(#xC1 #xBF) (list fffd fffd))
   (list '(#xE0 #x80 #x80) (list fffd fffd fffd))
   (list '(#xED #xA0 #x80) (list fffd fffd fffd))
   (list '(#xF4 #x90 #x80 #x80) (list fffd fffd fffd fffd))
   (list '(#xF5 #x80 #x80 #x80) (list fffd fffd fffd fffd))
   (list '(#xFF) (list fffd))
   (list '(#xE2 #x82 #x41) (list fffd #x41))
   (list '(#xC3 #xC3 #xA4) (list fffd #xE4))
   (list '(#xF0 #x9F #x41) (list fffd #x41))))

(defun decoder-vector-name (bytes)
  (apply concat (map (lambda (b) (format nil "~A " b)) bytes)))

(define (run-decoder-vector bytes expected)
  (assert-equal expected (decode-all bytes)
   (concat "decoder bytes " (decoder-vector-name bytes))))

(map (lambda (v) (run-decoder-vector (car v) (cadr v))) decoder-vectors)

;; A truncated character produces no code point and keeps its state.
(define partial-dec (terminal-make-decoder))

(assert-equal '() (decode-with partial-dec '(#xE2 #x82))
 "a truncated 3-byte sequence yields no code point")
(assert-equal '(#x20AC) (decode-with partial-dec '(#xAC))
 "the decoder keeps state across two batches")

;; ---------------------------------------------------------------------------
;; Split property: every vector, every split offset
;; ---------------------------------------------------------------------------
(defun split-at (lst n)
  (if (= n 0)
    (list '() lst)
    (let ((head (list (car lst)))
          (rest (cdr lst)))
      (let loop ((i 1)
                 (head head)
                 (rest rest))
        (if (or (= i n) (null? rest))
          (list head rest)
          (loop (+ i 1) (append head (list (car rest))) (cdr rest)))))))

(defun run-split (bytes expected)
  (let ((n (length bytes)))
    (let loop ((i 0))
      (when (<= i n)
        (let* ((split (split-at bytes i))
               (head (car split))
               (tail (cadr split))
               (dec (terminal-make-decoder))
               (got (append (decode-with dec head) (decode-with dec tail))))
          (assert-equal expected got
           (concat "split at " (format nil "~A" i) " of "
            (decoder-vector-name bytes))))
        (loop (+ i 1))))))

(map (lambda (v) (run-split (car v) (cadr v))) decoder-vectors)

;; ---------------------------------------------------------------------------
;; terminal-read-event with an explicit decoder
;; ---------------------------------------------------------------------------
(define ev-dec (terminal-make-decoder))

(defun reader-over (bytes)
  (let ((remaining bytes))
    (lambda ()
      (if (null? remaining)
        nil
        (let ((b (car remaining)))
          (set! remaining (cdr remaining))
          b)))))

(assert-nil (terminal-read-event (reader-over '(#xE2)) ev-dec)
 "a partial character yields no event")

(define euro (terminal-read-event (reader-over '(#x82 #xAC)) ev-dec))

(assert-equal 8364 (terminal-key-code euro)
 "the next read completes the held character")
(assert-equal "€" (terminal-key-name euro)
 "the completed character is the key name")
(assert-equal "€" (terminal-key-text euro)
 "the completed character is typed text")

;; The default decoder is fresh per call: a truncated character is dropped.
(assert-nil (terminal-read-event (reader-over '(#xE2 #x82)))
 "a fresh default decoder does not invent an event for a truncated character")
