;; cond and case examples
;; Demonstrate multi-way conditionals and pattern matching
(load "tests/test-helpers.lisp")

;; ===========================================
;; cond - Multi-way conditional
;; ===========================================
;; Grade function using cond
(define grade
  (lambda (score)
    (cond
      ((>= score 90) "A")
      ((>= score 80) "B")
      ((>= score 70) "C")
      ((>= score 60) "D")
      (else "F"))))

(assert-equal "A" (grade 95) "grade A")
(assert-equal "B" (grade 85) "grade B")
(assert-equal "C" (grade 75) "grade C")
(assert-equal "D" (grade 65) "grade D")
(assert-equal "F" (grade 50) "grade F")

;; Sign of a number
(define sign (lambda (n) (cond ((< n 0) -1) ((= n 0) 0) (else 1))))

(assert-equal -1 (sign -5) "sign of negative")
(assert-equal 0 (sign 0) "sign of zero")
(assert-equal 1 (sign 10) "sign of positive")

;; Abs using cond
(define abs (lambda (n) (cond ((< n 0) (- n)) (else n))))

(assert-equal 10 (abs -10) "absolute value of negative")
(assert-equal 5 (abs 5) "absolute value of positive")

;; Cond without else returns NIL
(assert-nil (cond ((< 5 3) "no") ((< 10 5) "nope"))
 "cond without else returns nil")
;; Empty cond
(assert-nil (cond) "empty cond returns nil")

;; Single clause cond
(assert-equal "yes" (cond ((= 2 2) "yes")) "single clause cond")

;; ===========================================
;; case - Pattern matching
;; ===========================================
;; Day of week
(define day-name
  (lambda (n)
    (case
      n
      ((1) "Monday")
      ((2) "Tuesday")
      ((3) "Wednesday")
      ((4) "Thursday")
      ((5) "Friday")
      ((6 7) "Weekend")
      (else "Invalid"))))

(assert-equal "Monday" (day-name 1) "day name Monday")
(assert-equal "Wednesday" (day-name 3) "day name Wednesday")
(assert-equal "Weekend" (day-name 6) "day 6 is weekend")
(assert-equal "Weekend" (day-name 7) "day 7 is weekend")
(assert-equal "Invalid" (day-name 99) "invalid day number")

;; Command dispatcher
(define command
  (lambda (cmd)
    (case
      cmd
      (("help" "h") "showing help")
      (("quit" "exit" "q") "quitting")
      (("save" "write") "saving")
      (else "unknown command"))))

(assert-equal "showing help" (command "help") "help command")
(assert-equal "showing help" (command "h") "h command")
(assert-equal "quitting" (command "quit") "quit command")
(assert-equal "unknown command" (command "xyz") "unknown command")

;; Number classifier
(define classify-number
  (lambda (x)
    (case
      x
      ((0) "zero")
      ((-1 -2 -3) "small negative")
      ((1 2 3) "small positive")
      ((4 5 6 7 8 9 10) "medium")
      (else "large"))))

(assert-equal "zero" (classify-number 0) "classify zero")
(assert-equal "small positive" (classify-number 3) "classify small positive")
(assert-equal "medium" (classify-number 7) "classify medium")
(assert-equal "large" (classify-number 100) "classify large")

;; Case without else returns NIL
(assert-nil (case 99 ((1 2 3) "match")) "case without else returns nil")

;; ===========================================
;; Nested conditionals
;; ===========================================
;; Nested cond
(define classify
  (lambda (x)
    (cond
      ((< x 0) "negative")
      ((= x 0) "zero")
      (else (cond ((even? x) "positive even") (else "positive odd"))))))

(assert-equal "negative" (classify -5) "classify negative")
(assert-equal "zero" (classify 0) "classify zero")
(assert-equal "positive even" (classify 4) "classify positive even")
(assert-equal "positive odd" (classify 7) "classify positive odd")

;; Complex nested structure
(define status
  (lambda (age status)
    (case
      status
      (("student") "eligible for student discount")
      (("senior")
       (cond ((>= age 65) "senior discount") (else "not yet senior")))
      (else "no discount"))))

(assert-equal "eligible for student discount" (status 20 "student")
 "student status")
(assert-equal "senior discount" (status 70 "senior") "senior discount")
(assert-equal "not yet senior" (status 50 "senior") "not yet senior")

;; ===========================================
;; Practical examples
;; ===========================================
;; Menu selection
(define menu-action
  (lambda (choice)
    (case
      choice
      ((1) "Creating new file")
      ((2) "Opening file")
      ((3) "Saving file")
      ((4) "Closing file")
      ((0) "Exiting")
      (else "Invalid choice"))))

(assert-equal "Creating new file" (menu-action 1) "menu option 1")
(assert-equal "Exiting" (menu-action 0) "menu option 0")
(assert-equal "Invalid choice" (menu-action 99) "invalid menu option")

;; Conditional evaluation order (cond)
(define test-order
  (lambda (x)
    (cond
      ((> x 100) "very large")
      ((> x 50) "large")
      ((> x 10) "medium")
      ((> x 0) "small")
      (else "non-positive"))))

(assert-equal "very large" (test-order 150) "test order very large")
(assert-equal "large" (test-order 75) "test order large")
(assert-equal "medium" (test-order 25) "test order medium")
(assert-equal "small" (test-order 5) "test order small")
(assert-equal "non-positive" (test-order 0) "test order non-positive")
