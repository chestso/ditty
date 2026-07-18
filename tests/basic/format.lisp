;;; Test format and terpri functions
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; Test format with nil destination (returns string)
(assert-equal "Hello, world!" (format nil "Hello, world!")
 "format with no directives")
(assert-equal "The answer is 42" (format nil "The answer is ~A" 42)
 "format with ~A directive")
(assert-equal "2 + 3 = 5" (format nil "~A + ~A = ~A" 2 3 5)
 "format with multiple ~A directives")
;; Test format with ~S (S-expression, with quotes)
(assert-equal "String: \"hello\"" (format nil "String: ~S" "hello")
 "format with ~S for string")
(assert-equal "Number: 42" (format nil "Number: ~S" 42)
 "format with ~S for number")
(assert-equal "List: (1 2 3)" (format nil "List: ~S" '(1 2 3))
 "format with ~S for list")
;; Test format with ~A (aesthetic, no quotes)
(assert-equal "String: hello" (format nil "String: ~A" "hello")
 "format with ~A for string (no quotes)")
(assert-equal "Name: Alice" (format nil "Name: ~A" "Alice")
 "format ~A displays string without quotes")
;; Test format with ~% (newline)
(assert-equal "Line 1\nLine 2" (format nil "Line 1~%Line 2")
 "format with ~% newline")
(assert-equal "First\nSecond\nThird" (format nil "First~%Second~%Third")
 "format with multiple ~% newlines")
;; Test format with ~~ (literal tilde)
(assert-equal "Tilde: ~" (format nil "Tilde: ~~")
 "format with ~~ produces literal tilde")
(assert-equal "~A is not a directive" (format nil "~~A is not a directive")
 "format with ~~ escapes directive")
;; Test format with mixed directives
(assert-equal
 "Name: Bob, Age: 30\n" (format nil "Name: ~A, Age: ~A~%" "Bob" 30)
 "format with mixed ~A and ~%")
(assert-equal "Result: \"success\"" (format nil "~A: ~S" "Result" "success")
 "format with ~A and ~S together")
;; Test format with numbers
(assert-equal "Integer: 123" (format nil "Integer: ~A" 123)
 "format with integer")
(assert-equal "Float: 3.14" (format nil "Float: ~A" 3.14) "format with float")
;; Test format with booleans
(assert-equal "Boolean: #t" (format nil "Boolean: ~A" #t)
 "format with boolean true")
(assert-equal "Boolean: nil" (format nil "Boolean: ~A" nil)
 "format with boolean false (#f = nil)")
;; Test format with symbols
(assert-equal "Symbol: foo" (format nil "Symbol: ~A" 'foo)
 "format with symbol using ~A")
(assert-equal "Symbol: bar" (format nil "Symbol: ~S" 'bar)
 "format with symbol using ~S")
;; Test format with lists
(assert-equal "List: (a b c)" (format nil "List: ~A" '(a b c))
 "format with list")
(assert-equal "Empty: nil" (format nil "Empty: ~A" 'nil)
 "format with empty list")

;; Test format with variables
(define x 100)

(assert-equal "Value of x is 100" (format nil "Value of x is ~A" x)
 "format with variable value")

(define name "Alice")

(assert-equal "Hello, Alice!" (format nil "Hello, ~A!" name)
 "format with string variable")
;; Test format building complex strings
(assert-equal "Hello World" (format nil "~A~A~A" "Hello" " " "World")
 "format concatenates multiple values")
(assert-equal "Result: Done\n" (format nil "Result: ~A~%" "Done")
 "format with string and newline")

;; Test terpri (just prints newline, returns nil)
(assert-nil (terpri) "terpri returns nil")

;; Note: format with #t destination prints to stdout and returns nil
;; (format #t "Hello!~%")  ; This would print "Hello!" followed by newline and return nil
;; We can't easily test stdout output in test format, so those are commented
;; Test case sensitivity of directives
(assert-equal "42 lowercase" (format nil "~a lowercase" 42)
 "format ~a directive (lowercase)")
(assert-equal "\"test\" lowercase" (format nil "~s lowercase" "test")
 "format ~s directive (lowercase)")
