;; Integration tests for require's load-path resolution.
;;
;; Covers two behaviors introduced by the load-path rework:
;;   1. The entry script's directory is prepended to *load-path* by the CLI,
;;      so (require 'x) finds a sibling library next to the script regardless
;;      of the current working directory (run-test.sh runs us from the
;;      project root, NOT from tests/advanced/).
;;   2. Mutating *load-path* at runtime is honored by require, so an app
;;      can add plugin / per-user library directories programmatically.
;;
;; Both tests rely on *load-pathname* / *load-path* to locate the script's
;; own directory at runtime, so they are independent of where they are
;; invoked from.
(load "tests/test-helpers.lisp")

;; -------------------------------------------
;; Sanity: *load-path* is a non-empty list whose first element is the
;; directory of this script (absolute, since the CLI resolves it).
;; -------------------------------------------
(assert-true (list? *load-path*) "*load-path* is a list")
(assert-true (not (null? *load-path*)) "*load-path* is non-empty")
(assert-true (string? (car *load-path*))
 "*load-path* first element is a string")

;; The script dir is the directory of *load-pathname* (this file's abs path).
;; file-name-directory returns a trailing slash; (car *load-path*) does not,
;; so compare by checking that the pathname starts with the load-path entry.
(define script-dir (car *load-path*))

(define script-dir-len (length script-dir))

(define pathname-prefix (substring *load-pathname* 0 script-dir-len))

(assert-true (string=? script-dir pathname-prefix)
 "*load-path* first entry is this script's directory")

(unwind-protect
  (progn
    ;; ===========================================
    ;; Test 1: require finds a sibling library via the script's own
    ;; directory being on *load-path*. We write app-helper.lisp next to
    ;; this script (into script-dir) at runtime, then require it. CWD is
    ;; the project root, so a CWD-based search would NOT find it; only the
    ;; script-dir entry on *load-path* can.
    ;; ===========================================
    (define helper-path (concat script-dir "/app-helper.lisp"))
    (define f (open helper-path "w"))
    (write-line f "(in-package 'app-helper)")
    (write-line f "(define app-helper-version \"1.0\")")
    (write-line f "(define app-helper-add (lambda (a b) (+ a b)))")
    (write-line f "(provide 'app-helper)")
    (close f)
    (assert-equal 'app-helper (require 'app-helper)
     "require finds sibling lib via script dir on *load-path*")
    (assert-true (provided? 'app-helper)
     "provide was called by the required sibling lib")
    (assert-equal "1.0" app-helper:app-helper-version
     "required sibling lib binding is accessible (version)")
    (assert-equal 42 (app-helper:app-helper-add 40 2)
     "required sibling lib function works")
    ;; ===========================================
    ;; Test 2: mutating *load-path* at runtime is honored by require.
    ;; tests/fixtures/extra_lib/extra-lib.lisp is a committed fixture that
    ;; is NOT in this script's directory and NOT on the env-var search path,
    ;; so the only way require can find it is if we prepend its directory
    ;; to *load-path*.
    ;; ===========================================
    (define extra-dir (concat script-dir "/../fixtures/extra_lib"))
    (set! *load-path* (cons extra-dir *load-path*))
    (assert-equal 'extra-lib (require 'extra-lib)
     "require honors a runtime-mutated *load-path* entry")
    (assert-true (provided? 'extra-lib)
     "provide was called by the mutated-path lib")
    (assert-equal 4242 extra-lib:extra-lib-value
     "mutated-path lib binding is accessible"))
  ;; ============================================
  ;; Cleanup: remove the temp helper file written into the script dir.
  ;; ============================================
  (progn (delete-file (concat script-dir "/app-helper.lisp"))))
