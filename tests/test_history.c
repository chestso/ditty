/* test_history.c - Tests for history escape/unescape and file operations */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <boba/cmd.h>
#include <boba/components/textinput.h>
#include <boba/msg.h>

#include "../cli/history.h"

/* Internal helpers under test - exposed for testing */
extern char *history_escape(const char *line);
extern char *history_unescape(const char *escaped);
extern const char *history_get_path(void);

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_STR_EQ(actual, expected, msg)                           \
    do {                                                               \
        tests_run++;                                                   \
        if (strcmp((actual), (expected)) == 0) {                       \
            tests_passed++;                                            \
        } else {                                                       \
            fprintf(stderr, "FAIL: %s: expected \"%s\", got \"%s\"\n", \
                    msg, (expected), (actual));                        \
        }                                                              \
    } while (0)

#define ASSERT_NOT_NULL(p, msg)                                    \
    do {                                                           \
        tests_run++;                                               \
        if ((p) != NULL) {                                         \
            tests_passed++;                                        \
        } else {                                                   \
            fprintf(stderr, "FAIL: %s: expected non-NULL\n", msg); \
        }                                                          \
    } while (0)

#define ASSERT_NULL(p, msg)                                                         \
    do {                                                                            \
        tests_run++;                                                                \
        if ((p) == NULL) {                                                          \
            tests_passed++;                                                         \
        } else {                                                                    \
            fprintf(stderr, "FAIL: %s: expected NULL, got %p\n", msg, (void *)(p)); \
        }                                                                           \
    } while (0)

#define ASSERT_TRUE(cond, msg)                                   \
    do {                                                         \
        tests_run++;                                             \
        if (cond) {                                              \
            tests_passed++;                                      \
        } else {                                                 \
            fprintf(stderr, "FAIL: %s: condition false\n", msg); \
        }                                                        \
    } while (0)

/* ---- Escape/Unescape Tests ---- */

static void test_escape_simple(void)
{
    char *result = history_escape("hello");
    ASSERT_NOT_NULL(result, "escape simple: non-NULL");
    ASSERT_STR_EQ(result, "hello", "escape simple: no change");
    free(result);
}

static void test_escape_newline(void)
{
    char *result = history_escape("line1\nline2");
    ASSERT_NOT_NULL(result, "escape newline: non-NULL");
    ASSERT_STR_EQ(result, "line1\\nline2", "escape newline: \\n escaped");
    free(result);
}

static void test_escape_backslash(void)
{
    char *result = history_escape("path\\to\\file");
    ASSERT_NOT_NULL(result, "escape backslash: non-NULL");
    ASSERT_STR_EQ(result, "path\\\\to\\\\file", "escape backslash: \\\\ escaped");
    free(result);
}

static void test_escape_mixed(void)
{
    char *result = history_escape("line1\nline2\\nmore");
    ASSERT_NOT_NULL(result, "escape mixed: non-NULL");
    ASSERT_STR_EQ(result, "line1\\nline2\\\\nmore", "escape mixed: both escaped");
    free(result);
}

static void test_escape_multiple_newlines(void)
{
    char *result = history_escape("a\nb\nc");
    ASSERT_NOT_NULL(result, "escape multiple newlines: non-NULL");
    ASSERT_STR_EQ(result, "a\\nb\\nc", "escape multiple newlines: all escaped");
    free(result);
}

static void test_escape_empty(void)
{
    char *result = history_escape("");
    ASSERT_NOT_NULL(result, "escape empty: non-NULL");
    ASSERT_STR_EQ(result, "", "escape empty: empty string");
    free(result);
}

static void test_escape_null(void)
{
    char *result = history_escape(NULL);
    ASSERT_NULL(result, "escape NULL: returns NULL");
}

/* ---- Unescape Tests ---- */

static void test_unescape_simple(void)
{
    char *result = history_unescape("hello");
    ASSERT_NOT_NULL(result, "unescape simple: non-NULL");
    ASSERT_STR_EQ(result, "hello", "unescape simple: no change");
    free(result);
}

static void test_unescape_newline(void)
{
    char *result = history_unescape("line1\\nline2");
    ASSERT_NOT_NULL(result, "unescape newline: non-NULL");
    ASSERT_STR_EQ(result, "line1\nline2", "unescape newline: \\n unescaped");
    free(result);
}

static void test_unescape_backslash(void)
{
    char *result = history_unescape("path\\\\to\\\\file");
    ASSERT_NOT_NULL(result, "unescape backslash: non-NULL");
    ASSERT_STR_EQ(result, "path\\to\\file", "unescape backslash: \\\\ unescaped");
    free(result);
}

static void test_unescape_mixed(void)
{
    char *result = history_unescape("line1\\nline2\\\\nmore");
    ASSERT_NOT_NULL(result, "unescape mixed: non-NULL");
    ASSERT_STR_EQ(result, "line1\nline2\\nmore", "unescape mixed: both unescaped");
    free(result);
}

static void test_unescape_literal_n(void)
{
    char *result = history_unescape("literal n");
    ASSERT_NOT_NULL(result, "unescape literal n: non-NULL");
    ASSERT_STR_EQ(result, "literal n", "unescape literal n: unchanged");
    free(result);
}

static void test_unescape_trailing_backslash(void)
{
    char *result = history_unescape("trailing\\");
    ASSERT_NOT_NULL(result, "unescape trailing backslash: non-NULL");
    ASSERT_STR_EQ(result, "trailing\\", "unescape trailing backslash: preserved");
    free(result);
}

static void test_unescape_empty(void)
{
    char *result = history_unescape("");
    ASSERT_NOT_NULL(result, "unescape empty: non-NULL");
    ASSERT_STR_EQ(result, "", "unescape empty: empty string");
    free(result);
}

static void test_unescape_null(void)
{
    char *result = history_unescape(NULL);
    ASSERT_NULL(result, "unescape NULL: returns NULL");
}

/* ---- Round-trip Tests ---- */

static void test_roundtrip_simple(void)
{
    const char *original = "hello world";
    char *escaped = history_escape(original);
    ASSERT_NOT_NULL(escaped, "roundtrip simple: escape non-NULL");
    char *unescaped = history_unescape(escaped);
    ASSERT_NOT_NULL(unescaped, "roundtrip simple: unescape non-NULL");
    ASSERT_STR_EQ(unescaped, original, "roundtrip simple: round-trip matches");
    free(escaped);
    free(unescaped);
}

static void test_roundtrip_newline(void)
{
    const char *original = "line1\nline2\nline3";
    char *escaped = history_escape(original);
    ASSERT_NOT_NULL(escaped, "roundtrip newline: escape non-NULL");
    char *unescaped = history_unescape(escaped);
    ASSERT_NOT_NULL(unescaped, "roundtrip newline: unescape non-NULL");
    ASSERT_STR_EQ(unescaped, original, "roundtrip newline: round-trip matches");
    free(escaped);
    free(unescaped);
}

static void test_roundtrip_mixed(void)
{
    const char *original = "define (f x)\n  (+ x \\n)";
    char *escaped = history_escape(original);
    ASSERT_NOT_NULL(escaped, "roundtrip mixed: escape non-NULL");
    char *unescaped = history_unescape(escaped);
    ASSERT_NOT_NULL(unescaped, "roundtrip mixed: unescape non-NULL");
    ASSERT_STR_EQ(unescaped, original, "roundtrip mixed: round-trip matches");
    free(escaped);
    free(unescaped);
}

/* ---- Path Tests ---- */

static void test_get_path_non_null(void)
{
    const char *path = history_get_path();
    ASSERT_NOT_NULL(path, "get_path: returns non-NULL");
}

static void test_get_path_contains_ditty(void)
{
    const char *path = history_get_path();
    ASSERT_NOT_NULL(path, "get_path contains: path non-NULL");
    tests_run++;
    if (strstr(path, "ditty") != NULL) {
        tests_passed++;
    } else {
        fprintf(stderr, "FAIL: get_path contains ditty: path should contain 'ditty', got '%s'\n", path);
    }
}

static void test_get_path_is_absolute(void)
{
    const char *path = history_get_path();
    ASSERT_NOT_NULL(path, "get_path absolute: path non-NULL");
    tests_run++;
#ifdef _WIN32
    /* Windows: check for drive letter like C: */
    int is_absolute = (path[0] != '\0' && path[1] == ':');
#else
    /* POSIX: check for leading / */
    int is_absolute = (path[0] == '/');
#endif
    if (is_absolute) {
        tests_passed++;
    } else {
        fprintf(stderr, "FAIL: get_path absolute: path should be absolute, got '%s'\n", path);
    }
}

/* ---- Load/Save Tests ---- */

/* history_get_path() derives the file from $XDG_STATE_HOME on POSIX, so
 * pointing that variable at a temp directory is all it takes to keep these
 * tests away from the real history file. Windows has no such override (the
 * path comes from the shell's known folders), so the file tests are
 * POSIX-only; everything above runs on both. */
#ifndef _WIN32

static char g_test_state_dir[4096];
static char g_test_history_file[4096];
static char g_test_ditty_dir[4096];
static char g_saved_state_home[4096];
static int g_had_state_home = 0;

static int setup_test_history_dir(void)
{
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir)
        tmpdir = "/tmp";
    snprintf(g_test_state_dir, sizeof(g_test_state_dir), "%s/ditty_test_XXXXXX",
             tmpdir);
    if (mkdtemp(g_test_state_dir) == NULL)
        return -1;

    snprintf(g_test_ditty_dir, sizeof(g_test_ditty_dir), "%s/ditty",
             g_test_state_dir);
    snprintf(g_test_history_file, sizeof(g_test_history_file), "%s/history",
             g_test_ditty_dir);

    const char *old = getenv("XDG_STATE_HOME");
    g_had_state_home = old != NULL;
    if (old)
        snprintf(g_saved_state_home, sizeof(g_saved_state_home), "%s", old);
    setenv("XDG_STATE_HOME", g_test_state_dir, 1);

    return 0;
}

static void cleanup_test_history_dir(void)
{
    unlink(g_test_history_file);
    rmdir(g_test_ditty_dir);
    rmdir(g_test_state_dir);

    if (g_had_state_home)
        setenv("XDG_STATE_HOME", g_saved_state_home, 1);
    else
        unsetenv("XDG_STATE_HOME");
}

/* Read a whole file into a malloc'd, NUL-terminated buffer; NULL if absent */
static char *slurp(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return NULL;

    size_t cap = 256, len = 0;
    char *buf = malloc(cap);
    if (!buf) {
        fclose(f);
        return NULL;
    }

    for (;;) {
        if (len + 1 >= cap) {
            char *bigger = realloc(buf, cap * 2);
            if (!bigger) {
                free(buf);
                fclose(f);
                return NULL;
            }
            buf = bigger;
            cap *= 2;
        }
        size_t n = fread(buf + len, 1, cap - len - 1, f);
        if (n == 0)
            break;
        len += n;
    }
    fclose(f);
    buf[len] = '\0';
    return buf;
}

/* Write a file with the exact bytes given (creating the ditty/ dir) */
static int write_history_file(const char *bytes)
{
    if (mkdir(g_test_ditty_dir, 0700) != 0 && errno != EEXIST)
        return -1;
    FILE *f = fopen(g_test_history_file, "wb");
    if (!f)
        return -1;
    size_t len = strlen(bytes);
    int ok = fwrite(bytes, 1, len, f) == len;
    fclose(f);
    return ok ? 0 : -1;
}

#define SETUP_OR_SKIP(name)                                   \
    do {                                                      \
        if (setup_test_history_dir() != 0) {                  \
            fprintf(stderr, "FAIL: %s: no temp dir\n", name); \
            tests_run++;                                      \
            return;                                           \
        }                                                     \
    } while (0)

/* Saved history is chronological: oldest at the top, newest at the bottom */
static void test_save_is_oldest_first(void)
{
    SETUP_OR_SKIP("test_save_is_oldest_first");

    TuiTextInput *input = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input, "save oldest first: input non-NULL");
    tui_textinput_set_history_size(input, HISTORY_MAX_ENTRIES);

    tui_textinput_history_add(input, "line1");
    tui_textinput_history_add(input, "line2");
    history_save(input); /* also creates the state directory tree */

    char *content = slurp(g_test_history_file);
    ASSERT_NOT_NULL(content, "save oldest first: file written");
    ASSERT_STR_EQ(content, "line1\nline2\n",
                  "save oldest first: oldest line first on disk");
    free(content);

    tui_textinput_free(input);
    cleanup_test_history_dir();
}

/* Empty ring → nothing written (not even an empty file) */
static void test_save_without_entries_writes_nothing(void)
{
    SETUP_OR_SKIP("test_save_without_entries_writes_nothing");

    TuiTextInput *input = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input, "save no entries: input non-NULL");
    tui_textinput_set_history_size(input, HISTORY_MAX_ENTRIES);

    history_save(input);

    char *content = slurp(g_test_history_file);
    ASSERT_NULL(content, "save no entries: no file created");

    tui_textinput_free(input);
    cleanup_test_history_dir();
}

/* The ring's dedup rule reaches the file: a repeat moves to the newest row */
static void test_save_squashes_repeats(void)
{
    SETUP_OR_SKIP("test_save_squashes_repeats");

    TuiTextInput *input = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input, "squash repeats: input non-NULL");
    tui_textinput_set_history_size(input, HISTORY_MAX_ENTRIES);

    tui_textinput_history_add(input, "(+ 1 2)");
    tui_textinput_history_add(input, "(+ 3 4)");
    tui_textinput_history_add(input, "(+ 1 2)");
    history_save(input);

    char *content = slurp(g_test_history_file);
    ASSERT_NOT_NULL(content, "squash repeats: file written");
    ASSERT_STR_EQ(content, "(+ 3 4)\n(+ 1 2)\n",
                  "squash repeats: repeat moved to the end, once");
    free(content);

    tui_textinput_free(input);
    cleanup_test_history_dir();
}

/* Empty lines are not history */
static void test_save_skips_empty_lines(void)
{
    SETUP_OR_SKIP("test_save_skips_empty_lines");

    TuiTextInput *input = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input, "skip empty: input non-NULL");
    tui_textinput_set_history_size(input, HISTORY_MAX_ENTRIES);

    tui_textinput_history_add(input, "");
    tui_textinput_history_add(input, "real");
    tui_textinput_history_add(input, "");
    history_save(input);

    char *content = slurp(g_test_history_file);
    ASSERT_NOT_NULL(content, "skip empty: file written");
    ASSERT_STR_EQ(content, "real\n", "skip empty: only the real line stored");
    free(content);

    tui_textinput_free(input);
    cleanup_test_history_dir();
}

/* The ring the file is written from is the one Up/Down walk: load fills it,
 * and Up reaches the entry the file ends with */
static void test_load_feeds_the_ring(void)
{
    SETUP_OR_SKIP("test_load_feeds_the_ring");

    if (write_history_file("oldest\nnewest\n") != 0) {
        fprintf(stderr, "FAIL: load feeds ring: could not write fixture\n");
        tests_run++;
        cleanup_test_history_dir();
        return;
    }

    TuiTextInput *input = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input, "load feeds ring: input non-NULL");
    tui_textinput_set_history_size(input, HISTORY_MAX_ENTRIES);

    history_load(input);
    ASSERT_TRUE(tui_textinput_history_count(input) == 2,
                "load feeds ring: both lines loaded");

    TuiUpdateResult r = tui_textinput_update(input, tui_msg_key(TUI_KEY_UP, 0, 0));
    if (r.cmd)
        tui_cmd_free(r.cmd);
    ASSERT_STR_EQ(tui_textinput_text(input), "newest",
                  "load feeds ring: Up reaches the newest line first");

    tui_textinput_free(input);
    cleanup_test_history_dir();
}

/* load(save(x)) == x — a session rewrites exactly what it read */
static void test_load_then_save_is_idempotent(void)
{
    SETUP_OR_SKIP("test_load_then_save_is_idempotent");

    /* /doc map, then a two-line form stored with an escaped newline */
    const char *escaped = "/doc map\n(define (f x)\\n  x)\n";
    if (write_history_file(escaped) != 0) {
        fprintf(stderr, "FAIL: load idempotent: could not write fixture\n");
        tests_run++;
        cleanup_test_history_dir();
        return;
    }

    TuiTextInput *input = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input, "load idempotent: input non-NULL");
    tui_textinput_set_history_size(input, HISTORY_MAX_ENTRIES);

    history_load(input);
    history_save(input);

    char *content = slurp(g_test_history_file);
    ASSERT_NOT_NULL(content, "load idempotent: file still there");
    ASSERT_STR_EQ(content, escaped,
                  "load idempotent: order and escaping survive a load/save");
    free(content);

    tui_textinput_free(input);
    cleanup_test_history_dir();
}

/* Nothing older than the ring's cap reaches the file, and the newest do */
static void test_save_honours_the_cap(void)
{
    SETUP_OR_SKIP("test_save_honours_the_cap");

    TuiTextInput *input = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input, "honours cap: input non-NULL");
    tui_textinput_set_history_size(input, 2); /* far smaller than the file cap */

    tui_textinput_history_add(input, "dropped");
    tui_textinput_history_add(input, "kept-older");
    tui_textinput_history_add(input, "kept-newer");
    history_save(input);

    char *content = slurp(g_test_history_file);
    ASSERT_NOT_NULL(content, "honours cap: file written");
    ASSERT_STR_EQ(content, "kept-older\nkept-newer\n",
                  "honours cap: only what the ring still holds is written");
    free(content);

    tui_textinput_free(input);
    cleanup_test_history_dir();
}

/* Multiline entries survive a save → fresh load → save round trip */
static void test_roundtrip_save_load(void)
{
    SETUP_OR_SKIP("test_roundtrip_save_load");

    TuiTextInput *input1 = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input1, "roundtrip: input1 non-NULL");
    tui_textinput_set_history_size(input1, HISTORY_MAX_ENTRIES);

    tui_textinput_history_add(input1, "simple");
    tui_textinput_history_add(input1, "multi\nline\nentry");
    tui_textinput_history_add(input1, "/doc map");
    history_save(input1);

    char *first = slurp(g_test_history_file);
    ASSERT_NOT_NULL(first, "roundtrip: file written");

    /* A fresh session: a new textinput, its ring filled from the file */
    TuiTextInput *input2 = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input2, "roundtrip: input2 non-NULL");
    tui_textinput_set_history_size(input2, HISTORY_MAX_ENTRIES);

    history_load(input2);
    history_save(input2);

    char *second = slurp(g_test_history_file);
    ASSERT_NOT_NULL(second, "roundtrip: file rewritten");
    ASSERT_STR_EQ(second, first,
                  "roundtrip: second session writes the same bytes");
    ASSERT_STR_EQ(first, "simple\nmulti\\nline\\nentry\n/doc map\n",
                  "roundtrip: chronological order, newlines escaped");

    free(first);
    free(second);
    tui_textinput_free(input1);
    tui_textinput_free(input2);
    cleanup_test_history_dir();
}
#endif /* !_WIN32 */

/* ---- Main ---- */

int main(void)
{
    /* Escape tests */
    test_escape_simple();
    test_escape_newline();
    test_escape_backslash();
    test_escape_mixed();
    test_escape_multiple_newlines();
    test_escape_empty();
    test_escape_null();

    /* Unescape tests */
    test_unescape_simple();
    test_unescape_newline();
    test_unescape_backslash();
    test_unescape_mixed();
    test_unescape_literal_n();
    test_unescape_trailing_backslash();
    test_unescape_empty();
    test_unescape_null();

    /* Round-trip tests */
    test_roundtrip_simple();
    test_roundtrip_newline();
    test_roundtrip_mixed();

    /* Path tests */
    test_get_path_non_null();
    test_get_path_contains_ditty();
    test_get_path_is_absolute();

#ifndef _WIN32
    /* Load/Save tests (POSIX: $XDG_STATE_HOME points at the temp dir) */
    test_save_is_oldest_first();
    test_save_without_entries_writes_nothing();
    test_save_squashes_repeats();
    test_save_skips_empty_lines();
    test_save_honours_the_cap();
    test_load_feeds_the_ring();
    test_load_then_save_is_idempotent();
    test_roundtrip_save_load();
#endif

    printf("%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
