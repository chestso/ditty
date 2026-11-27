;; String escape sequences in the reader.
;; Single-character escapes, octal (1-3 digits), and fixed-width hex
;; (\xHH, \uXXXX, \UXXXXXXXX). All numeric escapes denote Unicode
;; codepoints, encoded as UTF-8. Unknown or malformed escapes signal
;; reader-error; an escape truncated at end of input is unclosed-input.
(load "tests/test-helpers.lisp")

;;; Single-character escapes
(assert-equal "\n" (char->string #\newline) "\\n is newline")
(assert-equal "\t" (char->string #\tab) "\\t is tab")
(assert-equal "\r" (char->string #\return) "\\r is return")
(assert-equal "\a" (char->string (code-char 7)) "\\a is alarm")
(assert-equal "\b" (char->string (code-char 8)) "\\b is backspace")
(assert-equal "\v" (char->string (code-char 11)) "\\v is vertical tab")
(assert-equal "\f" (char->string (code-char 12)) "\\f is form feed")
(assert-equal "\e" (char->string #\escape) "\\e is escape")
(assert-equal "\\" (char->string #\\) "\\\\ is backslash")
(assert-equal "\"" (char->string #\") "\\\" is double quote")

;;; Octal escapes: 1-3 digits, greedy up to three
(assert-equal "\033" (char->string #\escape) "\\033 is escape")
(assert-equal "\101" "A" "\\101 is A")
(assert-equal "\7" (char->string (code-char 7)) "single octal digit")
(assert-equal "\12" (char->string #\newline) "two octal digits")
(assert-equal "\1234" "S4" "octal stops after three digits")

;;; Hex escapes: fixed width, codepoint semantics
(assert-equal "\x41" "A" "\\x41 is A")
(assert-equal "\x41bc" "Abc" "\\x takes exactly two hex digits")
(assert-equal "\x1b" (char->string #\escape) "\\x1b is escape")
(assert-equal (string-length "\xff") 1 "\\xff is one codepoint")
(assert-equal (string-ref "\xff" 0) (code-char 255) "\\xff is U+00FF")
(assert-equal "\u0041bc" "Abc" "\\u takes exactly four hex digits")
(assert-equal "\u4e16" "世" "\\u4e16 is CJK 世")
(assert-equal "\u20ac" "€" "\\u20ac is euro sign")
(assert-equal "\U00000041" "A" "\\U takes exactly eight hex digits")
(assert-equal "\U0001F600" "😀" "\\U0001F600 is grinning face")
(assert-equal (string-length "\U0001F600") 1 "non-BMP escape is one codepoint")
(assert-equal (string-length "\x1b[?1003h") 8 "terminal control string")

;;; Malformed escapes signal reader-error, not silent text. The bad
;;; source cannot appear literally here (the reader would reject this
;;; file), so it goes through read-from-string.
(assert-equal 'caught
 (condition-case err (read-from-string "\"\\q\"")
   (reader-error 'caught))
 "unknown escape signals reader-error")
(assert-equal 'caught
 (condition-case err (read-from-string "\"\\x1\"")
   (unclosed-input 'wrong)
   (reader-error 'caught))
 "short hex escape is reader-error, not unclosed-input")
(assert-equal 'caught
 (condition-case err (read-from-string "\"\\u12x4\"")
   (reader-error 'caught))
 "non-hex digit inside \\u signals reader-error")
(assert-equal 'caught
 (condition-case err (read-from-string "\"\\ud800\"")
   (reader-error 'caught))
 "surrogate codepoint signals reader-error")
(assert-equal 'caught
 (condition-case err (read-from-string "\"\\U00110000\"")
   (reader-error 'caught))
 "codepoint beyond U+10FFFF signals reader-error")
