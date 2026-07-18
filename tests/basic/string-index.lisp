;; Test string-index builtin function
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; Basic tests
(assert-equal 6 (string-index "hello world" "world")
 "find 'world' in 'hello world'")
(assert-equal 0 (string-index "hello world" "hello") "find 'hello' at start")
(assert-equal 4 (string-index "hello world" "o") "find first 'o'")
(assert-equal 5 (string-index "hello world" " ") "find space character")

;; Not found
(assert-nil (string-index "hello world" "xyz")
 "substring not found returns nil")
(assert-nil (string-index "hello world" "WORLD") "case-sensitive search fails")
;; Empty strings
(assert-nil (string-index "" "x") "search in empty string returns nil")

(assert-equal 0 (string-index "hello" "") "empty needle returns 0")
;; UTF-8 support
(assert-equal 7 (string-index "Hello, 世界!" "世") "find UTF-8 character")
(assert-equal 9 (string-index "Hello, 世界!" "!") "find character after UTF-8")
(assert-equal 1 (string-index "🌍🌎🌏" "🌎") "find emoji in emoji string")
;; Multi-character needles
(assert-equal 2 (string-index "abcdef" "cd") "find two-character substring")
(assert-equal 2 (string-index "abcdef" "cde") "find three-character substring")
(assert-equal 3 (string-index "abcdef" "def") "find substring at end")

;; Case sensitivity
(assert-nil (string-index "Hello" "hello")
 "case mismatch uppercase to lowercase")
(assert-nil (string-index "hello" "Hello")
 "case mismatch lowercase to uppercase")
