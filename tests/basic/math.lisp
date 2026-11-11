(load "tests/test-helpers.lisp")

;; abs
(assert-equal (abs -3) 3 "abs negative integer")

(assert-true (integer? (abs -3)) "abs integer stays integer")
(assert-true (approx= (abs -2.5) 2.5) "abs negative float")

(assert-false (integer? (abs -2.5)) "abs float stays float")

(assert-equal (abs 0) 0 "abs zero")

;; sqrt
(assert-true (approx= (sqrt 16) 4) "sqrt perfect square")

(assert-false (integer? (sqrt 16)) "sqrt returns a float")

(assert-true (approx= (sqrt 2) 1.4142135623730951) "sqrt two")

(assert-equal (sqrt 0) 0 "sqrt zero")

(assert-error (sqrt -1) "sqrt of a negative number")

;; sin/cos/tan
(assert-equal (sin 0) 0 "sin zero")
(assert-equal (cos 0) 1 "cos zero")

(assert-true (approx= (sin 1.5707963267948966) 1) "sin pi/2")
(assert-true (approx= (cos 3.141592653589793) -1) "cos pi")
(assert-true (approx= (sin 1000) 0.8268795405320025) "sin 1000")
(assert-true (approx= (tan 0.7853981633974483) 1) "tan pi/4")

;; asin/acos
(assert-true (approx= (asin 1) 1.5707963267948966) "asin one")

(assert-equal (acos 1) 0 "acos one")

(assert-true (approx= (acos -1) 3.141592653589793) "acos minus one")

(assert-error (asin 2) "asin out of range")
(assert-error (acos -2) "acos out of range")

;; atan/atan2
(assert-true (approx= (atan 1) 0.7853981633974483) "atan one")
(assert-true (approx= (atan 1 1) 0.7853981633974483) "atan two arguments")
(assert-true (approx= (atan 1 -1) 2.356194490192345) "atan quadrant two")
(assert-true (approx= (atan2 -1 -1) -2.356194490192345) "atan2 quadrant three")
(assert-true (approx= (atan2 0 -1) 3.141592653589793)
 "atan2 on the negative x axis")

;; exp/log
(assert-equal (exp 0) 1 "exp zero")

(assert-true (approx= (exp 1) 2.718281828459045) "exp one")

(assert-error (exp 1000) "exp overflow")

(assert-equal (log 1) 0 "log one")

(assert-true (approx= (log 2.718281828459045) 1) "log e")

(assert-error (log 0) "log zero")
(assert-error (log -1) "log of a negative number")

;; expt
(assert-equal (expt 2 10) 1024 "expt integer power")

(assert-true (integer? (expt 2 10)) "expt integer result stays integer")

(assert-equal (expt 2 0) 1 "expt zero exponent")
(assert-equal (expt 0 0) 1 "expt zero to the zero")
(assert-equal (expt -2 3) -8 "expt negative base")

(assert-true (approx= (expt 2 -1) 0.5) "expt negative exponent")
(assert-true (approx= (expt 2.0 3) 8) "expt float base")

(assert-false (integer? (expt 2.0 3)) "expt float base returns a float")

(assert-true (approx= (expt 4 0.5) 2) "expt fractional exponent")
;; 3^39 = 4052555153018976267 does not fit a double exactly: a float
;; fallback would make the low bit 0.
(assert-true (integer? (expt 3 39)) "expt stays exact above 2^53")

(assert-equal (logand (expt 3 39) 1) 1 "expt 3 39 is odd")

(assert-false (integer? (expt 2 63)) "expt overflow falls back to a float")

(assert-error (expt 0 -1) "expt zero to a negative power")
(assert-error (expt -8 0.5) "expt negative base fractional exponent")

;; float
(assert-equal (float 3) 3 "float of an integer")

(assert-false (integer? (float 3)) "float of an integer is a float")

(assert-equal (float 2.5) 2.5 "float of a float")

;; floor
(assert-equal (floor 7.5) 7 "floor of a positive float")
(assert-equal (floor -7.5) -8 "floor of a negative float")
(assert-equal (floor 7) 7 "floor of an integer")
(assert-equal (floor -0.5) -1 "floor of minus one half")
(assert-equal (floor 7.5 2) 3 "floor float division")
(assert-equal (floor -7.5 2) -4 "floor negative float division")
(assert-equal (floor 6.9 2.5) 2 "floor does not truncate operands")
(assert-equal (floor 7 2.0) 3 "floor mixed division")

(assert-true (integer? (floor 7.5)) "floor returns an integer")
(assert-true (integer? (floor 7.5 2)) "floor division returns an integer")
;; Integers above 2^53 must stay exact.
(assert-equal (logand (floor 9007199254740993 1) 1) 1 "floor integer exactness")

(define big (* 1000000000000.0 1000000000000.0 1000000000000.0))

(assert-false (integer? (floor big)) "floor of a huge float stays float")

(assert-equal (floor big) big "floor of a huge float is unchanged")

(assert-error (floor 7 0) "floor by integer zero")
(assert-error (floor 7.5 0) "floor by float zero")
(assert-error (floor 7.5 0.0) "floor by float zero float")
(assert-error (floor) "floor requires an argument")
(assert-error (floor 1 2 3) "floor takes at most two arguments")
(assert-error (floor "a") "floor of a string")

;; ceiling
(assert-equal (ceiling 1.2) 2 "ceiling positive")
(assert-equal (ceiling -1.2) -1 "ceiling negative")
(assert-equal (ceiling 3) 3 "ceiling of an integer")

;; truncate
(assert-equal (truncate 1.9) 1 "truncate positive")
(assert-equal (truncate -1.9) -1 "truncate negative")

;; round
(assert-equal (round 2.5) 2 "round ties to even (down)")
(assert-equal (round 3.5) 4 "round ties to even (up)")
(assert-equal (round -2.5) -2 "round negative ties to even")
(assert-equal (round 1.4) 1 "round down")
(assert-equal (round 1.6) 2 "round up")
(assert-equal (round 5) 5 "round of an integer")

;; arity and type errors
(assert-error (sqrt) "sqrt requires an argument")
(assert-error (sqrt 1 2) "sqrt takes one argument")
(assert-error (sin "x") "sin of a string")
(assert-error (abs 'a) "abs of a symbol")

;; random
(assert-equal (random 1) 0 "random with bound one")

(do ((i 0 (+ i 1))) ((>= i 200))
  (let ((n (random 10)))
    (assert-true (integer? n) "random integer type")
    (assert-true (and (>= n 0) (< n 10)) "random integer range")))
(do ((i 0 (+ i 1))) ((>= i 200))
  (let ((x (random 1.0)))
    (assert-false (integer? x) "random float type")
    (assert-true (and (>= x 0) (< x 1)) "random float range")))

(assert-error (random 0) "random zero bound")
(assert-error (random -5) "random negative bound")
(assert-error (random "a") "random of a string")

(random-seed 42)

(define random-a (random 1000))

(random-seed 42)

(assert-equal (random 1000) random-a "same seed, same value")
