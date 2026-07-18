;; Printing Functions (Common Lisp Style)
;; Tests for princ, prin1, and print functions
;; Note: These functions print to stdout and return the object being printed
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; princ - prints without quotes (human-readable), returns the object
(assert-equal "Hello, World!" (princ "Hello, World!") "princ returns string")
(assert-equal 42 (princ 42) "princ returns integer")
(assert-equal 3.14 (princ 3.14) "princ returns float")

(assert-true (princ #t) "princ returns boolean true")

(assert-false (princ nil) "princ returns boolean false")

(assert-nil (princ nil) "princ returns nil")

(assert-equal '(1 2 3) (princ '(1 2 3)) "princ returns list")
(assert-equal 'symbol (princ 'symbol) "princ returns symbol")
;; prin1 - prints with quotes (readable representation), returns the object
(assert-equal "Hello, World!" (prin1 "Hello, World!") "prin1 returns string")
(assert-equal 42 (prin1 42) "prin1 returns integer")
(assert-equal 3.14 (prin1 3.14) "prin1 returns float")

(assert-true (prin1 #t) "prin1 returns boolean true")

(assert-false (prin1 nil) "prin1 returns boolean false")

(assert-nil (prin1 nil) "prin1 returns nil")

(assert-equal '(1 2 3) (prin1 '(1 2 3)) "prin1 returns list")
(assert-equal 'symbol (prin1 'symbol) "prin1 returns symbol")
;; print - like prin1 but adds newline before and after, returns the object
(assert-equal "Hello, World!" (print "Hello, World!") "print returns string")
(assert-equal 42 (print 42) "print returns integer")
(assert-equal 3.14 (print 3.14) "print returns float")

(assert-true (print #t) "print returns boolean true")

(assert-false (print nil) "print returns boolean false")

(assert-nil (print nil) "print returns nil")

(assert-equal '(1 2 3) (print '(1 2 3)) "print returns list")
(assert-equal 'symbol (print 'symbol) "print returns symbol")

;; All functions return the object being printed (Common Lisp convention)
(define result1 (princ "test"))

(assert-equal "test" result1 "princ result assigned to variable")

(define result2 (prin1 "test"))

(assert-equal "test" result2 "prin1 result assigned to variable")

(define result3 (print "test"))

(assert-equal "test" result3 "print result assigned to variable")

;; Test with vectors
(define vec (make-vector 3))

(vector-set! vec 0 1)
(vector-set! vec 1 2)
(vector-set! vec 2 3)

(assert-equal vec (princ vec) "princ returns vector")
(assert-equal vec (prin1 vec) "prin1 returns vector")
(assert-equal vec (print vec) "print returns vector")
;; Test with nested structures
(assert-equal '(1 (2 3) 4) (princ '(1 (2 3) 4)) "princ returns nested list")
(assert-equal '(1 (2 3) 4) (prin1 '(1 (2 3) 4)) "prin1 returns nested list")
(assert-equal '(1 (2 3) 4) (print '(1 (2 3) 4)) "print returns nested list")

;; Test with empty structures
(assert-nil (princ 'nil) "princ returns empty list as nil")
(assert-nil (prin1 'nil) "prin1 returns empty list as nil")
(assert-nil (print 'nil) "print returns empty list as nil")

;; Test with strings containing special characters
(assert-equal "Hello\nWorld" (princ "Hello\nWorld")
 "princ returns string with newline")
(assert-equal "Hello\nWorld" (prin1 "Hello\nWorld")
 "prin1 returns string with newline")
(assert-equal "Hello\nWorld" (print "Hello\nWorld")
 "print returns string with newline")
