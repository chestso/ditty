;; Fixture library for the *load-path* mutation integration test.
;; Lives in tests/fixtures/extra_lib/, which is NOT the directory of the
;; running test file. The test prepends this directory to *load-path*
;; and then (require 'extra-lib) must succeed purely via the mutation.
(in-package 'extra-lib)

(define extra-lib-value 4242)

(provide 'extra-lib)
