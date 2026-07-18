;; Character Type Tests
;; Tests for the LISP_CHAR type, reader syntax, and character operations
(load "tests/test-helpers.lisp")

;; ===========================================
;; Character Literals
;; ===========================================
;; Simple ASCII characters
(assert-equal #\a #\a "char literal #\\a")
(assert-equal #\Z #\Z "char literal #\\Z")
(assert-equal #\0 #\0 "char literal #\\0")
;; Named characters
(assert-equal (code-char 32) #\space "named char #\\space")
(assert-equal (code-char 10) #\newline "named char #\\newline")
(assert-equal (code-char 9) #\tab "named char #\\tab")
(assert-equal (code-char 13) #\return "named char #\\return")
(assert-equal (code-char 27) #\escape "named char #\\escape")
(assert-equal (code-char 0) #\null "named char #\\null")
(assert-equal (code-char 8) #\backspace "named char #\\backspace")
(assert-equal (code-char 127) #\delete "named char #\\delete")
;; Hex notation
(assert-equal #\A #\A "hex char #\\x41 = A")
(assert-equal #\a #\a "hex char #\\x61 = a")
(assert-equal #\escape #\escape "hex char #\\x1b = escape")
;; Unicode notation
(assert-equal (code-char 19990) #\u4e16 "unicode char #\\u4e16")
(assert-equal 19990 (char-code #\u4e16) "unicode char code")

;; ===========================================
;; Type Predicate
;; ===========================================
(assert-true (char? #\a) "char? on character")

(assert-false (char? "a") "char? on string")
(assert-false (char? 97) "char? on integer")
(assert-false (char? nil) "char? on nil")

;; ===========================================
;; Character/Code Conversion
;; ===========================================
(assert-equal 97 (char-code #\a) "char-code #\\a")
(assert-equal 65 (char-code #\A) "char-code #\\A")
(assert-equal 32 (char-code #\space) "char-code #\\space")
(assert-equal 10 (char-code #\newline) "char-code #\\newline")
(assert-equal #\a (code-char 97) "code-char 97")
(assert-equal #\A (code-char 65) "code-char 65")
(assert-equal #\space (code-char 32) "code-char 32")
(assert-equal #\null (code-char 0) "code-char 0")
;; ===========================================
;; String/Character Conversion
;; ===========================================
(assert-equal "a" (char->string #\a) "char->string #\\a")
(assert-equal " " (char->string #\space) "char->string #\\space")
(assert-equal "\n" (char->string #\newline) "char->string #\\newline")
(assert-equal #\a (string->char "a") "string->char \"a\"")
(assert-equal #\space (string->char " ") "string->char \" \"")
(assert-equal #\newline (string->char "\n") "string->char newline")

;; ===========================================
;; Character Comparisons
;; ===========================================
(assert-true (char=? #\a #\a) "char=? equal")

(assert-false (char=? #\a #\b) "char=? not equal")

(assert-true (char<? #\a #\b) "char<? a < b")

(assert-false (char<? #\b #\a) "char<? b < a")
(assert-false (char<? #\a #\a) "char<? a < a")

(assert-true (char>? #\b #\a) "char>? b > a")

(assert-false (char>? #\a #\b) "char>? a > b")
(assert-false (char>? #\a #\a) "char>? a > a")

(assert-true (char<=? #\a #\b) "char<=? a <= b")
(assert-true (char<=? #\a #\a) "char<=? a <= a")

(assert-false (char<=? #\b #\a) "char<=? b <= a")

(assert-true (char>=? #\b #\a) "char>=? b >= a")
(assert-true (char>=? #\a #\a) "char>=? a >= a")

(assert-false (char>=? #\a #\b) "char>=? a >= b")

;; ===========================================
;; Case Conversion
;; ===========================================
(assert-equal #\A (char-upcase #\a) "char-upcase #\\a")
(assert-equal #\Z (char-upcase #\z) "char-upcase #\\z")
(assert-equal #\A (char-upcase #\A) "char-upcase #\\A already upper")
(assert-equal #\1 (char-upcase #\1) "char-upcase #\\1 unchanged")
(assert-equal #\a (char-downcase #\A) "char-downcase #\\A")
(assert-equal #\z (char-downcase #\Z) "char-downcase #\\Z")
(assert-equal #\a (char-downcase #\a) "char-downcase #\\a already lower")
(assert-equal #\1 (char-downcase #\1) "char-downcase #\\1 unchanged")

;; ===========================================
;; Character Classification
;; ===========================================
(assert-true (char-alphabetic? #\a) "char-alphabetic? #\\a")
(assert-true (char-alphabetic? #\Z) "char-alphabetic? #\\Z")

(assert-false (char-alphabetic? #\1) "char-alphabetic? #\\1")
(assert-false (char-alphabetic? #\space) "char-alphabetic? #\\space")

(assert-true (char-numeric? #\0) "char-numeric? #\\0")
(assert-true (char-numeric? #\9) "char-numeric? #\\9")

(assert-false (char-numeric? #\a) "char-numeric? #\\a")
(assert-false (char-numeric? #\space) "char-numeric? #\\space")

(assert-true (char-whitespace? #\space) "char-whitespace? #\\space")
(assert-true (char-whitespace? #\tab) "char-whitespace? #\\tab")
(assert-true (char-whitespace? #\newline) "char-whitespace? #\\newline")

(assert-false (char-whitespace? #\a) "char-whitespace? #\\a")
(assert-false (char-whitespace? #\0) "char-whitespace? #\\0")

;; ===========================================
;; string-ref returns character
;; ===========================================
(assert-true (char? (string-ref "hello" 0)) "string-ref returns char")

(assert-equal #\h (string-ref "hello" 0) "string-ref first char")
(assert-equal #\o (string-ref "hello" 4) "string-ref last char")
(assert-equal #\a (string-ref "a" 0) "string-ref single char string")
;; ===========================================
;; string-append alias for concat
;; ===========================================
(assert-equal "hello world" (string-append "hello" " " "world") "string-append")
(assert-equal "abc" (string-append "a" "b" "c") "string-append multiple")
(assert-equal "" (string-append) "string-append empty")
