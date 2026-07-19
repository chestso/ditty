/* test_file_utils.c - Tests for file_dirname and file_resolve_library_in_dirs */

#include "../include/file_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <unistd.h>
#define MKDIR(path) mkdir(path, 0755)
#endif

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

#define ASSERT_TRUE(cond, msg)                                   \
    do {                                                         \
        tests_run++;                                             \
        if (cond) {                                              \
            tests_passed++;                                      \
        } else {                                                 \
            fprintf(stderr, "FAIL: %s: condition false\n", msg); \
        }                                                        \
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

#define ASSERT_NOT_NULL(p, msg)                                    \
    do {                                                           \
        tests_run++;                                               \
        if ((p) != NULL) {                                         \
            tests_passed++;                                        \
        } else {                                                   \
            fprintf(stderr, "FAIL: %s: expected non-NULL\n", msg); \
        }                                                          \
    } while (0)

static void test_dirname_cases(void)
{
    char buf[4096];

    /* Absolute Unix path */
    ASSERT_STR_EQ(file_dirname("/a/b/c.lisp", buf, sizeof(buf)), "/a/b",
                  "dirname /a/b/c.lisp");

    /* Trailing separator is stripped before taking dirname */
    ASSERT_STR_EQ(file_dirname("/a/b/", buf, sizeof(buf)), "/a",
                  "dirname /a/b/");

    /* Bare filename -> "." */
    ASSERT_STR_EQ(file_dirname("c.lisp", buf, sizeof(buf)), ".",
                  "dirname c.lisp");

    /* Root path stays root */
    ASSERT_STR_EQ(file_dirname("/foo", buf, sizeof(buf)), "/",
                  "dirname /foo");

    /* Root only */
    ASSERT_STR_EQ(file_dirname("/", buf, sizeof(buf)), "/",
                  "dirname /");

    /* Current dir */
    ASSERT_STR_EQ(file_dirname(".", buf, sizeof(buf)), ".",
                  "dirname .");

    /* Relative multi-component */
    ASSERT_STR_EQ(file_dirname("a/b/c.lisp", buf, sizeof(buf)), "a/b",
                  "dirname a/b/c.lisp");

    /* NULL/empty input -> "." (degenerate) */
    ASSERT_STR_EQ(file_dirname(NULL, buf, sizeof(buf)), ".",
                  "dirname NULL");
    ASSERT_STR_EQ(file_dirname("", buf, sizeof(buf)), ".",
                  "dirname empty");

#ifdef _WIN32
    /* Windows drive paths */
    ASSERT_STR_EQ(file_dirname("C:\\x\\y.lisp", buf, sizeof(buf)), "C:\\x",
                  "dirname C:\\x\\y.lisp");
    ASSERT_STR_EQ(file_dirname("C:\\foo", buf, sizeof(buf)), "C:\\",
                  "dirname C:\\foo");
    /* Mixed separators tolerated */
    ASSERT_STR_EQ(file_dirname("C:/x/y.lisp", buf, sizeof(buf)), "C:/x",
                  "dirname C:/x/y.lisp");
#endif
}

/* Build a fixture tree under the system temp dir:
 *   <tmp>/ditty_fixtures/sibling/tapestry.lisp            (single-file lib)
 *   <tmp>/ditty_fixtures/sibling/mod/mod.lisp             (dir-based lib)
 * Then test file_resolve_library_in_dirs against it. */
static char g_fixture_dir_holder[4096];
static const char *g_fixture_dir = NULL;

static int write_file(const char *path, const char *contents)
{
    FILE *f = fopen(path, "wb");
    if (!f)
        return -1;
    fputs(contents, f);
    fclose(f);
    return 0;
}

static int build_fixtures(void)
{
    static char base[4096];
    char tmp[4096];
    char sub[4096];

    if (file_temp_directory(base, sizeof(base)) != 0)
        return -1;
    snprintf(tmp, sizeof(tmp), "%s/ditty_fixtures", base);
    if (MKDIR(tmp) != 0 && !file_exists(tmp))
        return -1;
    snprintf(g_fixture_dir_holder, sizeof(g_fixture_dir_holder), "%s", tmp);
    /* Use a static buffer so g_fixture_dir stays valid for the test lifetime. */
    g_fixture_dir = g_fixture_dir_holder;

    snprintf(sub, sizeof(sub), "%s/sibling", g_fixture_dir);
    if (MKDIR(sub) != 0 && !file_exists(sub))
        return -1;

    snprintf(tmp, sizeof(tmp), "%s/sibling/tapestry.lisp", g_fixture_dir);
    if (write_file(tmp, "(provide 'tapestry)\n") != 0)
        return -1;

    snprintf(sub, sizeof(sub), "%s/sibling/mod", g_fixture_dir);
    if (MKDIR(sub) != 0 && !file_exists(sub))
        return -1;
    snprintf(tmp, sizeof(tmp), "%s/sibling/mod/mod.lisp", g_fixture_dir);
    if (write_file(tmp, "(provide 'mod)\n") != 0)
        return -1;

    return 0;
}

static void test_resolve_library_in_dirs(void)
{
    char resolved[4096];

    if (!g_fixture_dir) {
        fprintf(stderr, "FAIL: fixtures not built\n");
        tests_run++;
        return;
    }

    char dir[4096];
    snprintf(dir, sizeof(dir), "%s/sibling", g_fixture_dir);

    const char *dirs[1] = { dir };

    /* Single-file library: <dir>/tapestry.lisp */
    const char *r = file_resolve_library_in_dirs("tapestry", dirs, 1,
                                                 resolved, sizeof(resolved));
    ASSERT_NOT_NULL(r, "resolve tapestry in sibling dir");
    ASSERT_TRUE(strstr(r, "tapestry.lisp") != NULL,
                "resolved path ends with tapestry.lisp");

    /* Directory-based library: <dir>/mod/mod.lisp */
    r = file_resolve_library_in_dirs("mod", dirs, 1,
                                     resolved, sizeof(resolved));
    ASSERT_NOT_NULL(r, "resolve mod (directory-based)");
    ASSERT_TRUE(strstr(r, "mod.lisp") != NULL,
                "resolved path ends with mod.lisp");

    /* Not present */
    r = file_resolve_library_in_dirs("nonexistent-xyz", dirs, 1,
                                     resolved, sizeof(resolved));
    ASSERT_NULL(r, "resolve nonexistent returns NULL");

    /* Empty / NULL dirs array */
    r = file_resolve_library_in_dirs("tapestry", NULL, 0,
                                     resolved, sizeof(resolved));
    ASSERT_NULL(r, "NULL dirs returns NULL");
    r = file_resolve_library_in_dirs("tapestry", dirs, 0,
                                     resolved, sizeof(resolved));
    ASSERT_NULL(r, "zero n_dirs returns NULL");

    /* NULL/empty name */
    r = file_resolve_library_in_dirs(NULL, dirs, 1, resolved, sizeof(resolved));
    ASSERT_NULL(r, "NULL name returns NULL");
    r = file_resolve_library_in_dirs("", dirs, 1, resolved, sizeof(resolved));
    ASSERT_NULL(r, "empty name returns NULL");

    /* Empty string entries are skipped (not crashed on) */
    const char *mixed[3] = { "", dir, "" };
    r = file_resolve_library_in_dirs("tapestry", mixed, 3,
                                     resolved, sizeof(resolved));
    ASSERT_NOT_NULL(r, "skips empty entries, finds in middle");

    /* First match wins: put a dir with a different tapestry.lisp first. */
    char other[4096];
    snprintf(other, sizeof(other), "%s/other", g_fixture_dir);
    if (MKDIR(other) == 0 || file_exists(other)) {
        char ofile[4096];
        snprintf(ofile, sizeof(ofile), "%s/other/tapestry.lisp", g_fixture_dir);
        write_file(ofile, "(provide 'tapestry)\n");
        const char *ordered[2] = { other, dir };
        r = file_resolve_library_in_dirs("tapestry", ordered, 2,
                                         resolved, sizeof(resolved));
        ASSERT_NOT_NULL(r, "first-match-wins: resolves");
        ASSERT_TRUE(strstr(r, "other") != NULL,
                    "first-match-wins: returned the first dir's copy");
    }
}

int main(void)
{
    test_dirname_cases();
    if (build_fixtures() != 0) {
        fprintf(stderr, "FAIL: could not build fixtures\n");
        printf("%d/%d tests passed\n", tests_passed, tests_run);
        return 1;
    }
    test_resolve_library_in_dirs();

    printf("%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
