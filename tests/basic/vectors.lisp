;; Vector operations examples
;; Load test helper macros
(load "tests/test-helpers.lisp")

;; Create vectors using literal syntax
(define v1 #(10 20 30))

(assert-equal #(10 20 30) v1 "vector literal creates vector")
(assert-equal 10 (vector-ref v1 0) "vector-ref at index 0")
(assert-equal 20 (vector-ref v1 1) "vector-ref at index 1")
(assert-equal 30 (vector-ref v1 2) "vector-ref at index 2")

;; Create empty vector
(define empty1 #())

(assert-equal #() empty1 "empty vector literal")
(assert-equal 0 (length empty1) "empty vector has length 0")

;; Create and manipulate vectors programmatically
(define v (make-vector 5))

(assert-equal 0 (length v) "make-vector creates empty vector")

;; Add elements
(vector-push! v 10)
(vector-push! v 20)
(vector-push! v 30)

(assert-equal 3 (length v) "vector has 3 elements after pushes")
(assert-equal #(10 20 30) v "vector contains pushed elements")

;; Create vector with initial value (regression test for make-vector bug)
(define v-init (make-vector 5 42))

(assert-equal 5 (length v-init)
 "make-vector with initial value creates vector of correct length")
(assert-equal 42 (vector-ref v-init 0) "first element has initial value")
(assert-equal 42 (vector-ref v-init 1) "second element has initial value")
(assert-equal 42 (vector-ref v-init 4) "last element has initial value")
(assert-equal #(42 42 42 42 42) v-init "all elements initialized correctly")

;; Create vector with initial value 0 (should not be nil)
(define v-zero (make-vector 3 0))

(assert-equal 0 (vector-ref v-zero 0) "zero is a valid initial value")
(assert-equal 0 (vector-ref v-zero 1) "all elements set to zero")
(assert-equal #(0 0 0) v-zero "vector of zeros created correctly")
;; Access elements
(assert-equal 10 (vector-ref v 0) "access first element")
(assert-equal 20 (vector-ref v 1) "access second element")
(assert-equal 30 (vector-ref v 2) "access third element")

;; Modify elements
(vector-set! v 0 100)

(assert-equal #(100 20 30) v "vector-set! modifies element")
;; Pop elements
(assert-equal 30 (vector-pop! v) "vector-pop! returns popped element")
(assert-equal 2 (length v) "vector length decreases after pop")

;; Vectors are always truthy (even when empty - only nil is falsy in this Lisp)
(define empty (make-vector 0))

(assert-equal #() empty "empty vector equals #()")
(assert-equal 0 (length empty) "empty vector has length 0")
(assert-equal "not empty" (if v "not empty" "empty")
 "non-empty vector is truthy")
(assert-equal "not empty" (if empty "not empty" "empty")
 "empty vector is truthy (only nil is falsy)")
(assert-equal "not empty" (if empty1 "not empty" "empty")
 "empty vector literal is truthy")
(assert-equal "not empty" (if v1 "not empty" "empty")
 "non-empty vector literal is truthy")
