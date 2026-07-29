/* test_history.c - Tests for history escape/unescape and file operations */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <boba/components/textinput.h>

/* Internal functions under test - declared static in history.c, exposed for testing */
extern char *history_escape(const char *line);
extern char *history_unescape(const char *escaped);
extern const char *history_get_path(void);

/* Public API under test */
extern void history_load(TuiTextInput *input);
extern void history_save(TuiTextInput *input);

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

static char g_test_history_dir[4096];
static char g_test_history_file[4096];

static int setup_test_history_dir(void)
{
    const char *tmpdir = getenv("TMPDIR");
    if (!tmpdir)
        tmpdir = "/tmp";
    snprintf(g_test_history_dir, sizeof(g_test_history_dir), "%s/ditty_test_XXXXXX", tmpdir);
    if (mkdtemp(g_test_history_dir) == NULL)
        return -1;
    snprintf(g_test_history_file, sizeof(g_test_history_file), "%s/history", g_test_history_dir);
    return 0;
}

static void cleanup_test_history_dir(void)
{
    if (g_test_history_file[0])
        unlink(g_test_history_file);
    if (g_test_history_dir[0])
        rmdir(g_test_history_dir);
}

static void test_save_creates_file(void)
{
    if (setup_test_history_dir() != 0) {
        fprintf(stderr, "FAIL: test_save_creates_file: could not create temp dir\n");
        tests_run++;
        return;
    }

    TuiTextInput *input = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input, "save creates file: input non-NULL");
    tui_textinput_set_history_size(input, 100);

    tui_textinput_history_add(input, "line1");
    tui_textinput_history_add(input, "line2");

    /* Override path for testing */
    FILE *f = fopen(g_test_history_file, "w");
    ASSERT_NOT_NULL(f, "save creates file: can open file for write");
    fprintf(f, "line1\nline2\n");
    fclose(f);

    /* Verify file was written */
    f = fopen(g_test_history_file, "r");
    ASSERT_NOT_NULL(f, "save creates file: can open file for read");

    char buf[256];
    int line_count = 0;
    while (fgets(buf, sizeof(buf), f))
        line_count++;
    fclose(f);

    tests_run++;
    if (line_count == 2) {
        tests_passed++;
    } else {
        fprintf(stderr, "FAIL: save creates file: expected 2 lines, got %d\n", line_count);
    }

    tui_textinput_free(input);
    cleanup_test_history_dir();
}

static void test_load_reads_file(void)
{
    if (setup_test_history_dir() != 0) {
        fprintf(stderr, "FAIL: test_load_reads_file: could not create temp dir\n");
        tests_run++;
        return;
    }

    /* Write test file */
    FILE *f = fopen(g_test_history_file, "w");
    ASSERT_NOT_NULL(f, "load reads file: can open file for write");
    fprintf(f, "line1\nline2\nline with\\\\nbackslash\n");
    fclose(f);

    TuiTextInput *input = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input, "load reads file: input non-NULL");
    tui_textinput_set_history_size(input, 100);

    /* Load from test file */
    f = fopen(g_test_history_file, "r");
    ASSERT_NOT_NULL(f, "load reads file: can reopen file");

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';
        char *unescaped = history_unescape(line);
        if (unescaped) {
            tui_textinput_history_add(input, unescaped);
            free(unescaped);
        }
    }
    fclose(f);

    tests_run++;
    if (input->history_count == 3) {
        tests_passed++;
    } else {
        fprintf(stderr, "FAIL: load reads file: expected 3 entries, got %d\n", input->history_count);
    }

    tui_textinput_free(input);
    cleanup_test_history_dir();
}

static void test_roundtrip_save_load(void)
{
    if (setup_test_history_dir() != 0) {
        fprintf(stderr, "FAIL: test_roundtrip_save_load: could not create temp dir\n");
        tests_run++;
        return;
    }

    /* Create input with multiline entry */
    TuiTextInput *input1 = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input1, "roundtrip: input1 non-NULL");
    tui_textinput_set_history_size(input1, 100);

    tui_textinput_history_add(input1, "simple");
    tui_textinput_history_add(input1, "multi\nline\nentry");
    tui_textinput_history_add(input1, "/doc map");

    /* Manually save to test file */
    FILE *f = fopen(g_test_history_file, "w");
    ASSERT_NOT_NULL(f, "roundtrip: can open file for write");
    for (int i = 0; i < input1->history_count; i++) {
        char *escaped = history_escape(input1->history[i]);
        if (escaped) {
            fprintf(f, "%s\n", escaped);
            free(escaped);
        }
    }
    fclose(f);

    /* Load into new input */
    TuiTextInput *input2 = tui_textinput_create(NULL);
    ASSERT_NOT_NULL(input2, "roundtrip: input2 non-NULL");
    tui_textinput_set_history_size(input2, 100);

    f = fopen(g_test_history_file, "r");
    ASSERT_NOT_NULL(f, "roundtrip: can open file for read");

    char line[1024];
    while (fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n')
            line[len - 1] = '\0';
        char *unescaped = history_unescape(line);
        if (unescaped) {
            tui_textinput_history_add(input2, unescaped);
            free(unescaped);
        }
    }
    fclose(f);

    /* Verify */
    tests_run++;
    if (input2->history_count == 3) {
        tests_passed++;
    } else {
        fprintf(stderr, "FAIL: roundtrip: expected 3 entries, got %d\n", input2->history_count);
    }

    tests_run++;
    if (input2->history_count >= 2 && strcmp(input2->history[1], "multi\nline\nentry") == 0) {
        tests_passed++;
    } else if (input2->history_count >= 2) {
        fprintf(stderr, "FAIL: roundtrip: multiline entry mismatch, got '%s'\n", input2->history[1]);
    }

    tui_textinput_free(input1);
    tui_textinput_free(input2);
    cleanup_test_history_dir();
}

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

    /* Load/Save tests */
    test_save_creates_file();
    test_load_reads_file();
    test_roundtrip_save_load();

    printf("%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
