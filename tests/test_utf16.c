/* test_utf16.c - Tests for utf8_encode_utf16_unit (Windows console input) */

#include "../include/utf8.h"

#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT_BYTES(actual, expected, len, msg)                         \
    do {                                                                 \
        tests_run++;                                                     \
        if (memcmp((actual), (expected), (len)) == 0) {                  \
            tests_passed++;                                              \
        } else {                                                         \
            fprintf(stderr, "FAIL: %s: expected", msg);                  \
            for (size_t i_ = 0; i_ < (len); i_++)                        \
                fprintf(stderr, " %02X", (unsigned char)(expected)[i_]); \
            fprintf(stderr, ", got");                                    \
            for (size_t i_ = 0; i_ < (len); i_++)                        \
                fprintf(stderr, " %02X", (unsigned char)(actual)[i_]);   \
            fprintf(stderr, "\n");                                       \
        }                                                                \
    } while (0)

#define ASSERT_INT(actual, expected, msg)                           \
    do {                                                            \
        tests_run++;                                                \
        if ((actual) == (expected)) {                               \
            tests_passed++;                                         \
        } else {                                                    \
            fprintf(stderr, "FAIL: %s: expected %d, got %d\n", msg, \
                    (int)(expected), (int)(actual));                \
        }                                                           \
    } while (0)

#define ASSERT_UINT(actual, expected, msg)                             \
    do {                                                               \
        tests_run++;                                                   \
        if ((actual) == (expected)) {                                  \
            tests_passed++;                                            \
        } else {                                                       \
            fprintf(stderr, "FAIL: %s: expected 0x%04X, got 0x%04X\n", \
                    msg, (unsigned)(expected), (unsigned)(actual));    \
        }                                                              \
    } while (0)

static void test_ascii(void)
{
    uint16_t pending = 0;
    char out[8];
    const char expected[] = { 'A' };

    ASSERT_INT(utf8_encode_utf16_unit(0x0041, &pending, out), 1, "ASCII length");
    ASSERT_BYTES(out, expected, 1, "ASCII bytes");
    ASSERT_UINT(pending, 0, "ASCII leaves no pending surrogate");
}

static void test_bmp(void)
{
    uint16_t pending = 0;
    char out[8];

    {
        const char expected[] = { (char)0xC3, (char)0xA5 };
        ASSERT_INT(utf8_encode_utf16_unit(0x00E5, &pending, out), 2, "U+00E5 length");
        ASSERT_BYTES(out, expected, 2, "U+00E5 bytes");
    }
    {
        const char expected[] = { (char)0xE2, (char)0x82, (char)0xAC };
        ASSERT_INT(utf8_encode_utf16_unit(0x20AC, &pending, out), 3, "U+20AC length");
        ASSERT_BYTES(out, expected, 3, "U+20AC bytes");
    }
    ASSERT_UINT(pending, 0, "BMP leaves no pending surrogate");
}

static void test_surrogate_pair(void)
{
    uint16_t pending = 0;
    char out[8];
    const char expected[] = { (char)0xF0, (char)0x9F, (char)0x98, (char)0x80 };

    ASSERT_INT(utf8_encode_utf16_unit(0xD83D, &pending, out), 0,
               "high surrogate is held");
    ASSERT_UINT(pending, 0xD83D, "high surrogate is pending");
    ASSERT_INT(utf8_encode_utf16_unit(0xDE00, &pending, out), 4,
               "surrogate pair is four bytes");
    ASSERT_BYTES(out, expected, 4, "surrogate pair is U+1F600");
    ASSERT_UINT(pending, 0, "pair clears the pending surrogate");
}

static void test_lone_low_surrogate(void)
{
    uint16_t pending = 0;
    char out[8];
    const char expected[] = { (char)0xEF, (char)0xBF, (char)0xBD };

    ASSERT_INT(utf8_encode_utf16_unit(0xDC00, &pending, out), 3,
               "lone low surrogate is replaced");
    ASSERT_BYTES(out, expected, 3, "lone low surrogate is U+FFFD");
    ASSERT_UINT(pending, 0, "lone low surrogate leaves nothing pending");
}

static void test_lone_high_then_ascii(void)
{
    uint16_t pending = 0;
    char out[8];
    const char expected[] = { (char)0xEF, (char)0xBF, (char)0xBD, 'x' };

    ASSERT_INT(utf8_encode_utf16_unit(0xD800, &pending, out), 0,
               "lone high surrogate is held");
    ASSERT_INT(utf8_encode_utf16_unit(0x0078, &pending, out), 4,
               "replacement plus ASCII is four bytes");
    ASSERT_BYTES(out, expected, 4, "lone high then ASCII");
    ASSERT_UINT(pending, 0, "lone high is cleared by the next unit");
}

static void test_high_then_high(void)
{
    uint16_t pending = 0;
    char out[8];
    const char expected[] = { (char)0xEF, (char)0xBF, (char)0xBD };

    ASSERT_INT(utf8_encode_utf16_unit(0xD800, &pending, out), 0,
               "first high surrogate is held");
    ASSERT_INT(utf8_encode_utf16_unit(0xDBFF, &pending, out), 3,
               "second high surrogate replaces the first");
    ASSERT_BYTES(out, expected, 3, "first of two high surrogates is U+FFFD");
    ASSERT_UINT(pending, 0xDBFF, "second high surrogate is now pending");
}

static void test_high_then_bmp(void)
{
    uint16_t pending = 0;
    char out[8];
    const char expected[] = { (char)0xEF, (char)0xBF, (char)0xBD,
                              (char)0xE2, (char)0x82, (char)0xAC };

    ASSERT_INT(utf8_encode_utf16_unit(0xD800, &pending, out), 0,
               "high surrogate is held");
    ASSERT_INT(utf8_encode_utf16_unit(0x20AC, &pending, out), 6,
               "replacement plus BMP is six bytes");
    ASSERT_BYTES(out, expected, 6, "high then BMP");
    ASSERT_UINT(pending, 0, "high is cleared by the BMP unit");
}

int main(void)
{
    test_ascii();
    test_bmp();
    test_surrogate_pair();
    test_lone_low_surrogate();
    test_lone_high_then_ascii();
    test_high_then_high();
    test_high_then_bmp();

    printf("%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
