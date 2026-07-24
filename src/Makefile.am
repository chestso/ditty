# Build static libraries
lib_LIBRARIES = libditty.a

# Generated headers: docstrings (from markdown) and version (from git state).
# ditty_version.h is regenerated on every `make` so the embedded version
# string tracks the current git HEAD (including a -dirty suffix when the
# working tree has uncommitted changes). The recipe always runs, but the
# file is only rewritten when the version actually changes — so dependent
# .o files stay cached when nothing changed.
BUILT_SOURCES = docstrings.gen.h ditty_version.h
CLEANFILES = docstrings.gen.h ditty_version.h
nodist_libditty_a_SOURCES = docstrings.gen.h ditty_version.h
EXTRA_DIST = builtins_internal.h

.PHONY: ditty_version_force
ditty_version.h: ditty_version_force
	@v=`"$(top_srcdir)/build-aux/git-version.sh" "$(top_srcdir)" 2>/dev/null`; \
	test -n "$$v" || v="$(DITTY_VERSION)"; \
	test -n "$$v" || v="unknown"; \
	base=`echo "$$v" | sed 's/-.*//'`; \
	major=`echo "$$base" | cut -d. -f1`; \
	minor=`echo "$$base" | cut -d. -f2`; \
	patch=`echo "$$base" | cut -d. -f3`; \
	printf '#ifndef DITTY_VERSION_H\n#define DITTY_VERSION_H\n#define DITTY_VERSION "%s"\n#define DITTY_VERSION_MAJOR %s\n#define DITTY_VERSION_MINOR %s\n#define DITTY_VERSION_PATCH %s\n#endif\n' "$$v" "$$major" "$$minor" "$$patch" > $@.tmp; \
	if cmp -s $@.tmp $@ 2>/dev/null; then \
	  rm -f $@.tmp; \
	else \
	  mv $@.tmp $@; \
	  echo "  GEN      $@ ($$v)"; \
	fi
ditty_version_force:

DOC_FILES = \
    $(top_srcdir)/docstrings/arithmetic.md \
    $(top_srcdir)/docstrings/characters.md \
    $(top_srcdir)/docstrings/comparison.md \
    $(top_srcdir)/docstrings/errors.md \
    $(top_srcdir)/docstrings/file-io.md \
    $(top_srcdir)/docstrings/filesystem.md \
    $(top_srcdir)/docstrings/functions.md \
    $(top_srcdir)/docstrings/hash-tables.md \
    $(top_srcdir)/docstrings/lists.md \
    $(top_srcdir)/docstrings/packages.md \
    $(top_srcdir)/docstrings/printing.md \
    $(top_srcdir)/docstrings/regex.md \
    $(top_srcdir)/docstrings/special-forms.md \
    $(top_srcdir)/docstrings/string-ports.md \
    $(top_srcdir)/docstrings/strings.md \
    $(top_srcdir)/docstrings/symbols.md \
    $(top_srcdir)/docstrings/time-and-profiling.md \
    $(top_srcdir)/docstrings/tui.md \
    $(top_srcdir)/docstrings/type-predicates.md \
    $(top_srcdir)/docstrings/vectors.md

docstrings.gen.h: $(top_srcdir)/scripts/gen-docstrings.sh $(DOC_FILES)
	$(top_srcdir)/scripts/gen-docstrings.sh $(top_srcdir)/docstrings > $@

# Main lisp library
libditty_a_SOURCES = \
    lisp.c \
    reader.c \
    eval.c \
    print.c \
    env.c \
    builtins.c \
    builtins_arithmetic.c \
    builtins_comparison.c \
    builtins_strings.c \
    builtins_characters.c \
    builtins_lists.c \
    builtins_vectors.c \
    builtins_hash_tables.c \
    builtins_regex.c \
    builtins_file_io.c \
    builtins_string_ports.c \
    builtins_type_predicates.c \
    builtins_symbols.c \
    builtins_functions.c \
    builtins_printing.c \
    builtins_errors.c \
    builtins_packages.c \
    builtins_filesystem.c \
    builtins_time_profiling.c \
    builtins_tui.c \
    utf8.c \
    regex.c \
    hash_table.c \
    file_utils.c \
    flare_lexer_ditty.c \
    flare_lexer_ditty_classifier.c \
    flare_lexer_commonmark.c \
    flare_token_type.c \
    flare_style.c \
    flare_style_monokai.c \
    flare_style_dracula.c \
    flare_style_github_dark.c \
    flare_style_github_light.c \
    flare_formatter.c \
    flare_formatter_terminal.c \
    flare_terminal.c \
    flare_color.c \
    flare_highlight.c \
    flare_source.c \
    flare_writer.c \
    flare_token_source.c \
    flare_iterator_reflow.c

libditty_a_CPPFLAGS = \
    -I$(top_srcdir)/include \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)
