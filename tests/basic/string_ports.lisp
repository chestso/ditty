;; String Port Tests
;; Tests for string ports providing efficient character-by-character reading
(load "tests/test-helpers.lisp")

;; ===========================================
;; Creating String Ports
;; ===========================================
(define port (open-input-string "hello"))

(assert-true (string-port? port) "string-port? on port")

(assert-false (string-port? "hello") "string-port? on string")
(assert-false (string-port? nil) "string-port? on nil")
(assert-false (string-port? 42) "string-port? on number")

;; ===========================================
;; Reading Characters
;; ===========================================
(define p1 (open-input-string "abc"))

(assert-equal #\a (port-read-char p1) "port-read-char first")
(assert-equal #\b (port-read-char p1) "port-read-char second")
(assert-equal #\c (port-read-char p1) "port-read-char third")

(assert-nil (port-read-char p1) "port-read-char at EOF")
(assert-nil (port-read-char p1) "port-read-char past EOF")

;; ===========================================
;; Peeking Characters
;; ===========================================
(define p2 (open-input-string "xyz"))

(assert-equal #\x (port-peek-char p2) "port-peek-char first")
(assert-equal #\x (port-peek-char p2) "port-peek-char same again")
(assert-equal #\x (port-read-char p2) "port-read-char after peek")
(assert-equal #\y (port-peek-char p2) "port-peek-char second")

;; ===========================================
;; Position Tracking
;; ===========================================
(define p3 (open-input-string "test"))

(assert-equal 0 (port-position p3) "port-position at start")

(port-read-char p3)

(assert-equal 1 (port-position p3) "port-position after one read")

(port-read-char p3)
(port-read-char p3)

(assert-equal 3 (port-position p3) "port-position after three reads")

(port-peek-char p3) ; peek doesn't advance

(assert-equal 3 (port-position p3) "port-position unchanged after peek")

;; ===========================================
;; Source String
;; ===========================================
(define p4 (open-input-string "source"))

(assert-equal "source" (port-source p4) "port-source at start")

(port-read-char p4)
(port-read-char p4)

(assert-equal "source" (port-source p4) "port-source unchanged after reads")

;; ===========================================
;; EOF Detection
;; ===========================================
(define p5 (open-input-string "ab"))

(assert-false (port-eof? p5) "port-eof? at start")

(port-read-char p5)

(assert-false (port-eof? p5) "port-eof? after first read")

(port-read-char p5)

(assert-true (port-eof? p5) "port-eof? at end")

(port-read-char p5) ; read past EOF

(assert-true (port-eof? p5) "port-eof? past end")

;; ===========================================
;; Empty String
;; ===========================================
(define p6 (open-input-string ""))

(assert-true (port-eof? p6) "port-eof? on empty string")

(assert-nil (port-read-char p6) "port-read-char on empty string")
(assert-nil (port-peek-char p6) "port-peek-char on empty string")

(assert-equal 0 (port-position p6) "port-position on empty string")
(assert-equal "" (port-source p6) "port-source on empty string")

;; ===========================================
;; Multi-byte UTF-8 Characters
;; ===========================================
(define p7 (open-input-string "日本語"))

(assert-equal #\u65e5 (port-read-char p7) "port-read-char UTF-8 first")
(assert-equal 1 (port-position p7) "port-position counts chars not bytes")
(assert-equal #\u672c (port-read-char p7) "port-read-char UTF-8 second")
(assert-equal #\u8a9e (port-read-char p7) "port-read-char UTF-8 third")

(assert-true (port-eof? p7) "port-eof? after UTF-8")

;; ===========================================
;; Mixed ASCII and UTF-8
;; ===========================================
(define p8 (open-input-string "a日b"))

(assert-equal #\a (port-read-char p8) "mixed: ASCII first")
(assert-equal #\u65e5 (port-read-char p8) "mixed: UTF-8 middle")
(assert-equal #\b (port-read-char p8) "mixed: ASCII last")
(assert-equal 3 (port-position p8) "mixed: position is char count")

;; ===========================================
;; Whitespace and Special Characters
;; ===========================================
(define p9 (open-input-string "a b\tc\n"))

(assert-equal #\a (port-read-char p9) "special: first char")
(assert-equal #\space (port-read-char p9) "special: space")
(assert-equal #\b (port-read-char p9) "special: after space")
(assert-equal #\tab (port-read-char p9) "special: tab")
(assert-equal #\c (port-read-char p9) "special: after tab")
(assert-equal #\newline (port-read-char p9) "special: newline")

;; ===========================================
;; Output String Ports
;; ===========================================
(define op1 (open-output-string))

(assert-true (string-port? op1) "output port is a string-port")

(assert-equal "" (get-output-string op1) "fresh output port is empty")

(port-write-string op1 "hello")

(assert-equal "hello" (get-output-string op1) "write-string appends")

(port-write-string op1 ", world")

(assert-equal "hello, world" (get-output-string op1) "write-string accumulates")

(port-write-char op1 #\!)

(assert-equal "hello, world!" (get-output-string op1) "write-char appends")

;; Growth beyond the initial 64-byte capacity
(define op2 (open-output-string))

(do ((i 0 (+ i 1))) ((>= i 100))
  (port-write-string op2 "0123456789"))

(assert-equal 1000 (length (get-output-string op2))
 "output port grows past initial capacity")

;; Multi-byte UTF-8 output
(define op3 (open-output-string))

(port-write-string op3 "日本")
(port-write-string op3 "go")

(assert-equal "日本go" (get-output-string op3) "output port handles UTF-8")
(assert-equal 4 (length (get-output-string op3))
 "UTF-8 char count tracked, not bytes")

;; with-output-to-string sugar
(assert-equal
 (with-output-to-string (p)
  (port-write-string p "a")
  (port-write-char p #\b)
  (port-write-string p "c"))
 "abc"
 "with-output-to-string returns accumulated text")

;; Type guards: writing to an input port errors
(assert-error (port-write-string (open-input-string "x") "y")
 "port-write-string rejects input ports")
(assert-error (get-output-string (open-input-string "x"))
 "get-output-string rejects input ports")
