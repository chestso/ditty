(load "tests/test-helpers.lisp")

;; directory-file-name / file-name-directory
(assert-equal "/home/user/" (directory-file-name "/home/user/file.txt")
 "directory-file-name absolute")
(assert-equal nil (directory-file-name "file.txt") "directory-file-name no dir")
(assert-equal "/home/user/" (file-name-directory "/home/user/file.txt")
 "file-name-directory alias")

;; file-name-nondirectory
(assert-equal "file.txt" (file-name-nondirectory "/home/user/file.txt")
 "file-name-nondirectory absolute")
(assert-equal "file.txt" (file-name-nondirectory "file.txt")
 "file-name-nondirectory relative")
(assert-equal "" (file-name-nondirectory "/home/user/")
 "file-name-nondirectory trailing slash")

;; Error cases
(assert-error (directory-file-name 123) "directory-file-name non-string")
(assert-error (file-name-nondirectory 123) "file-name-nondirectory non-string")
