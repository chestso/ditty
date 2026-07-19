;; Fixture library for the script-dir require integration test.
;; Sits next to a "main" script under tests/fixtures/app_lib/ so that
;; (require 'app-helper) finds it via the entry script's directory being
;; on *load-path*.
(in-package 'app-helper)

(define app-helper-version "1.0")

(define (app-helper-add a b) (+ a b))

(provide 'app-helper)
