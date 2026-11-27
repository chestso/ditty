;; read-from-string: parse the first S-expression from a string as data.
;; Parse failures raise the reader's typed errors (reader-error and its
;; child unclosed-input); empty/comment-only input returns nil.
(load "tests/test-helpers.lisp")

;;; Reads forms as data, without evaluating them
(assert-equal '(+ 1 2) (read-from-string "(+ 1 2)") "list form")
(assert-equal 3 (eval (read-from-string "(+ 1 2)")) "eval of read form")
(assert-equal 42 (read-from-string "42") "integer atom")
(assert-equal 'hello (read-from-string "hello") "symbol atom")
(assert-equal ''x (read-from-string "'x") "quoted form")
(assert-equal #(1 2 3) (read-from-string "#(1 2 3)") "vector literal")
(assert-equal :key (read-from-string ":key") "keyword")

(assert-true (read-from-string "#t") "#t reads as true")

(assert-nil (read-from-string "#f") "#f reads as nil")

(assert-equal "a\nb" (read-from-string "\"a\\nb\"") "string with escape")
(assert-equal #\A (read-from-string "#\\x41") "character literal")

;;; Only the first expression is read; the rest is ignored
(assert-equal 1 (read-from-string "1 2 3") "first expression only")
(assert-equal '(a) (read-from-string "(a) (b)") "first form only")
(assert-equal 'x (read-from-string "  \t x  ") "leading whitespace skipped")
(assert-equal 'x (read-from-string "; comment\nx") "leading comment skipped")

;;; Nothing to read returns nil
(assert-nil (read-from-string "") "empty string")
(assert-nil (read-from-string "   \t\n ") "whitespace only")
(assert-nil (read-from-string "; just a comment") "comment only")

;;; Incomplete input raises unclosed-input (caught by reader-error too)
(assert-equal 'incomplete
 (condition-case err (read-from-string "(unclosed")
   (unclosed-input 'incomplete))
 "unclosed list raises unclosed-input")
(assert-equal 'incomplete
 (condition-case err (read-from-string "\"unclosed")
   (reader-error 'incomplete))
 "reader-error handler catches unclosed-input")
(assert-equal 'incomplete
 (condition-case err (read-from-string "\"\\x1")
   (unclosed-input 'incomplete))
 "truncated hex escape is unclosed-input")

;;; Malformed input raises reader-error, not unclosed-input
(assert-equal 'malformed
 (condition-case err (read-from-string ")")
   (unclosed-input 'wrong)
   (reader-error 'malformed))
 "stray paren is reader-error")
(assert-equal 'malformed
 (condition-case err (read-from-string "\"\\q\"")
   (reader-error 'malformed))
 "unknown string escape is reader-error")

;;; Argument checking
(assert-error (read-from-string 42) "non-string argument")
(assert-error (read-from-string) "missing argument")

(assert-equal 'type-error
 (condition-case err (read-from-string 42)
   (type-error 'type-error))
 "non-string argument raises type-error")
(assert-equal 'program-error
 (condition-case err (read-from-string)
   (program-error 'program-error))
 "missing argument raises program-error")
