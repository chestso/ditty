;; String to Number Conversion Tests
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; Basic Positive Integers
(assert-equal 0 (string->number "0") "parse '0'")
(assert-equal 1 (string->number "1") "parse '1'")
(assert-equal 42 (string->number "42") "parse '42'")
(assert-equal 12345 (string->number "12345") "parse '12345'")
;; Negative Integers
(assert-equal -1 (string->number "-1") "parse '-1'")
(assert-equal -42 (string->number "-42") "parse '-42'")
(assert-equal -12345 (string->number "-12345") "parse '-12345'")
;; With Plus Sign
(assert-equal 1 (string->number "+1") "parse '+1'")
(assert-equal 42 (string->number "+42") "parse '+42'")
;; Leading/Trailing Whitespace
(assert-equal 42 (string->number "  42") "parse with leading whitespace")
(assert-equal 42 (string->number "42  ") "parse with trailing whitespace")
(assert-equal 42 (string->number "  42  ")
 "parse with both leading and trailing whitespace")
(assert-equal -42 (string->number "  -42  ") "parse negative with whitespace")
;; Edge Cases
(assert-equal 0 (string->number "0") "parse zero")
(assert-equal 0 (string->number "-0") "parse negative zero")
;; Use with arithmetic
(assert-equal 30 (+ (string->number "10") (string->number "20"))
 "add parsed numbers")
(assert-equal 15 (* (string->number "5") (string->number "3"))
 "multiply parsed numbers")
;; =============================================================================
;; R7RS Compliance Tests - Radix Parameter
;; =============================================================================
;; Binary (base 2)
(assert-equal 10 (string->number "1010" 2) "parse binary '1010'")
(assert-equal 255 (string->number "11111111" 2) "parse binary '11111111'")
(assert-equal -10 (string->number "-1010" 2) "parse negative binary")
;; Octal (base 8)
(assert-equal 63 (string->number "77" 8) "parse octal '77'")
(assert-equal 255 (string->number "377" 8) "parse octal '377'")
(assert-equal -63 (string->number "-77" 8) "parse negative octal")
;; Hexadecimal (base 16)
(assert-equal 255 (string->number "ff" 16) "parse hex 'ff'")
(assert-equal 255 (string->number "FF" 16) "parse hex 'FF' uppercase")
(assert-equal 16 (string->number "10" 16) "parse hex '10'")
(assert-equal -255 (string->number "-ff" 16) "parse negative hex")
;; Base 36 (max)
(assert-equal 35 (string->number "z" 36) "parse base-36 'z'")
(assert-equal 36 (string->number "10" 36) "parse base-36 '10'")
;; =============================================================================
;; R7RS Compliance Tests - Radix Prefixes
;; =============================================================================
;; Binary prefix (#b)
(assert-equal 10 (string->number "#b1010") "parse with #b prefix")
(assert-equal 10 (string->number "#B1010") "parse with #B prefix (uppercase)")
(assert-equal 255 (string->number "#b11111111") "parse #b11111111")
;; Octal prefix (#o)
(assert-equal 63 (string->number "#o77") "parse with #o prefix")
(assert-equal 63 (string->number "#O77") "parse with #O prefix (uppercase)")
(assert-equal 255 (string->number "#o377") "parse #o377")
;; Decimal prefix (#d)
(assert-equal 123 (string->number "#d123") "parse with #d prefix")
(assert-equal 456 (string->number "#D456") "parse with #D prefix (uppercase)")
;; Hexadecimal prefix (#x)
(assert-equal 255 (string->number "#xff") "parse with #x prefix")
(assert-equal 255 (string->number "#XFF") "parse with #X prefix (uppercase)")
(assert-equal 16 (string->number "#x10") "parse #x10")
;; =============================================================================
;; R7RS Compliance Tests - Float Parsing
;; =============================================================================
;; Basic floats
(assert-equal 3.14 (string->number "3.14") "parse float '3.14'")
(assert-equal -3.14 (string->number "-3.14") "parse negative float")
(assert-equal 0.5 (string->number "0.5") "parse '0.5'")
(assert-equal 0.5 (string->number ".5") "parse '.5' without leading zero")
;; Scientific notation
(assert-equal 10000000000.0 (string->number "1e10") "parse '1e10'")
(assert-equal 10000000000.0 (string->number "1E10") "parse '1E10' uppercase")
(assert-equal 314.0 (string->number "3.14e2") "parse '3.14e2'")
(assert-equal -0.0025 (string->number "-2.5e-3")
 "parse negative scientific notation")
(assert-equal 123000.0 (string->number "1.23e+5")
 "parse with explicit plus exponent")

;; =============================================================================
;; R7RS Compliance Tests - Return #f on Failure
;; =============================================================================
;; Invalid strings return nil (#f), not error
(assert-nil (string->number "xyz") "invalid string returns nil")
(assert-nil (string->number "12.34.56") "multiple decimal points returns nil")
(assert-nil (string->number "") "empty string returns nil")
(assert-nil (string->number "  ") "whitespace only returns nil")
(assert-nil (string->number "abc" 10) "invalid base-10 string returns nil")
(assert-nil (string->number "xyz" 16) "invalid hex string returns nil")
(assert-nil (string->number "2" 2) "digit out of range for base returns nil")

;; =============================================================================
;; R7RS Compliance Tests - number->string with Radix
;; =============================================================================
;; Base 10 (default)
(assert-equal "0" (number->string 0) "convert 0 to string")
(assert-equal "42" (number->string 42) "convert 42 to string")
(assert-equal "-42" (number->string -42) "convert -42 to string")
;; Binary (base 2)
(assert-equal "1010" (number->string 10 2) "convert 10 to binary")
(assert-equal "11111111" (number->string 255 2) "convert 255 to binary")
(assert-equal "-1010" (number->string -10 2) "convert -10 to binary")
;; Octal (base 8)
(assert-equal "77" (number->string 63 8) "convert 63 to octal")
(assert-equal "377" (number->string 255 8) "convert 255 to octal")
(assert-equal "-77" (number->string -63 8) "convert -63 to octal")
;; Hexadecimal (base 16)
(assert-equal "ff" (number->string 255 16) "convert 255 to hex")
(assert-equal "10" (number->string 16 16) "convert 16 to hex")
(assert-equal "-ff" (number->string -255 16) "convert -255 to hex")
;; Base 36 (max)
(assert-equal "z" (number->string 35 36) "convert 35 to base-36")
(assert-equal "10" (number->string 36 36) "convert 36 to base-36")
;; Floats (only base 10)
(assert-equal "3.14" (number->string 3.14) "convert float 3.14 to string")
(assert-equal "-3.14" (number->string -3.14) "convert -3.14 to string")
(assert-equal "10000000000.0" (number->string 10000000000.0)
 "convert large float to string")
;; =============================================================================
;; Round-trip Tests (string->number and number->string)
;; =============================================================================
;; Integers in various bases
(assert-equal "1010" (number->string (string->number "1010" 2) 2)
 "round-trip binary")
(assert-equal "ff" (number->string (string->number "ff" 16) 16)
 "round-trip hex")
(assert-equal "77" (number->string (string->number "77" 8) 8)
 "round-trip octal")
;; Floats
(assert-equal 3.14 (string->number (number->string 3.14)) "round-trip float")
