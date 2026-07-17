(load "tests/test-helpers.lisp")

;; directory-file-name / file-name-directory
(assert-equal (directory-file-name "/home/user/file.txt") "/home/user/"
 "directory-file-name absolute")
(assert-equal (directory-file-name "file.txt") nil "directory-file-name no dir")
(assert-equal (file-name-directory "/home/user/file.txt") "/home/user/"
 "file-name-directory alias")

;; file-name-nondirectory
(assert-equal (file-name-nondirectory "/home/user/file.txt") "file.txt"
 "file-name-nondirectory absolute")
(assert-equal (file-name-nondirectory "file.txt") "file.txt"
 "file-name-nondirectory relative")
(assert-equal (file-name-nondirectory "/home/user/") ""
 "file-name-nondirectory trailing slash")

;; Error cases
(assert-error (directory-file-name 123) "directory-file-name non-string")
(assert-error (file-name-nondirectory 123) "file-name-nondirectory non-string")
