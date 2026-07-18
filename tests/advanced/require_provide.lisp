;; Test for require/provide library system
(load "tests/test-helpers.lisp")

;; ===========================================
;; *features* starts as nil (or empty list)
;; ===========================================
(assert-true (list? *features*) "*features* is a list")

;; ===========================================
;; provide adds a feature to *features*
;; ===========================================
(provide 'test-lib-a)

(assert-true (memq 'test-lib-a *features*) "provide adds feature to *features*")

;; ===========================================
;; provide is idempotent (calling twice doesn't duplicate)
;; ===========================================
(provide 'test-lib-a)

(let ((count 0)
      (rest *features*))
  (do () ((null? rest))
    (when (eq? (car rest) 'test-lib-a) (set! count (+ count 1)))
    (set! rest (cdr rest)))
  (assert-equal 1 count "provide is idempotent"))

;; ===========================================
;; provided? returns #t for provided features
;; ===========================================
(assert-true (provided? 'test-lib-a)
 "provided? returns #t for provided feature")

;; ===========================================
;; provided? returns nil for unprovided features
;; ===========================================
(assert-nil (provided? 'nonexistent-feature)
 "provided? returns nil for unknown feature")

;; ===========================================
;; require returns immediately if already provided
;; ===========================================
(assert-equal 'test-lib-a (require 'test-lib-a)
 "require returns feature symbol when already loaded")

;; ===========================================
;; require loads a library file and checks provide was called
;; ===========================================
(unwind-protect
  (progn
    ;; Create a simple library file (filename must match feature name)
    (define lib-file (open "test-req-lib.lisp" "w"))
    (write-line lib-file "(in-package 'test-req-lib)")
    (write-line lib-file "(define req-lib-val 42)")
    (write-line lib-file "(provide 'test-req-lib)")
    (close lib-file)
    ;; require should load it
    (assert-equal 'test-req-lib (require 'test-req-lib)
     "require loads library and returns feature symbol")
    ;; Library binding should be accessible via package-qualified access
    (assert-equal 42 test-req-lib:req-lib-val
     "library binding accessible after require")
    ;; provided? should now return true
    (assert-true (provided? 'test-req-lib)
     "provided? returns #t after require loads library"))
  (progn (delete-file "test-req-lib.lisp")))

;; ===========================================
;; require does not reload if already provided
;; ===========================================
(unwind-protect
  (progn
    ;; Create a library
    (define lib-file2 (open "test-req-lib2.lisp" "w"))
    (write-line lib-file2 "(define req-lib2-counter 0)")
    (write-line lib-file2 "(set! req-lib2-counter (+ req-lib2-counter 1))")
    (write-line lib-file2 "(provide 'test-req-lib2)")
    (close lib-file2)
    ;; First require loads it
    (require 'test-req-lib2)
    (assert-equal 1 req-lib2-counter "first require loads library")
    ;; Second require should NOT reload
    (require 'test-req-lib2)
    (assert-equal 1 req-lib2-counter "second require does not reload"))
  (progn (delete-file "test-req-lib2.lisp")))

;; ===========================================
;; require handles transitive dependencies
;; ===========================================
(unwind-protect
  (progn
    ;; Create a dependency library
    (define dep-file (open "test-dep.lisp" "w"))
    (write-line dep-file "(in-package 'test-dep)")
    (write-line dep-file "(define dep-val 99)")
    (write-line dep-file "(provide 'test-dep)")
    (close dep-file)
    ;; Create a main library that requires the dep
    (define main-file (open "test-main-lib.lisp" "w"))
    (write-line main-file "(require 'test-dep)")
    (write-line main-file "(in-package 'test-main-lib)")
    (write-line main-file "(define main-val (+ test-dep:dep-val 1))")
    (write-line main-file "(provide 'test-main-lib)")
    (close main-file)
    ;; Require the main library — should transitively load dep
    (require 'test-main-lib)
    (assert-true (provided? 'test-dep) "transitive dependency loaded")
    (assert-true (provided? 'test-main-lib) "main library loaded")
    (assert-equal 100 test-main-lib:main-val
     "transitive dependency values accessible"))
  (progn (delete-file "test-dep.lisp") (delete-file "test-main-lib.lisp")))

;; ===========================================
;; require errors if library file not found
;; ===========================================
(assert-error (require 'nonexistent-library-xyz)
 "require errors on missing library")

;; ===========================================
;; require errors if file loads but doesn't provide the feature
;; ===========================================
(unwind-protect
  (progn (define no-provide-file (open "no-provide-test.lisp" "w"))
    (write-line no-provide-file "(define some-var 123)")
    (close no-provide-file)
    (assert-error (require 'no-provide-test)
     "require errors when file doesn't provide expected feature"))
  (progn (delete-file "no-provide-test.lisp")))

;; ===========================================
;; require saves/restores *package*
;; ===========================================
(unwind-protect
  (progn (define pkg-file (open "pkg-test-feature.lisp" "w"))
    (write-line pkg-file "(in-package 'pkg-test-ns)")
    (write-line pkg-file "(define pkg-test-var 77)")
    (write-line pkg-file "(provide 'pkg-test-feature)")
    (close pkg-file)
    ;; We're in user package
    (assert-equal 'user (current-package) "in user package before require")
    ;; require a library that switches package
    (require 'pkg-test-feature)
    ;; *package* should be restored to user
    (assert-equal 'user (current-package)
     "require restores *package* after loading"))
  (progn (delete-file "pkg-test-feature.lisp")))

;; ===========================================
;; require accepts string argument (converted to symbol)
;; ===========================================
(unwind-protect
  (progn (define str-lib-file (open "str-test-lib.lisp" "w"))
    (write-line str-lib-file "(define str-test-val 55)")
    (write-line str-lib-file "(provide 'str-test-lib)")
    (close str-lib-file)
    ;; require with string should work
    (assert-equal 'str-test-lib (require "str-test-lib")
     "require accepts string argument"))
  (progn (delete-file "str-test-lib.lisp")))

;; ===========================================
;; Directory-based library resolution: name/name.lisp
;; ===========================================
(unwind-protect
  (progn
    ;; Create a directory with a library entry point
    (mkdir "dir-test-lib")
    (define dir-lib-file (open "dir-test-lib/dir-test-lib.lisp" "w"))
    (write-line dir-lib-file "(define dir-lib-val 33)")
    (write-line dir-lib-file "(provide 'dir-test-lib)")
    (close dir-lib-file)
    ;; require should find dir-test-lib/dir-test-lib.lisp
    (require 'dir-test-lib)
    (assert-true (provided? 'dir-test-lib)
     "require resolves directory-based library")
    (assert-equal 33 dir-lib-val
     "directory-based library bindings accessible"))
  (progn (delete-file "dir-test-lib/dir-test-lib.lisp")
    (delete-directory "dir-test-lib")))

;; ===========================================
;; export marks symbols as exported in a package
;; ===========================================
(in-package 'export-test)

(export 'greet 'farewell)

(defun greet (name) (concat "Hello, " name))

(defun farewell (name) (concat "Goodbye, " name))

(defun internal-helper () "secret")

(provide 'export-test)

(in-package 'user)

(assert-true (provided? 'export-test) "export-test package provided")

;; ===========================================
;; package-exports returns the exported symbols list
;; ===========================================
(let ((exports (package-exports 'export-test)))
  (assert-true (list? exports) "package-exports returns a list")
  (assert-true (memq 'greet exports) "package-exports includes greet")
  (assert-true (memq 'farewell exports) "package-exports includes farewell")
  (assert-false (memq 'internal-helper exports)
   "package-exports excludes non-exported symbols"))

;; ===========================================
;; use-package imports exported symbols into current package
;; ===========================================
(use-package 'export-test)

(assert-equal "Hello, World" (greet "World")
 "use-package brings exported function into current scope")
(assert-equal "Goodbye, World" (farewell "World")
 "use-package brings all exported functions")

;; ===========================================
;; use-package with no explicit exports imports all bindings
;; ===========================================
(in-package 'no-export-pkg)

(define no-export-val 123)

(defun no-export-fn (x) (* x 2))

(provide 'no-export-pkg)

(in-package 'user)

(use-package 'no-export-pkg)

(assert-equal 123 no-export-val
 "use-package imports all bindings when no exports defined")
(assert-equal 10 (no-export-fn 5)
 "use-package imports all functions when no exports defined")

;; ===========================================
;; use-package errors on unknown package
;; ===========================================
(assert-error (use-package 'nonexistent-pkg-xyz)
 "use-package errors on unknown package")

;; ===========================================
;; *load-path* is a list of strings
;; ===========================================
(assert-true (list? *load-path*) "*load-path* is a list")

(let ((all-strings #t)
      (rest *load-path*))
  (do () ((null? rest))
    (unless (string? (car rest)) (set! all-strings nil))
    (set! rest (cdr rest)))
  (assert-true all-strings "*load-path* contains only strings"))
