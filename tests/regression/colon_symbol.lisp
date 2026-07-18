;; Regression test for package-qualified symbols (pkg:sym syntax)
;; Bug: the reader desugared pkg:sym into a (package-ref "pkg" sym) cons cell,
;; which broke define/set!/defun for any name containing a colon (e.g., a:b,
;; input:foo) because those special forms require a LISP_SYMBOL, not a list.
;; It also broke forward references: (progn (require 'lib) lib:val) reads the
;; whole progn before require runs, so lib:val could not see the package that
;; require would create.
;;
;; Fix: the reader now produces a single LISP_SYMBOL with a `namespace` field
;; (Clojure-style). The evaluator resolves qualified symbols at eval time via
;; env_lookup_in_package, so forward references work and colon-containing names
;; are proper symbols that define/set!/defun accept.
(load "tests/test-helpers.lisp")

;; define/set!/defun accept names containing colons (they are plain symbols
;; with a namespace field; define stores the binding under the qualified symbol
;; itself, so env_lookup finds it directly).
(defun a:b (x)
  (define b x)
  b)

(assert-equal 42 (a:b 42) "defun with colon symbol works")

(define input:foo "hello")

(assert-equal "hello" input:foo "define with colon symbol works")

(set! input:foo "world")

(assert-equal "world" input:foo "set! with colon symbol works")

;; Package-qualified access resolves cross-package bindings at eval time.
(in-package 'math)

(define math-add (lambda (a b) (+ a b)))

(in-package 'user)

(assert-equal 5 (math:math-add 2 3) "package-qualified access works")

;; core:+ still resolves to the builtin
(assert-equal 30 (core:+ 10 20) "core:+ resolves to addition")

;; Forward reference inside a progn: the package is created by require before
;; the next expression is evaluated, so the qualified symbol resolves.
(unwind-protect
  (progn (define lib-file (open "test-req-lib-colon.lisp" "w"))
    (write-line lib-file "(in-package 'test-req-lib-colon)")
    (write-line lib-file "(define req-lib-val 42)")
    (write-line lib-file "(provide 'test-req-lib-colon)")
    (close lib-file)
    (require 'test-req-lib-colon)
    (assert-equal 42 test-req-lib-colon:req-lib-val
     "forward reference to require'd package works"))
  (progn (delete-file "test-req-lib-colon.lisp")))

;; Unknown package:symbol is an undefined symbol error
(assert-error (eval 'unknownpkg:nonexistent)
 "undefined pkg:symbol signals error")

;; Printing a qualified symbol round-trips through the reader
(assert-equal "math:math-add"
 (symbol->string 'math:math-add)
 "symbol->string includes namespace")
