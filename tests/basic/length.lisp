;; Tests for length and substring functions
;; Both work with Unicode code points (not graphemes)
(load "tests/test-helpers.lisp")

;; ============================================================
;; Length tests
;; ============================================================
;; Basic ASCII
(assert-equal 5 (length "hello") "ASCII string length")
(assert-equal 0 (length "") "Empty string length")
(assert-equal 4 (string-length "test") "string-length alias works")
;; CJK characters (each is one code point)
(assert-equal 2 (length "世界") "CJK characters")
(assert-equal 10 (length "Hello, 世界!") "Mixed ASCII and CJK")
;; Emoji without variation selector (one code point)
(assert-equal 1 (length "🌍") "Single emoji")
(assert-equal 12 (length "Hello, 世界! 🌍") "String with emoji")
;; Multiple emoji
(assert-equal 3 (length "🌍🌎🌏") "Multiple emoji")
;; Emoji with variation selector (U+FE0F) - counts as 2 code points
;; These are emoji followed by VS16 to force emoji presentation
(assert-equal 2 (length "🌍️") "Emoji with variation selector")
(assert-equal 2 (length "⚔️") "Crossed swords with VS16")
(assert-equal 2 (length "▶️") "Play button with VS16")
;; Multiple emoji with variation selectors (3 emoji + 3 VS16 = 6 code points)
(assert-equal 6 (length "🌍️🌎️🌏️") "Multiple emoji with VS16")
;; Mixed: some with VS16, some without (2+1+2 = 5 code points)
(assert-equal 5 (length "🌍️🌎🌏️") "Mixed emoji with/without VS16")
;; Precomposed characters
(assert-equal 4 (length "café") "Precomposed café")
;; Lists (unchanged behavior)
(assert-equal 0 (length 'nil) "Empty list length")
(assert-equal 3 (length '(1 2 3)) "List length")
(assert-equal 5 (length '(a b c d e)) "Symbol list length")
;; Vectors (unchanged behavior)
(assert-equal 0 (length #()) "Empty vector length")
(assert-equal 3 (length #(1 2 3)) "Vector length")
;; ============================================================
;; Substring tests (must be consistent with length)
;; ============================================================
;; Basic ASCII substring
(assert-equal "hello" (substring "hello" 0 5) "Full ASCII substring")
(assert-equal "ell" (substring "hello" 1 4) "Middle ASCII substring")
(assert-equal "" (substring "hello" 0 0) "Empty substring")
;; CJK substring
(assert-equal "世界" (substring "世界" 0 2) "Full CJK substring")
(assert-equal "世" (substring "世界" 0 1) "First CJK char")
(assert-equal "界" (substring "世界" 1 2) "Second CJK char")
;; Emoji substring - code point based indexing
;; "🌍️" has 2 code points: emoji (pos 0) and VS16 (pos 1)
(assert-equal "🌍" (substring "🌍️" 0 1) "First code point (emoji)")
(assert-equal "️" (substring "🌍️" 1 2) "Second code point (VS16)")
(assert-equal "🌍️" (substring "🌍️" 0 2) "Both code points")
(assert-equal 1 (length (substring "🌍️" 0 1)) "Substring length matches")
;; Verify substring 0 to length returns original string
(assert-equal "🌍️" (substring "🌍️" 0 (length "🌍️"))
 "substring 0 length = original")
(assert-equal
 "Hello, 世界! 🌍" (substring "Hello, 世界! 🌍" 0 (length "Hello, 世界! 🌍"))
 "Full string via length")
;; Mixed string substring
(assert-equal "世界" (substring "Hello, 世界! 🌍" 7 9) "CJK from mixed string")
(assert-equal "🌍" (substring "Hello, 世界! 🌍" 11 12) "Emoji from mixed string")
;; Multiple emoji with VS16 - code point indexing
;; "🌍️🌎️🌏️" = positions: 🌍(0) ️(1) 🌎(2) ️(3) 🌏(4) ️(5)
(assert-equal "🌍️" (substring "🌍️🌎️🌏️" 0 2) "First emoji+VS16")
(assert-equal "🌎️" (substring "🌍️🌎️🌏️" 2 4) "Second emoji+VS16")
(assert-equal "🌏️" (substring "🌍️🌎️🌏️" 4 6) "Third emoji+VS16")

(princ "All length and substring tests passed!\n")
