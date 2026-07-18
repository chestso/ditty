#include "../include/utf8.h"
#include <string.h>
#include <stdlib.h>

/* Check if codepoint is a grapheme extender (variation selector, combining mark, ZWJ) */
static int is_grapheme_extender(int cp)
{
    /* Variation selectors */
    if ((cp >= 0xFE00 && cp <= 0xFE0F) || /* Variation Selectors */
        (cp >= 0xE0100 && cp <= 0xE01EF)) /* Variation Selectors Supplement */
        return 1;

    /* Zero-width joiner (used in emoji sequences) */
    if (cp == 0x200D)
        return 1;

    /* Combining diacritical marks */
    if ((cp >= 0x0300 && cp <= 0x036F) || /* Combining Diacritical Marks */
        (cp >= 0x1AB0 && cp <= 0x1AFF) || /* Combining Diacritical Marks Extended */
        (cp >= 0x1DC0 && cp <= 0x1DFF) || /* Combining Diacritical Marks Supplement */
        (cp >= 0x20D0 && cp <= 0x20FF) || /* Combining Diacritical Marks for Symbols */
        (cp >= 0xFE20 && cp <= 0xFE2F))   /* Combining Half Marks */
        return 1;

    return 0;
}

/* Count UTF-8 code points in string (not bytes, not graphemes).
 * This matches the indexing used by string-ref and utf8_char_at. */
size_t utf8_strlen(const char *str)
{
    if (str == NULL)
        return 0;

    size_t count = 0;
    const char *ptr = str;
    while (*ptr) {
        int bytes = utf8_char_bytes(ptr);
        count++;
        ptr += bytes > 0 ? bytes : 1;
    }
    return count;
}

/* Get character at character index (not byte index) */
const char *utf8_char_at(const char *str, size_t char_index)
{
    if (str == NULL)
        return NULL;

    size_t count = 0;
    const char *ptr = str;
    while (*ptr && count < char_index) {
        /* Advance to next character */
        if ((*ptr & 0x80) == 0) {
            ptr++;
        } else if ((*ptr & 0xE0) == 0xC0) {
            ptr += 2;
        } else if ((*ptr & 0xF0) == 0xE0) {
            ptr += 3;
        } else if ((*ptr & 0xF8) == 0xF0) {
            ptr += 4;
        } else {
            ptr++;
        }
        count++;
    }

    return (count == char_index) ? ptr : NULL;
}

/* Advance pointer to next UTF-8 character */
const char *utf8_next_char(const char *ptr)
{
    if (ptr == NULL || *ptr == '\0')
        return NULL;

    if ((*ptr & 0x80) == 0) {
        return ptr + 1; /* ASCII */
    } else if ((*ptr & 0xE0) == 0xC0) {
        return ptr + 2; /* 2-byte */
    } else if ((*ptr & 0xF0) == 0xE0) {
        return ptr + 3; /* 3-byte */
    } else if ((*ptr & 0xF8) == 0xF0) {
        return ptr + 4; /* 4-byte */
    }
    return ptr + 1; /* Invalid, advance by 1 */
}

/* Move pointer to previous UTF-8 character */
const char *utf8_prev_char(const char *str, const char *ptr)
{
    if (str == NULL || ptr == NULL || ptr <= str)
        return str;

    /* Move back one byte */
    ptr--;

    /* Skip UTF-8 continuation bytes (10xxxxxx = 0x80-0xBF) */
    while (ptr > str && (*ptr & 0xC0) == 0x80)
        ptr--;

    return ptr;
}

/* Validate UTF-8 sequence */
int utf8_validate(const char *str)
{
    if (str == NULL)
        return 1;

    const char *ptr = str;
    while (*ptr) {
        if ((*ptr & 0x80) == 0) {
            ptr++;
        } else if ((*ptr & 0xE0) == 0xC0) {
            if ((*(ptr + 1) & 0xC0) != 0x80)
                return 0;
            ptr += 2;
        } else if ((*ptr & 0xF0) == 0xE0) {
            if ((*(ptr + 1) & 0xC0) != 0x80 || (*(ptr + 2) & 0xC0) != 0x80)
                return 0;
            ptr += 3;
        } else if ((*ptr & 0xF8) == 0xF0) {
            if ((*(ptr + 1) & 0xC0) != 0x80 || (*(ptr + 2) & 0xC0) != 0x80 || (*(ptr + 3) & 0xC0) != 0x80)
                return 0;
            ptr += 4;
        } else {
            return 0;
        }
    }
    return 1;
}

/* Get byte offset to codepoint at char_index (codepoint-based indexing) */
size_t utf8_byte_offset(const char *str, size_t char_index)
{
    if (str == NULL)
        return 0;

    size_t count = 0;
    size_t byte_offset = 0;
    const char *ptr = str;

    while (*ptr && count < char_index) {
        int bytes = utf8_char_bytes(ptr);
        byte_offset += bytes;
        ptr += bytes;
        count++;
    }

    return byte_offset;
}

/* Get byte offset to grapheme cluster at grapheme_index (grapheme-based indexing).
 * This counts visible characters, skipping variation selectors, combining marks, and ZWJ. */
size_t utf8_grapheme_byte_offset(const char *str, size_t grapheme_index)
{
    if (str == NULL)
        return 0;

    size_t count = 0;
    const char *ptr = str;

    while (*ptr) {
        int cp = utf8_get_codepoint(ptr);

        /* Check if this is a grapheme start (non-extender) */
        if (cp >= 0 && !is_grapheme_extender(cp)) {
            if (count >= grapheme_index) {
                /* We've reached the target grapheme start */
                return ptr - str;
            }
            count++;
        }

        /* Move to next codepoint */
        ptr += utf8_char_bytes(ptr);
    }

    /* Reached end of string */
    return ptr - str;
}

/* Get byte length of grapheme cluster starting at ptr.
 * Returns the total bytes including any following extenders (VS, combining marks, ZWJ). */
size_t utf8_grapheme_bytes(const char *ptr)
{
    if (ptr == NULL || *ptr == '\0')
        return 0;

    size_t total_bytes = 0;
    const char *p = ptr;

    /* Get the base character */
    int bytes = utf8_char_bytes(p);
    total_bytes += bytes;
    p += bytes;

    /* Include any following grapheme extenders */
    while (*p) {
        int cp = utf8_get_codepoint(p);
        if (cp < 0 || !is_grapheme_extender(cp))
            break;
        bytes = utf8_char_bytes(p);
        total_bytes += bytes;
        p += bytes;
    }

    return total_bytes;
}

/* Count bytes in UTF-8 character at ptr */
int utf8_char_bytes(const char *ptr)
{
    if (ptr == NULL || *ptr == '\0')
        return 0;

    if ((*ptr & 0x80) == 0) {
        return 1; /* ASCII */
    } else if ((*ptr & 0xE0) == 0xC0) {
        return 2; /* 2-byte */
    } else if ((*ptr & 0xF0) == 0xE0) {
        return 3; /* 3-byte */
    } else if ((*ptr & 0xF8) == 0xF0) {
        return 4; /* 4-byte */
    }
    return 1; /* Invalid, treat as 1 byte */
}

/* Get Unicode codepoint from UTF-8 character */
int utf8_get_codepoint(const char *ptr)
{
    if (ptr == NULL || *ptr == '\0')
        return -1;

    if ((*ptr & 0x80) == 0) {
        return (unsigned char)*ptr; /* ASCII */
    } else if ((*ptr & 0xE0) == 0xC0) {
        return ((ptr[0] & 0x1F) << 6) | (ptr[1] & 0x3F);
    } else if ((*ptr & 0xF0) == 0xE0) {
        return ((ptr[0] & 0x0F) << 12) | ((ptr[1] & 0x3F) << 6) | (ptr[2] & 0x3F);
    } else if ((*ptr & 0xF8) == 0xF0) {
        return ((ptr[0] & 0x07) << 18) | ((ptr[1] & 0x3F) << 12) | ((ptr[2] & 0x3F) << 6) | (ptr[3] & 0x3F);
    }
    return -1; /* Invalid */
}

/* Encode Unicode codepoint as UTF-8 into buffer */
/* Returns number of bytes written (1-4) or 0 if invalid */
/* Buffer must have space for at least 5 bytes (4 UTF-8 bytes + null terminator) */
int utf8_put_codepoint(unsigned int codepoint, char *buf)
{
    if (buf == NULL)
        return 0;

    if (codepoint < 0x80) {
        /* 1-byte sequence (ASCII) */
        buf[0] = (char)codepoint;
        buf[1] = '\0';
        return 1;
    } else if (codepoint < 0x800) {
        /* 2-byte sequence */
        buf[0] = (char)(0xC0 | (codepoint >> 6));
        buf[1] = (char)(0x80 | (codepoint & 0x3F));
        buf[2] = '\0';
        return 2;
    } else if (codepoint < 0x10000) {
        /* 3-byte sequence */
        buf[0] = (char)(0xE0 | (codepoint >> 12));
        buf[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (codepoint & 0x3F));
        buf[3] = '\0';
        return 3;
    } else if (codepoint < 0x110000) {
        /* 4-byte sequence */
        buf[0] = (char)(0xF0 | (codepoint >> 18));
        buf[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buf[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buf[3] = (char)(0x80 | (codepoint & 0x3F));
        buf[4] = '\0';
        return 4;
    }
    /* Invalid codepoint */
    buf[0] = '\0';
    return 0;
}

/* East Asian Ambiguous codepoints: explicitly narrow (width 1) in ditty,
 * matching coffer's no-locale model. Kept as a sorted range table for parity
 * and so future locale-aware work has a hook. */
static const struct
{
    uint32_t lo;
    uint32_t hi;
} ambiguous_ranges[] = {
    { 0x00A1, 0x00A1 },
    { 0x00A4, 0x00A4 },
    { 0x00A7, 0x00A8 },
    { 0x00AA, 0x00AA },
    { 0x00AD, 0x00AE },
    { 0x00B1, 0x00B1 },
    { 0x00B4, 0x00B4 },
    { 0x00B6, 0x00B6 },
    { 0x00B8, 0x00B8 },
    { 0x00BB, 0x00BC },
    { 0x00BF, 0x00BF },
    { 0x00C6, 0x00C6 },
    { 0x00D0, 0x00D0 },
    { 0x00D7, 0x00D7 },
    { 0x00D8, 0x00D8 },
    { 0x00DE, 0x00E1 },
    { 0x00E6, 0x00E6 },
    { 0x00E8, 0x00EA },
    { 0x00EC, 0x00ED },
    { 0x00F0, 0x00F0 },
    { 0x00F2, 0x00F3 },
    { 0x00F7, 0x00F7 },
    { 0x00F9, 0x00FA },
    { 0x00FC, 0x00FC },
    { 0x00FE, 0x00FE },
    { 0x0101, 0x0101 },
    { 0x0111, 0x0111 },
    { 0x0113, 0x0113 },
    { 0x011B, 0x011B },
    { 0x0126, 0x0127 },
    { 0x012B, 0x012B },
    { 0x0133, 0x0133 },
    { 0x0138, 0x0138 },
    { 0x013F, 0x0142 },
    { 0x0144, 0x0144 },
    { 0x0148, 0x014B },
    { 0x014D, 0x014D },
    { 0x0152, 0x0153 },
    { 0x0166, 0x0167 },
    { 0x016B, 0x016B },
    { 0x01CE, 0x01CE },
    { 0x01D0, 0x01D0 },
    { 0x01D2, 0x01D2 },
    { 0x01D4, 0x01D4 },
    { 0x01D6, 0x01D6 },
    { 0x01D8, 0x01D8 },
    { 0x01DA, 0x01DA },
    { 0x01DC, 0x01DC },
    { 0x0251, 0x0251 },
    { 0x0261, 0x0261 },
    { 0x026B, 0x026B },
    { 0x02C4, 0x02C4 },
    { 0x02C7, 0x02C7 },
    { 0x02C9, 0x02CB },
    { 0x02CD, 0x02CD },
    { 0x02D0, 0x02D0 },
    { 0x02D8, 0x02DB },
    { 0x02DD, 0x02DD },
    { 0x02DF, 0x02DF },
    { 0x0300, 0x036F },
    { 0x0391, 0x03A1 },
    { 0x03A3, 0x03A9 },
    { 0x03B1, 0x03C1 },
    { 0x03C3, 0x03C9 },
    { 0x0401, 0x0401 },
    { 0x0410, 0x044F },
    { 0x0451, 0x0451 },
    { 0x2010, 0x2010 },
    { 0x2013, 0x2016 },
    { 0x2018, 0x2019 },
    { 0x201C, 0x201D },
    { 0x2020, 0x2022 },
    { 0x2024, 0x2027 },
    { 0x2030, 0x2030 },
    { 0x2032, 0x2033 },
    { 0x2035, 0x2035 },
    { 0x203B, 0x203B },
    { 0x203E, 0x203E },
    { 0x2074, 0x2074 },
    { 0x207F, 0x207F },
    { 0x2081, 0x2084 },
    { 0x20AC, 0x20AC },
    { 0x2103, 0x2103 },
    { 0x2105, 0x2105 },
    { 0x2109, 0x2109 },
    { 0x2113, 0x2113 },
    { 0x2116, 0x2116 },
    { 0x2121, 0x2122 },
    { 0x2126, 0x2126 },
    { 0x212B, 0x212B },
    { 0x2153, 0x2154 },
    { 0x215B, 0x215E },
    { 0x2160, 0x216B },
    { 0x2170, 0x2179 },
    { 0x2189, 0x2189 },
    { 0x2190, 0x2199 },
    { 0x21B8, 0x21B9 },
    { 0x21D2, 0x21D2 },
    { 0x21D4, 0x21D4 },
    { 0x21E7, 0x21E7 },
    { 0x2200, 0x2200 },
    { 0x2202, 0x2203 },
    { 0x2207, 0x2208 },
    { 0x220B, 0x220B },
    { 0x220F, 0x220F },
    { 0x2211, 0x2211 },
    { 0x2215, 0x2215 },
    { 0x221A, 0x221A },
    { 0x221D, 0x2220 },
    { 0x2223, 0x2223 },
    { 0x2225, 0x2225 },
    { 0x2227, 0x222C },
    { 0x222E, 0x222E },
    { 0x2234, 0x2237 },
    { 0x223C, 0x223D },
    { 0x2248, 0x2248 },
    { 0x224C, 0x224C },
    { 0x2252, 0x2252 },
    { 0x2260, 0x2261 },
    { 0x2264, 0x2267 },
    { 0x226A, 0x226B },
    { 0x226E, 0x226F },
    { 0x2282, 0x2283 },
    { 0x2286, 0x2287 },
    { 0x2295, 0x2295 },
    { 0x2299, 0x2299 },
    { 0x22A5, 0x22A5 },
    { 0x22BF, 0x22BF },
    { 0x2312, 0x2312 },
    { 0x2460, 0x24E9 },
    { 0x24EB, 0x254B },
    { 0x2550, 0x2573 },
    { 0x2580, 0x2580 },
    { 0x2592, 0x2595 },
    { 0x25A0, 0x25A1 },
    { 0x25A3, 0x25A9 },
    { 0x25B2, 0x25B3 },
    { 0x25B6, 0x25B7 },
    { 0x25BC, 0x25BD },
    { 0x25C0, 0x25C1 },
    { 0x25C6, 0x25C8 },
    { 0x25CB, 0x25CB },
    { 0x25CE, 0x25D1 },
    { 0x25E2, 0x25E5 },
    { 0x25EF, 0x25EF },
    { 0x2605, 0x2606 },
    { 0x2609, 0x2609 },
    { 0x260E, 0x260F },
    { 0x261C, 0x261C },
    { 0x261E, 0x261E },
    { 0x2640, 0x2640 },
    { 0x2642, 0x2642 },
    { 0x2660, 0x2666 },
    { 0x266A, 0x266F },
    { 0x267C, 0x267D },
    { 0x2693, 0x2693 },
    { 0x26A0, 0x26A1 },
    { 0x26AA, 0x26AB },
    { 0x26BD, 0x26BE },
    { 0x26C4, 0x26C5 },
    { 0x26CE, 0x26CE },
    { 0x26D4, 0x26D4 },
    { 0x26EA, 0x26EA },
    { 0x26F2, 0x26F3 },
    { 0x26F5, 0x26F5 },
    { 0x26FA, 0x26FA },
    { 0x26FD, 0x26FD },
    { 0x2705, 0x2705 },
    { 0x270A, 0x270B },
    { 0x2728, 0x2728 },
    { 0x274C, 0x274C },
    { 0x274E, 0x274E },
    { 0x2753, 0x2755 },
    { 0x2757, 0x2757 },
    { 0x275F, 0x2760 },
    { 0x2764, 0x2764 },
    { 0x2795, 0x2797 },
    { 0x27B0, 0x27B0 },
    { 0x27BF, 0x27BF },
    { 0x27E6, 0x27EB },
    { 0x28FF, 0x28FF },
    { 0x2B1B, 0x2B1C },
    { 0x2B50, 0x2B50 },
    { 0x2B55, 0x2B55 },
    { 0x2E80, 0x2E99 },
    { 0x2E9B, 0x2EF3 },
    { 0x2F00, 0x2FD5 },
    { 0x2FF0, 0x2FFB },
    { 0x3000, 0x303E },
    { 0x3041, 0x3096 },
    { 0x3099, 0x30FF },
    { 0x3105, 0x312F },
    { 0x3131, 0x318E },
    { 0x3190, 0x31BA },
    { 0x31C0, 0x31E3 },
    { 0x31F0, 0x321C },
    { 0x3220, 0x324F },
    { 0x3260, 0x327F },
    { 0x3280, 0x32B0 },
    { 0x32C0, 0x32FE },
    { 0x3300, 0x33FF },
    { 0x3400, 0x4DBF },
    { 0x4E00, 0x9FFF },
    { 0xA000, 0xA48C },
    { 0xA490, 0xA4C6 },
    { 0xA960, 0xA97C },
    { 0xAC00, 0xD7A3 },
    { 0xD7B0, 0xD7C6 },
    { 0xD7CB, 0xD7FB },
    { 0xE000, 0xF8FF },
    { 0xF900, 0xFAFF },
    { 0xFE10, 0xFE19 },
    { 0xFE30, 0xFE52 },
    { 0xFE54, 0xFE66 },
    { 0xFE68, 0xFE6B },
    { 0xFF01, 0xFF60 },
    { 0xFFE0, 0xFFE6 },
    { 0x16FE0, 0x16FE1 },
    { 0x17000, 0x187F7 },
    { 0x18800, 0x18AF2 },
    { 0x1B000, 0x1B11E },
    { 0x1B150, 0x1B152 },
    { 0x1B164, 0x1B167 },
    { 0x1B170, 0x1B2FB },
    { 0x1F004, 0x1F004 },
    { 0x1F0CF, 0x1F0CF },
    { 0x1F18E, 0x1F18E },
    { 0x1F191, 0x1F19A },
    { 0x1F200, 0x1F202 },
    { 0x1F210, 0x1F23B },
    { 0x1F240, 0x1F248 },
    { 0x1F250, 0x1F251 },
    { 0x1F260, 0x1F265 },
    { 0x1F300, 0x1F320 },
    { 0x1F32D, 0x1F335 },
    { 0x1F337, 0x1F37C },
    { 0x1F37E, 0x1F393 },
    { 0x1F3A0, 0x1F3CA },
    { 0x1F3CF, 0x1F3D3 },
    { 0x1F3E0, 0x1F3F0 },
    { 0x1F3F4, 0x1F3F4 },
    { 0x1F3F8, 0x1F43E },
    { 0x1F440, 0x1F440 },
    { 0x1F442, 0x1F4FC },
    { 0x1F4FF, 0x1F53D },
    { 0x1F549, 0x1F54E },
    { 0x1F550, 0x1F567 },
    { 0x1F56F, 0x1F570 },
    { 0x1F573, 0x1F57A },
    { 0x1F587, 0x1F587 },
    { 0x1F58A, 0x1F58D },
    { 0x1F590, 0x1F590 },
    { 0x1F595, 0x1F596 },
    { 0x1F5A4, 0x1F5A5 },
    { 0x1F5A8, 0x1F5A8 },
    { 0x1F5B1, 0x1F5B2 },
    { 0x1F5BC, 0x1F5BC },
    { 0x1F5C2, 0x1F5C4 },
    { 0x1F5D1, 0x1F5D3 },
    { 0x1F5DC, 0x1F5DE },
    { 0x1F5E1, 0x1F5E1 },
    { 0x1F5E3, 0x1F5E3 },
    { 0x1F5E8, 0x1F5E8 },
    { 0x1F5EF, 0x1F5EF },
    { 0x1F5F3, 0x1F5F3 },
    { 0x1F5FA, 0x1F5FA },
    { 0x1F5FB, 0x1F64F },
    { 0x1F680, 0x1F6C5 },
    { 0x1F6CB, 0x1F6D2 },
    { 0x1F6D5, 0x1F6D5 },
    { 0x1F6E9, 0x1F6E9 },
    { 0x1F6EB, 0x1F6EC },
    { 0x1F6F0, 0x1F6F0 },
    { 0x1F6F3, 0x1F6F7 },
    { 0x1F700, 0x1F773 },
    { 0x1F780, 0x1F7D4 },
    { 0x1F800, 0x1F80B },
    { 0x1F810, 0x1F847 },
    { 0x1F850, 0x1F859 },
    { 0x1F860, 0x1F887 },
    { 0x1F890, 0x1F8AD },
    { 0x1F900, 0x1F90B },
    { 0x1F90D, 0x1F971 },
    { 0x1F973, 0x1F976 },
    { 0x1F97A, 0x1F9A2 },
    { 0x1F9A5, 0x1F9AA },
    { 0x1F9AE, 0x1F9CA },
    { 0x1F9CD, 0x1FA53 },
    { 0x1FA60, 0x1FA6D },
    { 0x1FA70, 0x1FA73 },
    { 0x1FA78, 0x1FA7A },
    { 0x1FA80, 0x1FA82 },
    { 0x1FA90, 0x1FA95 },
    { 0x20000, 0x2A6DF },
    { 0x2A700, 0x2B73F },
    { 0x2B740, 0x2B81F },
    { 0x2B820, 0x2CEAF },
    { 0x2CEB0, 0x2EBEF },
    { 0x2F800, 0x2FA1F },
    { 0x30000, 0x3134A },
};

static int ambiguous_width(int cp)
{
    if (cp < 0)
        return 0;

    size_t lo = 0;
    size_t hi = sizeof(ambiguous_ranges) / sizeof(ambiguous_ranges[0]);
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (cp < (int)ambiguous_ranges[mid].lo)
            hi = mid;
        else if (cp > (int)ambiguous_ranges[mid].hi)
            lo = mid + 1;
        else
            return 1;
    }
    return 0;
}

static int is_regional_indicator(uint32_t cp)
{
    return cp >= 0x1F1E6 && cp <= 0x1F1FF;
}

static int is_extended_pictographic(uint32_t cp)
{
    return utf8_is_emoji_presentation(cp) || utf8_is_emoji_property(cp) ||
           (cp >= 0x1F300 && cp <= 0x1F5FF) || (cp >= 0x1F600 && cp <= 0x1F64F) ||
           (cp >= 0x1F680 && cp <= 0x1F6FF) || (cp >= 0x1F900 && cp <= 0x1F9FF) ||
           (cp >= 0x1FA00 && cp <= 0x1FAFF);
}

static int is_grapheme_extend(uint32_t cp)
{
    return (cp >= 0xFE00 && cp <= 0xFE0F) || (cp >= 0xE0100 && cp <= 0xE01EF) ||
           (cp >= 0x0300 && cp <= 0x036F) || (cp >= 0x1AB0 && cp <= 0x1AFF) ||
           (cp >= 0x1DC0 && cp <= 0x1DFF) || (cp >= 0x20D0 && cp <= 0x20FF) ||
           (cp >= 0xFE20 && cp <= 0xFE2F) || cp == 0x200D;
}

static int is_spacing_mark(uint32_t cp)
{
    (void)cp;
    return 0;
}

bool utf8_grapheme_break_before(uint32_t prev, uint32_t cur)
{
    if (prev == 0)
        return true;

    /* GB3: CR x LF */
    if (prev == 0x0D && cur == 0x0A)
        return false;

    /* GB9, GB9a: x Extend | ZWJ | SpacingMark */
    if (is_grapheme_extend(cur) || is_spacing_mark(cur))
        return false;

    /* GB11: Extended_Pictographic Extend* ZWJ x Extended_Pictographic
     * Conservative stateless approximation: ZWJ sticks to whatever follows. */
    if (prev == 0x200D)
        return false;

    /* GB12: ^(RI RI)* RI x RI */
    if (is_regional_indicator(prev) && is_regional_indicator(cur))
        return false;

    /* GB999: any / any */
    return true;
}

int utf8_cluster_width(const uint32_t *cps, size_t len)
{
    if (cps == NULL || len == 0)
        return 0;

    uint32_t head = cps[0];

    /* RI pair -> 2 */
    if (len >= 2 && is_regional_indicator(head) && is_regional_indicator(cps[1]))
        return 2;

    int has_vs15 = 0;
    int has_vs16 = 0;
    int has_zwj = 0;
    for (size_t i = 1; i < len; i++) {
        if (cps[i] == 0xFE0E)
            has_vs15 = 1;
        else if (cps[i] == 0xFE0F)
            has_vs16 = 1;
        else if (cps[i] == 0x200D)
            has_zwj = 1;
    }

    if (has_vs15)
        return 1;
    if (has_vs16)
        return 2;

    /* ZWJ emoji family: head is pictographic, cluster contains ZWJ, no VS15 */
    if (has_zwj && is_extended_pictographic(head))
        return 2;

    return utf8_codepoint_width((int)head);
}

/* Get display width of a single codepoint (base width, no cluster selectors) */
int utf8_codepoint_width(int cp)
{
    if (cp < 0)
        return 0;

    /* Control characters */
    if (cp < 0x20 || (cp >= 0x7F && cp < 0xA0))
        return 0;

    /* Combining characters (zero width) */
    if ((cp >= 0x0300 && cp <= 0x036F) || /* Combining Diacritical Marks */
        (cp >= 0x1AB0 && cp <= 0x1AFF) || /* Combining Diacritical Marks Extended */
        (cp >= 0x1DC0 && cp <= 0x1DFF) || /* Combining Diacritical Marks Supplement */
        (cp >= 0x20D0 && cp <= 0x20FF) || /* Combining Diacritical Marks for Symbols */
        (cp >= 0xFE20 && cp <= 0xFE2F))   /* Combining Half Marks */
        return 0;

    /* Variation selectors (zero width) */
    if ((cp >= 0xFE00 && cp <= 0xFE0F) || /* Variation Selectors */
        (cp >= 0xE0100 && cp <= 0xE01EF)) /* Variation Selectors Supplement */
        return 0;

    /* Wide characters (width 2) */
    /* CJK ranges */
    if ((cp >= 0x1100 && cp <= 0x115F) ||   /* Hangul Jamo */
        (cp >= 0x2E80 && cp <= 0x9FFF) ||   /* CJK blocks */
        (cp >= 0xAC00 && cp <= 0xD7A3) ||   /* Hangul Syllables */
        (cp >= 0xF900 && cp <= 0xFAFF) ||   /* CJK Compatibility Ideographs */
        (cp >= 0xFE10 && cp <= 0xFE1F) ||   /* Vertical Forms */
        (cp >= 0xFE30 && cp <= 0xFE6F) ||   /* CJK Compatibility Forms */
        (cp >= 0xFF00 && cp <= 0xFF60) ||   /* Fullwidth Forms */
        (cp >= 0xFFE0 && cp <= 0xFFE6) ||   /* Fullwidth Forms */
        (cp >= 0x20000 && cp <= 0x2FFFF) || /* CJK Extension B+ */
        (cp >= 0x30000 && cp <= 0x3FFFF))   /* CJK Extension G+ */
        return 2;

    /* Emoji with Emoji_Presentation=Yes (width 2) */
    if (utf8_is_emoji_presentation((uint32_t)cp))
        return 2;

    /* Emoji ranges (width 2) - Misc Symbols/Pictographs, Emoticons, Transport, etc */
    if ((cp >= 0x1F300 && cp <= 0x1F5FF) || /* Misc Symbols and Pictographs */
        (cp >= 0x1F600 && cp <= 0x1F64F) || /* Emoticons */
        (cp >= 0x1F680 && cp <= 0x1F6FF) || /* Transport/Map */
        (cp >= 0x1F900 && cp <= 0x1F9FF) || /* Supplemental Symbols */
        (cp >= 0x1FA00 && cp <= 0x1FAFF))   /* Chess, Extended-A */
        return 2;

    /* Default width 1 */
    return 1;
}

/* Calculate display width of UTF-8 string in terminal columns (grapheme-aware) */
int utf8_display_width(const char *str)
{
    if (str == NULL)
        return 0;

    /* First pass: decode to codepoints. */
    uint32_t stack[256];
    uint32_t *cps = stack;
    size_t cap = sizeof(stack) / sizeof(stack[0]);
    size_t len = 0;
    const char *ptr = str;

    while (*ptr) {
        int cp = utf8_get_codepoint(ptr);
        if (cp < 0)
            cp = 0xFFFD; /* replacement character */

        if (len >= cap) {
            size_t new_cap = cap * 2;
            uint32_t *new_cps;
            if (cps == stack) {
                new_cps = (uint32_t *)malloc(new_cap * sizeof(uint32_t));
                if (new_cps != NULL)
                    memcpy(new_cps, stack, cap * sizeof(uint32_t));
            } else {
                new_cps = (uint32_t *)realloc(cps, new_cap * sizeof(uint32_t));
            }
            if (new_cps == NULL) {
                if (cps != stack)
                    free(cps);
                return 0;
            }
            cps = new_cps;
            cap = new_cap;
        }
        cps[len++] = (uint32_t)cp;
        ptr = utf8_next_char(ptr);
        if (ptr == NULL)
            break;
    }

    /* Second pass: segment graphemes and sum cluster widths. */
    int width = 0;
    size_t start = 0;
    for (size_t i = 1; i < len; i++) {
        if (utf8_grapheme_break_before(cps[i - 1], cps[i])) {
            width += utf8_cluster_width(&cps[start], i - start);
            start = i;
        }
    }
    if (len > 0)
        width += utf8_cluster_width(&cps[start], len - start);

    if (cps != stack)
        free(cps);

    return width;
}
