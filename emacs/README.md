# ditty-mode

Emacs major mode for editing [ditty](https://codeberg.org/thomasc/ditty) Lisp
source files. Bundled with ditty.

## Features

- Syntax highlighting for all 17 special forms (`define`, `set!`, `defmacro`,
  `do`, `case`, `condition-case`, `unwind-protect`, ...) and stdlib macros
  (`defun`, `when`, `unless`, `defvar`, ...)
- Built-in function highlighting (arithmetic, strings, lists, vectors, hash
  tables, regex, file I/O, packages, ...)
- Reader syntax: `pkg:symbol` package prefixes, `:keyword` literals, `#t`/`#f`
  booleans, `#\char` literals (`#\a`, `#\space`, `#\x41`, `#\uHHHH`)
- Definition-name highlighting for `defun`, `define`, `defmacro`, `defvar`,
  `defconst`, `defalias`
- Indentation hints for ditty forms (overrides `lisp-mode`'s Common-Lisp
  defaults)
- 8 customizable faces

## Build targets

From the ditty build directory:

```bash
make byte-compile  # byte-compile ditty-mode.el
make test          # run ERT test suite
make elisp-format  # indent elisp files
make install       # install to $(prefix)/share/emacs/site-lisp/ditty/
make clean         # remove byte-compiled artifacts
```

These are manual targets (not hooked into `make check`) because they require
Emacs at build time, which is not a configure dependency. The `EMACS` variable
selects the Emacs binary (default: `emacs`):

```bash
make test EMACS=/path/to/emacs
```

## Installation

### Via ditty install

```bash
cd chest/ditty
./autogen.sh && mkdir build && cd build
../configure --prefix=$HOME/.local
make install
```

Installs to `$(prefix)/share/emacs/site-lisp/ditty/`. Add to your init file:

```elisp
(add-to-list 'load-path
  (expand-file-name "share/emacs/site-lisp/ditty"
    (or (getenv "XDG_DATA_HOME") "~/.local/share")))
(require 'ditty-mode)
```

### Manual

Add the `emacs/` directory to your `load-path`:

```elisp
(add-to-list 'load-path "/path/to/chest/ditty/emacs")
(require 'ditty-mode)
```

### use-package :vc (Emacs 30+)

```elisp
(use-package ditty-mode
  :vc (:url "https://codeberg.org/thomasc/ditty"
      :branch "master" :lisp-dir "emacs"))
```

### straight.el

```elisp
(straight-use-package
 '(ditty-mode :type git :host nil
              :repo "https://codeberg.org/thomasc/ditty"
              :files "emacs/*.el"))
```

## Usage

The mode does not auto-activate by file extension. To activate manually:

```
M-x ditty-mode
```

To associate `.ditty` files automatically, add to your init file:

```elisp
(add-to-list 'auto-mode-alist '("\\.ditty\\'" . ditty-mode))
```

### Customization

All faces are customizable via `M-x customize-group RET ditty RET`.

## License

GPL-3.0-or-later. See [COPYING](COPYING).
