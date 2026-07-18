;; List Reverse Tests
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; Basic List Reversal
(assert-nil (reverse 'nil) "reverse of empty list is nil")
(assert-nil (reverse nil) "reverse of nil is nil")

(assert-equal '(1) (reverse '(1)) "reverse of single element list")
(assert-equal '(2 1) (reverse '(1 2)) "reverse of two element list")
(assert-equal '(3 2 1) (reverse '(1 2 3)) "reverse of three element list")
(assert-equal '(5 4 3 2 1) (reverse '(1 2 3 4 5))
 "reverse of five element list")
;; Different Element Types
(assert-equal '("c" "b" "a") (reverse '("a" "b" "c")) "reverse string list")
(assert-equal '(#t nil #t) (reverse '(#t nil #t)) "reverse boolean list")
(assert-equal '(3 "two" 1) (reverse '(1 "two" 3)) "reverse mixed type list")
;; Nested Lists
(assert-equal '((3 4) (1 2)) (reverse '((1 2) (3 4))) "reverse list of lists")
(assert-equal '(4 (2 3) 1) (reverse '(1 (2 3) 4))
 "reverse list with nested list")

;; Use with Other Functions
(define lst '(1 2 3 4 5))

(assert-equal 5 (length (reverse lst)) "length of reversed list unchanged")
(assert-equal 5 (car (reverse lst)) "car of reversed list is last element")
(assert-equal 5 (list-ref (reverse lst) 0)
 "list-ref at index 0 of reversed list")
(assert-equal 1 (list-ref (reverse lst) 4)
 "list-ref at index 4 of reversed list")
;; Double reverse returns original
(assert-equal '(1 2 3) (reverse (reverse '(1 2 3)))
 "double reverse returns original")
;; Integration with cons and car/cdr
(assert-equal 3 (car (reverse '(1 2 3))) "car of reversed list")
(assert-equal '(2 1) (cdr (reverse '(1 2 3))) "cdr of reversed list")
