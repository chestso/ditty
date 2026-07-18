;; Test quasiquote (backquote) functionality
(load "tests/test-helpers.lisp")

;; Test 1: Simple quasiquote without unquote (same as quote)
(assert-equal '(1 2 3) `(1 2 3) "simple quasiquote")

;; Test 2: Unquote a single value
(define x 42)

(assert-equal '(1 42 3) `(1 ,x 3) "unquote single value")
;; Test 3: Multiple unquotes
(assert-equal '(42 3 6) `(,x ,(+ 1 2) ,(* 2 3)) "multiple unquotes")
;; Test 4: Nested lists with unquote
(assert-equal '(a (b 42 c) d) `(a (b ,x c) d) "nested lists with unquote")

;; Test 5: Unquote-splicing with a list
(define lst '(a b c))

(assert-equal '(1 a b c 4) `(1 ,@lst 4) "unquote-splicing")
;; Test 6: Multiple unquote-splicing
(assert-equal '(a b c a b c) `(,@lst ,@lst) "multiple unquote-splicing")
;; Test 7: Mixing unquote and unquote-splicing
(assert-equal '(start 42 middle a b c end) `(start ,x middle ,@lst end)
 "mixing unquote and unquote-splicing")

;; Test 8: Empty list unquote-splicing
(define empty 'nil)

(assert-equal '(1 2) `(1 ,@empty 2) "empty list unquote-splicing")

;; Test 9: Using quasiquote in a macro
(defmacro my-when (condition body) `(if ,condition ,body nil))

(assert-equal 99 (my-when #t 99) "macro with quasiquote returns value")

(assert-nil (my-when nil 99) "macro with quasiquote returns nil")

;; Test 10: Building code with quasiquote
(define make-adder (lambda (n) `(lambda (x) (+ x ,n))))

(assert-equal '(lambda (x) (+ x 5)) (make-adder 5)
 "building code with quasiquote")
