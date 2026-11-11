;; Test Assertion Macros for telnet-lisp Test Suite
;; Provides assertion macros for test files: assert-equal, assert-true, assert-false,
;; assert-error, and assert-nil, plus the approx= float comparison helper.
;; All macros abort tests via (error ...) on failure.
;; Assert that actual equals expected (handles numbers and structural equality)
;; Usage: (assert-equal expected actual "description")
;; Returns: nil on success, aborts test with error on failure
(defmacro assert-equal (expected actual message)
  `(let ((expected-val ,expected)
         (actual-val ,actual))
     ;; Use = for numbers, equal? for everything else
     (let ((values-equal
            (if (and (number? actual-val) (number? expected-val))
              (= actual-val expected-val)
              (equal? actual-val expected-val))))
       (if values-equal
         nil ; Success: silent
         (error
          (format nil "Assertion failed: ~A~%  Expected: ~S~%  Actual:   ~S"
           ,message expected-val actual-val))))))

;; Assert that condition evaluates to truthy value (anything except nil/#f)
;; Usage: (assert-true condition "description")
;; Returns: nil on success, aborts test with error on failure
(defmacro assert-true (condition message)
  `(let ((result ,condition))
     (if result
       nil ; Success: silent
       (error
        (format nil "Assertion failed: ~A (expected truthy, got: ~S)" ,message
         result)))))

;; Assert that condition evaluates to falsy value (nil or #f)
;; Usage: (assert-false condition "description")
;; Returns: nil on success, aborts test with error on failure
(defmacro assert-false (condition message)
  `(let ((result ,condition))
     (if result
       (error
        (format nil "Assertion failed: ~A (expected falsy, got: ~S)" ,message
         result))
       nil))) ; Success: silent

;; Assert that expression throws an error
;; Usage: (assert-error expr "description")
;; Returns: nil on success (if error thrown), aborts test if expr succeeds
;; The expression may have any shape — a signal unwinds to this
;; condition-case from wherever it was raised, so a body consisting of
;; several forms (or a call into user code) is fine. The failure
;; (error ...) is raised outside the condition-case so the handler cannot
;; swallow it.
(defmacro assert-error (expr message)
  `(let ((outcome (condition-case err ,expr (error 'assert-error-raised))))
     (if (eq? outcome 'assert-error-raised)
       nil
       (error
        (format nil "Assertion failed: ~A (expected error but succeeded)"
         ,message)))))

;; True when A and B differ by less than 1e-9 (for float results)
;; Usage: (assert-true (approx= expected actual) "description")
(defun approx= (a b) (< (abs (- a b)) 0.000000001))

;; Assert that expression evaluates to nil explicitly
;; Usage: (assert-nil expr "description")
;; Returns: nil on success, aborts test with error on failure
(defmacro assert-nil (expr message)
  `(let ((result ,expr))
     (if (null? result)
       nil ; Success
       (error
        (format nil "Assertion failed: ~A (expected nil, got: ~S)" ,message
         result)))))
