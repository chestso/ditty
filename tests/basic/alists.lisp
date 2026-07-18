;;; Test alist operations
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; Create a simple alist
(define simple-alist '(("a" . 1) ("b" . 2) ("c" . 3)))

;; Test assoc - find by value equality
(assert-equal '("a" . 1) (assoc "a" simple-alist)
 "assoc finds first pair with key 'a'")
(assert-equal '("b" . 2) (assoc "b" simple-alist)
 "assoc finds pair with key 'b'")
(assert-equal '("c" . 3) (assoc "c" simple-alist)
 "assoc finds pair with key 'c'")

(assert-nil (assoc "d" simple-alist) "assoc returns nil for missing key")

;; Test assoc with integer keys
(define num-alist '((1 . "one") (2 . "two") (3 . "three")))

(assert-equal '(1 . "one") (assoc 1 num-alist) "assoc works with integer key 1")
(assert-equal '(2 . "two") (assoc 2 num-alist) "assoc works with integer key 2")

(assert-nil (assoc 4 num-alist) "assoc returns nil for missing integer key")

;; Test alist-get - returns value only
(assert-equal 1 (alist-get "a" simple-alist)
 "alist-get returns value for key 'a'")
(assert-equal 2 (alist-get "b" simple-alist)
 "alist-get returns value for key 'b'")
(assert-equal 3 (alist-get "c" simple-alist)
 "alist-get returns value for key 'c'")

(assert-nil (alist-get "d" simple-alist)
 "alist-get returns nil for missing key")

;; Test alist-get with default value
(assert-equal 1 (alist-get "a" simple-alist 99)
 "alist-get returns found value, ignoring default")
(assert-equal 99 (alist-get "z" simple-alist 99)
 "alist-get returns default for missing key")

;; Test with nested pairs
(define nested-alist '(("x" . 10) ("y" . 20) ("z" . 30)))

(assert-equal "x" (car (assoc "x" nested-alist))
 "car of assoc result gives key")
(assert-equal 10 (cdr (assoc "x" nested-alist))
 "cdr of assoc result gives value")

;; Test assq - pointer equality (for symbols/same objects)
(define sym1 "test")

(define sym2 "test")

(define sym-alist (list (cons sym1 100) (cons sym2 200)))

;; assoc uses value equality, so finds first match
(assert-equal 100 (alist-get sym1 sym-alist)
 "assoc uses value equality for string keys")
;; Test mapping functions
;; Test map with builtin function
(assert-equal '(2 4 6 8 10) (map (lambda (x) (* x 2)) '(1 2 3 4 5))
 "map doubles list elements")
;; Test mapcar (should be same as map)
(assert-equal '(2 4 6 8 10) (mapcar (lambda (x) (* x 2)) '(1 2 3 4 5))
 "mapcar doubles list elements")
;; Test map with addition
(assert-equal '(11 12 13) (map (lambda (x) (+ x 10)) '(1 2 3))
 "map adds 10 to each element")

;; Test map with string operations
(define words '("hello" "world" "lisp"))

(assert-equal
 '("HELLO" "WORLD" "LISP") (map (lambda (s) (string-upcase s)) words)
 "map converts strings to uppercase")

;; Test map on empty list
(assert-nil (map (lambda (x) (* x 2)) 'nil) "map on empty list returns nil")

;; Test map with more complex function
(assert-equal
 '("small" "big" "small" "big" "small")
 (map (lambda (x) (if (> x 5) "big" "small")) '(3 7 2 9 4))
 "map classifies numbers as big or small")

;; Test combining alists and map
(define people '(("Alice" . 25) ("Bob" . 30) ("Carol" . 35)))

(assert-equal '("Alice" "Bob" "Carol") (map (lambda (pair) (car pair)) people)
 "map extracts keys from alist")
(assert-equal '(25 30 35) (map (lambda (pair) (cdr pair)) people)
 "map extracts values from alist")
;; Test map to double all values in an alist (returns list of pairs)
(assert-equal
 (map (lambda (pair) (cons (car pair) (* (cdr pair) 2)))
  '((1 . 10) (2 . 20) (3 . 30)))
 '((1 . 20) (2 . 40) (3 . 60)) "map transforms alist values")
