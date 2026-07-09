# Packages

Package (namespace) and library management.

## `in-package`

Set the current package.

### Parameters

- `name` - Package name (symbol). Strings also accepted for convenience.

### Returns

The package name as a symbol.

### Examples

```lisp
(in-package 'math)
(in-package 'user)
```

## `current-package`

Return the current package name as a symbol.

### Examples

```lisp
(current-package) ; => user
```

## `package-symbols`

Return an alist of bindings in the named package.

### Parameters

- `name` - Package name (symbol). Strings also accepted for convenience.

### Returns

An alist of `(symbol . value)` pairs.

### Examples

```lisp
(package-symbols 'user) ; => ((x . 42) ...)
```

## `list-packages`

Return a list of distinct package names as symbols.

### Examples

```lisp
(list-packages) ; => (core user)
```

## `package-save`

Save package bindings to a file as valid Lisp source.

### Parameters

- `filename` - Path to write (string)
- `package` (optional) - Package name (symbol). Defaults to current package.
- `:defun` (optional) - Extract lambdas into named `defun` forms for readability.
- `:format` (optional) - Pretty-print the output (requires lisp-fmt.lisp to be loaded).

### Returns

`#t` on success.

### Examples

```lisp
(define x 42)
(package-save "my-pkg.lisp")
(package-save "math.lisp" 'math)
(package-save "my-pkg.lisp" :defun)
(package-save "math.lisp" 'math :defun)
(package-save "my-pkg.lisp" :defun :format)
```

### Notes

The output file can be loaded with `(load filename)`. Strings are also accepted
for the package name and converted to symbols internally.

With `:defun`, top-level lambdas emit as `(defun name ...)` and nested lambdas
are extracted into separate defun forms referenced by name.

With `:format`, the output is pretty-printed using `format-file` from lisp-fmt.lisp.
Load lisp-fmt.lisp first: `(load "lisp/lisp-fmt.lisp")`.

## `provide`

Register a feature as loaded by adding it to `*features*`.

Calling `provide` multiple times with the same feature is idempotent —
the feature is only added once.

### Parameters

- `feature` - Feature name (symbol). Strings also accepted.

### Returns

The feature symbol.

### Examples

```lisp
(provide 'mylib)
(provide "mylib") ; string also accepted
```

## `require`

Load a library if it has not already been loaded.

Checks if the feature is already in `*features*`. If so, returns immediately.
Otherwise, searches load paths for the library file, loads it (which should
call `provide`), and verifies the feature was provided.

### Load Path Search Order

1. Current directory (`name.lisp`, then `name/name.lisp`)
2. Directories in `DITTY_LISP_PATH` (colon-separated on Unix, semicolon on Windows)
3. `$XDG_DATA_HOME/ditty/lisp/` (default: `~/.local/share/ditty/lisp/`)
4. Each dir in `$XDG_DATA_DIRS/ditty/lisp/` (default: `/usr/local/share:/usr/share`)

On Windows: `%APPDATA%\ditty\lisp\` (user) and exe-relative `..\share\ditty\lisp\` (system).

### Parameters

- `feature` - Feature name (symbol). Strings also accepted.

### Returns

The feature symbol on success.

### Errors

- If the library file cannot be found.
- If the file loads but does not call `(provide 'feature)` for the expected feature.

### Examples

```lisp
(require 'mylib)
(require "mylib") ; string also accepted
```

### Notes

`require` saves and restores `*package*`, so a library's `in-package` does not
leak to the caller. Transitive dependencies work naturally: if `mylib` calls
`(require 'utils)` at its top, loading `mylib` automatically pulls in `utils`.

## `provided?`

Check if a feature has been provided (is in `*features*`).

### Parameters

- `feature` - Feature name (symbol). Strings also accepted.

### Returns

`#t` if the feature is loaded, `nil` otherwise.

### Examples

```lisp
(provided? 'mylib) ; => #t if (provide 'mylib) or (require 'mylib) was called
(provided? 'nonexistent) ; => nil
```

## `export`

Mark symbols as exported from the current package.

Exported symbols are the public API of a package. Non-exported symbols are
internal and only accessible via `pkg:symbol` qualified access.

If `export` is never called for a package, `use-package` imports all bindings
(default = all exported).

### Parameters

- `symbol` ... - One or more quoted symbols to export.

### Returns

`#t` on success.

### Examples

```lisp
(in-package 'mylib)
(export 'greet 'farewell)
(defun greet (name) (concat "Hello, " name))
(defun farewell (name) (concat "Goodbye, " name))
(defun internal-helper () "secret") ; not exported
(provide 'mylib)
```

## `package-exports`

Return the list of exported symbols for a package.

### Parameters

- `name` - Package name (symbol). Strings also accepted.

### Returns

A list of exported symbols, or `nil` if no explicit exports were set.

### Examples

```lisp
(package-exports 'mylib) ; => (greet farewell)
(package-exports 'core) ; => nil (no explicit exports)
```

### Notes

Returns `nil` when no `export` has been called for the package. In this case,
`use-package` treats all bindings as exported (Emacs Lisp convention).

## `use-package`

Import exported symbols from a package into the current package.

After `use-package`, exported symbols from the source package are accessible
unqualified in the current package. Non-exported symbols remain accessible
only via `pkg:symbol` qualified syntax.

If the source package has no explicit `export` list, all bindings are imported.

### Parameters

- `name` - Package name (symbol). Strings also accepted.

### Returns

`#t` on success.

### Errors

- If the named package does not exist (has no bindings).

### Examples

```lisp
(require 'mylib)
(use-package 'mylib)
(greet "World") ; now accessible unqualified
```

## `*features*`

A list of loaded feature symbols. Updated by `provide` and checked by `require`.
Initially `nil`.

```lisp
*features* ; => (mylib utils lisp-fmt)
```

## `*load-path*`

A read-only list of directory strings where `require` searches for libraries.
Populated at startup from `DITTY_LISP_PATH` and XDG data directories.

```lisp
*load-path* ; => ("/home/me/.local/share/ditty/lisp" "/usr/local/share/ditty/lisp" ...)
```

## `*command-line-args*`

A list of command-line arguments passed to the script after the `--` separator.
Only defined when `--` is present on the command line; use `(bound? '*command-line-args*)` to check.

```lisp
; Run: ditty script.lisp -- arg1 arg2 arg3
(if (bound? '*command-line-args*)
    (format #t "Args: ~A~%" *command-line-args*)
    (format #t "No arguments~%"))

; Iterate over arguments
(when (bound? '*command-line-args*)
  (do ((args *command-line-args* (cdr args)))
    ((null? args))
    (format #t "Arg: ~A~%" (car args))))
```

Arguments are strings in the order they appeared. Files before `--` are executed; arguments after `--` are passed to the last script.

## Package Aliases

The following aliases are defined in the standard library for tab-completion discoverability. They resolve to the same function via `defalias`.

- `package-set` - Alias for `in-package`
- `package-current` - Alias for `current-package`
- `package-list` - Alias for `list-packages`
