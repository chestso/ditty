;; Test: Tail recursion - tail call unwrapping in lambda bodies
;; Regression test for bug where tail calls in lambda bodies weren't being unwrapped
;; This caused functions to return #<tail-call> objects instead of actual values
(load "tests/test-helpers.lisp")

;; Helper function that returns a value from another function in tail position
(define helper
  (lambda (x)
    (if (> x 0) (concat "positive-" (number->string x)) "non-positive")))

;; Function that calls helper in tail position
(define wrapper (lambda (x) (if (> x 10) (helper x) "small")))

;; Test: Simple tail call should return actual value, not tail-call object
(assert-equal "positive-15" (wrapper 15)
 "Simple tail call returns actual value")
(assert-equal "small" (wrapper 5) "Non-tail call returns value")

;; Test: Nested tail calls
(define level1 (lambda (x) (concat "L1-" x)))

(define level2 (lambda (x) (level1 (concat "L2-" x))))

(define level3 (lambda (x) (level2 (concat "L3-" x))))

(assert-equal "L1-L2-L3-test" (level3 "test") "Nested tail calls")

;; Test: Conditional tail calls in both branches
(define conditional-tail
  (lambda (x pred) (if pred (helper x) (level1 (number->string x)))))

(assert-equal "positive-20" (conditional-tail 20 #t)
 "Conditional tail call - true branch")
(assert-equal "L1-20" (conditional-tail 20 nil)
 "Conditional tail call - false branch")

;; Test: Tail call in let expression
(define let-tail (lambda (x) (let ((y (* x 2))) (helper y))))

(assert-equal "positive-14" (let-tail 7) "Tail call in let expression")

;; Test: Complex nested structure with multiple tail calls
(define trim-punctuation
  (lambda (word)
    (if (not (and (string? word) (> (length word) 0)))
      ""
      (let* ((no-trailing (regex-replace "[.,!?;:]+" word ""))
             (cleaned (regex-replace "^[.,!?;:]+" no-trailing "")))
        cleaned))))

(define clean-word
  (lambda (word)
    (if (and (string? word) (> (length word) 0)) (trim-punctuation word) "")))

;; This was the original bug case - clean-word would return #<tail-call> instead of string
(assert-equal "hello" (clean-word "hello,") "Complex nested tail call - hello,")
(assert-equal "world" (clean-word "world!") "Complex nested tail call - world!")
(assert-equal "" (clean-word "") "Complex nested tail call - empty string")

;; Test: Multiple arguments with tail call
(define multi-arg-tail (lambda (a b c) (concat a b c)))

(define multi-wrapper (lambda (x) (multi-arg-tail "A-" x "-Z")))

(assert-equal "A-test-Z" (multi-wrapper "test")
 "Multiple arguments with tail call")
