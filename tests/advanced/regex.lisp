;; Regular Expression Operations
;; Demonstration of PCRE2 regex pattern matching and manipulation
(load "tests/test-helpers.lisp")

;; ===========================================
;; Basic Regex Matching
;; ===========================================
;; Test digit matching
(assert-true (regex-match? "\\d+" "hello123") "regex-match? digits")
;; Test character class matching
(assert-true (regex-match? "^[a-z]+$" "hello") "regex-match? character class")
;; Simple substring matching
(assert-true (regex-match? "test" "this is a test") "regex-match? substring")

;; ===========================================
;; Finding Matches
;; ===========================================
;; Find first match
(assert-equal "123" (regex-find "\\d+" "abc123def") "regex-find first match")
;; Find all matches
(assert-equal '("1" "2" "3") (regex-find-all "\\d+" "a1b2c3")
 "regex-find-all digits")
;; Find all words
(assert-equal
 '("hello" "world" "test") (regex-find-all "\\w+" "hello world test")
 "regex-find-all words")
;; ===========================================
;; Extracting Capture Groups
;; ===========================================
;; Extract email username and domain
(assert-equal '("user" "domain") (regex-extract "(\\w+)@(\\w+)" "user@domain")
 "regex-extract email parts")
;; Extract date components (YYYY-MM-DD)
(assert-equal
 '("2025" "10" "24") (regex-extract "(\\d+)-(\\d+)-(\\d+)" "2025-10-24")
 "regex-extract date parts")
;; Extract code parts
(assert-equal '("ABC" "123") (regex-extract "([A-Z]+)-(\\d+)" "ABC-123")
 "regex-extract code parts")
;; ===========================================
;; Replacing Text
;; ===========================================
;; Replace all occurrences (replaces all by default)
(assert-equal "aXbXcX" (regex-replace "\\d+" "a1b2c3" "X")
 "regex-replace all digits")
;; Replace all occurrences explicitly
(assert-equal "aXbXcX" (regex-replace-all "\\d+" "a1b2c3" "X")
 "regex-replace-all digits")
;; Replace with capture groups - swap email parts
(assert-equal
 "domain@user" (regex-replace "(\\w+)@(\\w+)" "user@domain" "$2@$1")
 "regex-replace swap email parts")
;; Replace with capture groups - change date format
(assert-equal "10/2025" (regex-replace "(\\d+)-(\\d+)" "2025-10" "$2/$1")
 "regex-replace date format")
;; ===========================================
;; Replacing - Partial Match Tests
;; ===========================================
;; Replace only matched portion (trailing comma)
(assert-equal "helloX" (regex-replace ",$" "hello," "X")
 "regex-replace trailing comma")
;; Remove trailing punctuation (empty replacement)
(assert-equal "b" (regex-replace "a+$" "baaa" "")
 "regex-replace remove trailing")
;; Remove leading punctuation
(assert-equal "b" (regex-replace "^a+" "aaab" "")
 "regex-replace remove leading")
;; ===========================================
;; Splitting Strings
;; ===========================================
;; Split by whitespace
(assert-equal
 '("hello" "world" "test") (regex-split "\\s+" "hello  world  test")
 "regex-split by whitespace")
;; Split by comma
(assert-equal
 '("apple" "banana" "cherry") (regex-split "," "apple,banana,cherry")
 "regex-split by comma")
;; Split by multiple delimiters
(assert-equal '("a" "b" "c" "d") (regex-split "[,;]" "a,b;c,d")
 "regex-split by multiple delimiters")
;; ===========================================
;; Utility Functions
;; ===========================================
;; Escape special regex characters
(assert-equal
 "test\\.string\\*with\\?special" (regex-escape "test.string*with?special")
 "regex-escape special characters")

;; Check if regex pattern is valid
(assert-true (regex-valid? "\\d+") "regex-valid? valid pattern")

(assert-false (regex-valid? "[invalid") "regex-valid? invalid pattern")

;; ===========================================
;; Enhanced Wildcard Matching (using regex-match? for pattern matching)
;; ===========================================
;; Character classes - match any of a, b, or c
(assert-true (regex-match? "[abc]test" "atest") "regex-match? character class")
;; Character ranges - match lowercase
(assert-true (regex-match? "[a-z]+" "hello") "regex-match? character range")
;; Negated classes - match non-digits
(assert-true (regex-match? "[^0-9]+" "abc") "regex-match? negated class")
;; Wildcards - .* matches zero or more
(assert-true (regex-match? "test.*" "test123") "regex-match? asterisk wildcard")
;; Wildcards - . matches exactly one
(assert-true (regex-match? "test." "test1") "regex-match? question wildcard")

;; ===========================================
;; Practical Examples
;; ===========================================
;; Email validation
(define email-pattern "^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$")

(assert-true (regex-match? email-pattern "user@example.com")
 "email validation valid")

(assert-false (regex-match? email-pattern "invalid.email")
 "email validation invalid")

;; Extract phone numbers
(assert-equal
 '("555-1234" "555-5678")
 (regex-find-all "\\d{3}-\\d{4}" "Call 555-1234 or 555-5678")
 "extract phone numbers")
;; URL parsing
(assert-equal
 (regex-extract "(https?)://([^/]+)(/.*)?" "https://example.com/path")
 '("https" "example.com" "/path") "URL parsing")
;; Extract all numbers from text
(assert-equal
 '("3" "5") (regex-find-all "\\d+" "I have 3 apples and 5 oranges")
 "extract numbers")
;; Clean multiple whitespace
(assert-equal
 "hello world test" (regex-replace-all "\\s+" "hello    world   test" " ")
 "clean multiple whitespace")
;; Extract capitalized words
(assert-equal
 '("Hello" "World" "Test") (regex-find-all "[A-Z][a-z]+" "Hello World Test")
 "extract capitalized words")

;; Password validation (8+ chars, alphanumeric)
(define password-pattern "^(?=.*[A-Za-z])(?=.*\\d)[A-Za-z\\d]{8,}$")

(assert-true (regex-match? password-pattern "Password123")
 "password validation strong")

(assert-false (regex-match? password-pattern "weak") "password validation weak")

;; Extract hashtags
(assert-equal
 '("#lisp" "#programming")
 (regex-find-all "#\\w+" "Check out #lisp and #programming!")
 "extract hashtags")
;; Remove HTML tags
(assert-equal
 "Hello World" (regex-replace-all "<[^>]+>" "<p>Hello <b>World</b></p>" "")
 "remove HTML tags")
;; Extract IPv4 addresses
(assert-equal
 (regex-find-all "\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}"
  "Server at 192.168.1.1 and 10.0.0.1")
 '("192.168.1.1" "10.0.0.1") "extract IPv4 addresses")

;; ===========================================
;; Compiled regex objects (regex-compile)
;; ===========================================
(define digits-re (regex-compile "\\d+"))

(assert-true (regex? digits-re) "regex? on compiled regex")

(assert-false (regex? "\\d+") "regex? rejects pattern strings")
(assert-false (regex? 42) "regex? rejects non-strings")

(assert-true (regex-match? digits-re "abc123") "compiled: regex-match?")

(assert-nil (regex-match? digits-re "abc") "compiled: regex-match? no match")

(assert-equal "123" (regex-find digits-re "abc123def") "compiled: regex-find")
(assert-equal '("1" "22" "333") (regex-find-all digits-re "a1b22c333")
 "compiled: regex-find-all")
(assert-equal "aXbXcX" (regex-replace digits-re "a1b2c3" "X")
 "compiled: regex-replace")
(assert-equal "aXbXcX" (regex-replace-all digits-re "a1b2c3" "X")
 "compiled: regex-replace-all")
(assert-equal '("a" "b" "c") (regex-split digits-re "a1b2c")
 "compiled: regex-split")

(assert-true (regex-valid? digits-re) "regex-valid? on compiled regex")

;; Capture groups via compiled regex
(define email-re (regex-compile "(\\w+)@(\\w+)"))

(assert-equal '("user" "host") (regex-extract email-re "user@host")
 "compiled: regex-extract")

;; Reuse: same compiled regex across many calls
(define re-word (regex-compile "\\w+"))

(assert-equal "hello" (regex-find re-word "hello world") "reuse 1")
(assert-equal "second" (regex-find re-word "second") "reuse 2")
(assert-equal '("a" "b" "c" "d") (regex-find-all re-word "a b c d") "reuse 3")

;; Bad pattern surfaces error at compile time
(assert-error (regex-compile "[unclosed") "regex-compile rejects bad pattern")

;; Type errors
(assert-error (regex-compile 42) "regex-compile rejects non-strings")
(assert-error (regex-match? 42 "abc")
 "regex-match? rejects non-string non-regex pattern")
