;; Collection Iteration Examples
;; Demonstration of iterating over hash tables, vectors, and lists using do loops
(load "tests/test-helpers.lisp")

;; ===========================================
;; Hash Table Iteration
;; ===========================================
(define ht (make-hash-table))

(hash-set! ht "name" "Alice")
(hash-set! ht "age" 30)
(hash-set! ht "city" "NYC")

(assert-equal 3 (hash-count ht) "hash table with 3 entries")

;; Get all keys
(define keys (hash-keys ht))

(assert-equal '("city" "name" "age") keys "hash-keys returns all keys")

;; Get all values
(define values (hash-values ht))

(assert-equal '("NYC" "Alice" 30) values "hash-values returns all values")

;; Get key-value pairs as list of cons cells
(define entries (hash-entries ht))

(assert-equal '(("city" . "NYC") ("name" . "Alice") ("age" . 30)) entries
 "hash-entries returns key-value pairs")
;; Iterate over keys using do loop
(assert-equal
 (do ((remaining keys (cdr remaining))) ((null? remaining) "done")
   (car remaining)) "done"
 "iterate over hash keys")
;; Iterate over entries and process key-value pairs
(assert-equal
 (do ((remaining entries (cdr remaining))) ((null? remaining) "done")
   (let ((entry (car remaining)))
     (let ((key (car entry))
           (val (cdr entry)))
       (if (string? val)
         (concat key ": " val)
         (concat key ": " (number->string val)))))) "done"
 "iterate over hash entries")

;; ===========================================
;; Vector Iteration
;; ===========================================
(define v #(10 20 30 40 50))

;; Iterate with index and access elements
(assert-equal
 "done" (do ((i 0 (+ i 1))) ((>= i (length v)) "done") (vector-ref v i))
 "iterate over vector with index")

;; Sum all elements using accumulator
(define sum 0)

(assert-equal
 (do ((i 0 (+ i 1)) (acc 0)) ((>= i (length v)) acc)
   (set! acc (+ acc (vector-ref v i)))) 150 "sum vector elements")

;; ===========================================
;; List Iteration
;; ===========================================
(define lst '(1 2 3 4 5))

;; Basic list traversal with do loop
(assert-equal
 (do ((remaining lst (cdr remaining))) ((null? remaining) "done")
   (car remaining)) "done"
 "iterate over list")
;; Get list length
(assert-equal 5 (length lst) "length")
;; Access by index
(assert-equal 1 (list-ref lst 0) "list-ref at index 0")
(assert-equal 3 (list-ref lst 2) "list-ref at index 2")
(assert-equal 5 (list-ref lst 4) "list-ref at index 4")
;; Iterate with index counter
(assert-equal
 (do ((remaining lst (cdr remaining)) (i 0 (+ i 1))) ((null? remaining) "done")
   (car remaining))
 "done" "iterate over list with index counter")

;; ===========================================
;; Practical Examples: Filtering and Transformation
;; ===========================================
;; Collect all positive numbers from a list
(define numbers '(10 -5 3 -2 8 0 -1))

(define positive 'nil)

(assert-equal
 (do ((remaining numbers (cdr remaining))) ((null? remaining) positive)
   (let ((n (car remaining))) (if (> n 0) (set! positive (cons n positive)))))
 '(8 3 10) "filter positive numbers")

;; Count hash table entries with specific value
(define user_ht (make-hash-table))

(hash-set! user_ht "user1" "admin")
(hash-set! user_ht "user2" "user")
(hash-set! user_ht "user3" "admin")

(assert-equal 3 (hash-count user_ht) "user hash table has 3 entries")

(define admins 0)

(define entries2 (hash-entries user_ht))

(assert-equal
 (do ((remaining entries2 (cdr remaining))) ((null? remaining) admins)
   (let ((entry (car remaining)))
     (let ((key (car entry))
           (val (cdr entry)))
       (if (string=? val "admin") (set! admins (+ admins 1)))))) 2
 "count admins in hash table")

;; ===========================================
;; Empty Collections
;; ===========================================
(define empty_ht (make-hash-table))

(assert-equal 'nil (hash-keys empty_ht) "empty hash keys")
(assert-equal 'nil (hash-values empty_ht) "empty hash values")
(assert-equal 'nil (hash-entries empty_ht) "empty hash entries")
(assert-equal 0 (hash-count empty_ht) "empty hash count")

(define empty_list 'nil)

(assert-equal 0 (length empty_list) "empty list length")

(assert-true (null? empty_list) "empty list is null")

(define empty_vec #())

(assert-equal 0 (length empty_vec) "empty vector length")
