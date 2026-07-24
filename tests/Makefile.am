# Test suite using Automake's parallel test harness
# C unit tests (compiled, run as plain executables)
FLARE_TESTS = \
    test_token_type \
    test_color \
    test_style \
    test_flare \
    test_flare_lexer_ditty_classifier \
    test_flare_lexer_ditty \
    test_flare_lexer_commonmark

check_PROGRAMS = test_sf_kind test_repl_completeness test_auto_indent test_file_utils $(FLARE_TESTS)
if HAVE_BOBA
check_PROGRAMS += test_repl_app
endif

test_sf_kind_SOURCES = test_sf_kind.c
test_sf_kind_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)
test_sf_kind_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

test_file_utils_SOURCES = test_file_utils.c
test_file_utils_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)
test_file_utils_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

# Flare unit tests
AM_FLARE_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)

AM_FLARE_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

test_token_type_SOURCES = test_token_type.c
test_token_type_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_token_type_LDADD = $(AM_FLARE_LDADD)

test_color_SOURCES = test_color.c
test_color_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_color_LDADD = $(AM_FLARE_LDADD)

test_style_SOURCES = test_style.c
test_style_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_style_LDADD = $(AM_FLARE_LDADD)

test_flare_SOURCES = test_flare.c
test_flare_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_flare_LDADD = $(AM_FLARE_LDADD)

test_flare_lexer_ditty_classifier_SOURCES = test_flare_lexer_ditty_classifier.c
test_flare_lexer_ditty_classifier_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_flare_lexer_ditty_classifier_LDADD = $(AM_FLARE_LDADD)

test_flare_lexer_ditty_SOURCES = test_flare_lexer_ditty.c
test_flare_lexer_ditty_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_flare_lexer_ditty_LDADD = $(AM_FLARE_LDADD)

test_flare_lexer_commonmark_SOURCES = test_flare_lexer_commonmark.c
test_flare_lexer_commonmark_CPPFLAGS = $(AM_FLARE_CPPFLAGS)
test_flare_lexer_commonmark_LDADD = $(AM_FLARE_LDADD)

test_repl_completeness_SOURCES = test_repl_completeness.c
test_repl_completeness_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)
test_repl_completeness_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

test_auto_indent_SOURCES = test_auto_indent.c
test_auto_indent_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS)
test_auto_indent_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS)

test_repl_app_SOURCES = test_repl_app.c ../cli/repl_app.c
test_repl_app_CPPFLAGS = \
    -I$(top_srcdir)/include \
    -I$(top_srcdir)/src \
    -I$(top_builddir)/src \
    -I$(top_srcdir)/cli \
    $(GC_CFLAGS) \
    $(PCRE2_CFLAGS) \
    $(BOBA_CFLAGS)
test_repl_app_LDADD = \
    $(top_builddir)/src/libditty.a \
    $(GC_LIBS) \
    $(PCRE2_LIBS) \
    $(BOBA_LIBS)

# Force ditty_version.h to exist before test objects compile
$(test_sf_kind_OBJECTS): $(top_builddir)/src/ditty_version.h
$(test_repl_completeness_OBJECTS): $(top_builddir)/src/ditty_version.h
$(test_auto_indent_OBJECTS): $(top_builddir)/src/ditty_version.h
$(test_file_utils_OBJECTS): $(top_builddir)/src/ditty_version.h
if HAVE_BOBA
$(test_repl_app_OBJECTS): $(top_builddir)/src/ditty_version.h
endif
$(FLARE_TESTS_OBJECTS): $(top_builddir)/src/ditty_version.h

BOBA_TESTS =
if HAVE_BOBA
BOBA_TESTS += test_repl_app
endif

# Lisp script tests
TESTS = \
    basic/alists.lisp \
    basic/apply.lisp \
    basic/and_or_list.lisp \
    basic/characters.lisp \
    basic/define_shorthand.lisp \
    basic/filesystem.lisp \
    basic/floor_modulo.lisp \
    basic/format.lisp \
    basic/hash_tables.lisp \
    basic/integer_ops.lisp \
    basic/integers.lisp \
    basic/keywords.lisp \
    basic/length.lisp \
    basic/list_reverse.lisp \
    basic/make_string.lisp \
    basic/named_let.lisp \
    basic/number_literals.lisp \
    basic/optional_defaults.lisp \
    basic/path_expansion.lisp \
    basic/path_functions.lisp \
    basic/printing.lisp \
    basic/recursion.lisp \
    basic/string-index.lisp \
    basic/string_number_conversion.lisp \
    basic/string_ports.lisp \
    basic/strings.lisp \
    basic/symbols.lisp \
    basic/packages.lisp \
    basic/vectors.lisp \
    basic/utf8_display_width.lisp \
    basic/tui_sleep.lisp \
    basic/tui_terminal.lisp \
    basic/tui_read_byte.lisp \
    basic/tui_events.lisp \
    advanced/collection_iteration.lisp \
    advanced/cond_case.lisp \
    advanced/condition-system.lisp \
    advanced/do_loop.lisp \
    advanced/docstrings.lisp \
    advanced/equality.lisp \
    advanced/file_io.lisp \
    advanced/let_star.lisp \
    advanced/lisp-fmt.lisp \
    advanced/load.lisp \
    advanced/macros.lisp \
    advanced/named_functions.lisp \
    advanced/optional-parameters.lisp \
    advanced/progn.lisp \
    advanced/quasiquote.lisp \
    advanced/regex.lisp \
    advanced/require_provide.lisp \
    advanced/introspection.lisp \
    advanced/tail_recursion.lisp \
    advanced/utf8.lisp \
    advanced/load_path.lisp \
    regression/colon_symbol.lisp \
    regression/cond_case_multi_expr.lisp \
    regression/list_ref_type_check.lisp \
    regression/multiline_parsing.lisp \
    regression/nested-unknown-hash-syntax.lisp \
    regression/tail_call_unwrap.lisp \
    test_sf_kind \
    test_repl_completeness \
    test_auto_indent \
    test_file_utils \
    $(FLARE_TESTS) \
    $(BOBA_TESTS)

TEST_EXTENSIONS = .lisp
LISP_LOG_COMPILER = $(srcdir)/run-test.sh

AM_TESTS_ENVIRONMENT = \
    DITTY_BIN="$(abs_top_builddir)/cli/ditty"; \
    export DITTY_BIN;

EXTRA_DIST = \
    run-test.sh \
    test_sf_kind.c \
    test_repl_completeness.c \
    test_auto_indent.c \
    test_file_utils.c \
    test_repl_app.c \
    flare_testkit.h \
    test_hyperlink.c \
    test-helpers.lisp \
    basic/alists.lisp \
    basic/apply.lisp \
    basic/and_or_list.lisp \
    basic/characters.lisp \
    basic/define_shorthand.lisp \
    basic/filesystem.lisp \
    basic/floor_modulo.lisp \
    basic/format.lisp \
    basic/hash_tables.lisp \
    basic/integer_ops.lisp \
    basic/integers.lisp \
    basic/keywords.lisp \
    basic/length.lisp \
    basic/list_reverse.lisp \
    basic/make_string.lisp \
    basic/named_let.lisp \
    basic/number_literals.lisp \
    basic/optional_defaults.lisp \
    basic/path_expansion.lisp \
    basic/path_functions.lisp \
    basic/printing.lisp \
    basic/recursion.lisp \
    basic/string-index.lisp \
    basic/string_number_conversion.lisp \
    basic/string_ports.lisp \
    basic/strings.lisp \
    basic/symbols.lisp \
    basic/packages.lisp \
    basic/vectors.lisp \
    basic/utf8_display_width.lisp \
    basic/tui_sleep.lisp \
    basic/tui_terminal.lisp \
    basic/tui_read_byte.lisp \
    basic/tui_events.lisp \
    advanced/collection_iteration.lisp \
    advanced/cond_case.lisp \
    advanced/condition-system.lisp \
    advanced/do_loop.lisp \
    advanced/docstrings.lisp \
    advanced/equality.lisp \
    advanced/file_io.lisp \
    advanced/let_star.lisp \
    advanced/lisp-fmt.lisp \
    advanced/load.lisp \
    advanced/macros.lisp \
    advanced/named_functions.lisp \
    advanced/optional-parameters.lisp \
    advanced/progn.lisp \
    advanced/quasiquote.lisp \
    advanced/regex.lisp \
    advanced/require_provide.lisp \
    advanced/introspection.lisp \
    advanced/tail_recursion.lisp \
    advanced/utf8.lisp \
    advanced/load_path.lisp \
    regression/colon_symbol.lisp \
    regression/cond_case_multi_expr.lisp \
    regression/list_ref_type_check.lisp \
    regression/multiline_parsing.lisp \
    regression/nested-unknown-hash-syntax.lisp \
    regression/tail_call_unwrap.lisp \
    fixtures/app_lib/app-helper.lisp \
    fixtures/extra_lib/extra-lib.lisp
