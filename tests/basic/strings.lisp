;; Advanced string operations
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; ===========================================
;; String Type Checking (regression test for string? predicate)
;; ===========================================
(assert-true (string? "hello") "string? recognizes non-empty string")
(assert-true (string? "") "string? recognizes empty string")

(assert-nil (string? 42) "string? rejects integer")
(assert-nil (string? nil) "string? rejects nil")
(assert-nil (string? #(1 2 3)) "string? rejects vector")
(assert-nil (string? '(a b c)) "string? rejects list")

;; Split test
(assert-equal '("apple" "banana" "cherry") (split "apple-banana-cherry" "-")
 "split string by delimiter")
(assert-equal '("a" "b" "c") (string-split "a,b,c" ",")
 "string-split alias works")
;; Join test
(assert-equal "a,b,c" (join '("a" "b" "c") ",") "join strings with comma")
(assert-equal "hello world" (join '("hello" "world") " ")
 "join strings with space")
(assert-equal "one" (join '("one") "-") "join single element list")
(assert-equal "" (join 'nil ",") "join empty list returns empty string")
(assert-equal "x-y-z" (string-join '("x" "y" "z") "-")
 "string-join alias works")

;; String contains
(assert-true (string-contains? "hello world" "world")
 "string-contains? finds substring")
;; String match with wildcards
(assert-true (string-match? "hello" "h*o")
 "string-match? with wildcard pattern")
;; String prefix
(assert-true (string-prefix? "hel" "hello") "string-prefix? matches prefix")
(assert-true (string-prefix? "lis" "lisp")
 "string-prefix? matches 'lis' in 'lisp'")

(assert-false (string-prefix? "xyz" "hello")
 "string-prefix? rejects non-matching prefix")

;; String transformations
(assert-equal
 "hello universe" (string-replace "hello world" "world" "universe")
 "string-replace changes substring")
(assert-equal "heLLo" (string-replace "hello" "l" "L")
 "string-replace changes all occurrences")
(assert-equal "barbarbar" (string-replace "foofoofoo" "foo" "bar")
 "string-replace multiple occurrences")
(assert-equal "y" (string-replace "x" "x" "y")
 "string-replace single character")
(assert-equal "abc" (string-replace "abc" "x" "y")
 "string-replace with no match returns original")
;; String case conversion
(assert-equal "HELLO WORLD" (string-upcase "hello world")
 "string-upcase converts to uppercase")
(assert-equal "HELLO WORLD" (string-upcase "Hello World")
 "string-upcase on mixed case")
(assert-equal "123ABC" (string-upcase "123abc")
 "string-upcase preserves numbers")
(assert-equal "hello world" (string-downcase "HELLO WORLD")
 "string-downcase converts to lowercase")
(assert-equal "hello world" (string-downcase "Hello World")
 "string-downcase on mixed case")
(assert-equal "123abc" (string-downcase "123ABC")
 "string-downcase preserves numbers")

;; String comparisons
(assert-true (string<? "abc" "def") "string<? compares strings")
