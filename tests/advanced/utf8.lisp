;; UTF-8 String Operations
;; Demonstrates character-based (not byte-based) string operations with Unicode
;; The language handles multi-byte UTF-8 sequences correctly
(load "tests/test-helpers.lisp")

;; Test string with mixed scripts: ASCII, Chinese, emoji
(define test_str "Hello, 世界! 🌍")

;; String length counts actual characters, not bytes
;; H-e-l-l-o-,- -世-界-!- -🌍 = 12 characters
(assert-equal 12 (length test_str) "UTF-8 string length")
;; Extract substring by character index (not byte index)
(assert-equal "世界" (substring test_str 7 9) "substring with Chinese characters")
;; Get character at specific index (returns character type)
(assert-equal #\H (string-ref test_str 0) "string-ref ASCII character")
(assert-equal (code-char 19990) (string-ref test_str 7)
 "string-ref Chinese character")

;; Test with emoji (composed of multiple UTF-8 bytes)
(define emoji_str "Hello 🚀 World")

(assert-equal 13 (length emoji_str) "string length with emoji")
;; Extract just the emoji by character index
(assert-equal "🚀" (substring emoji_str 6 7) "substring with emoji")
;; String concatenation works correctly with UTF-8
(assert-equal "Hello, 世界!" (concat "Hello, " "世界!") "concat with UTF-8")
;; Verify substring with Japanese characters
(assert-equal "こ" (substring "こんにちは" 0 1) "substring with Japanese characters")
;; String transformations with UTF-8
;; Note: Case conversion only works for ASCII characters, Unicode is preserved
(assert-equal "HELLO 世界" (string-upcase "hello 世界") "upcase preserves Unicode")
(assert-equal "hello 世界" (string-downcase "HELLO 世界")
 "downcase preserves Unicode")
(assert-equal "hello universe" (string-replace "hello 世界" "世界" "universe")
 "replace Chinese characters")
(assert-equal "hellO 世界" (string-replace "hello 世界" "o" "O")
 "replace ASCII in UTF-8 string")
