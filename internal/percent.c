#include "percent.h"

#include "charset.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/// @return ASCII hex character @c c converted to an integer.
static int asciihex_to_int(int c)
{
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('A' <= c && c <= 'F') {
        return 0x0A + c - 'A';
    }

    return 0;
}

bool normalize_percent_encodings(char *str)
{
    // §2.3
    // For consistency, percent-encoded octets in the ranges of ALPHA (%41-%5A and %61-%7A), DIGIT (%30-%39), hyphen (%2D), period (%2E), underscore (%5F), or tilde (%7E) should
    // not be created by URI producers and, when found in a URI, should be decoded to their corresponding unreserved characters by URI normalizers.

    for (; str && *str; ++str) {
        if (str[0] == '%') {
            // pct-encoded   = "%" HEXDIG HEXDIG
            if (!isxdigit(str[1]) || !isxdigit(str[2])) {
                // Invalid HEXDIG.
                return false;

            } else {
                int v;

                // §2.1
                // For consistency, URI producers and normalizers should use uppercase hexadecimal digits for all percent-encodings.

                str[1] = (char)toupper(str[1]);
                str[2] = (char)toupper(str[2]);

                v = (asciihex_to_int(str[1]) << 4) + asciihex_to_int(str[2]);
                if (v == 0) {
                    // §7.3
                    // Disallow NUL.
                    return false;

                } else if (is_unreserved(v)) {
                    // Remove redundant percent-encoding.
                    str[0] = (char)v;
                    memmove(&str[1], &str[3], strlen(str + 3) + 1 /*NUL*/);
                }
            }
        }
    }

    return true;
}

bool percent_decode(char *str)
{
    for (; str && *str; ++str) {
        if (str[0] == '%') {
            // pct-encoded   = "%" HEXDIG HEXDIG
            str[0] = (char)((asciihex_to_int(toupper(str[1])) << 4) + asciihex_to_int(toupper(str[2])));
            memmove(&str[1], &str[3], strlen(str + 3) + 1 /*NUL*/);
        }
    }

    return true;
}

char *percent_encode(const char *str, bool (*predicate)(int))
{
    // §2.1
    // For consistency, URI producers and normalizers should use uppercase hexadecimal digits for all percent-encodings.

    static const char *hex = "0123456789ABCDEF";

    size_t len = 0;
    char *encoded = NULL;

    if (!str) {
        return NULL;
    }

    for (const char *src = str; *src; ++src) {
        if (*src == '%' || !predicate(*src)) {
            len += 2;
        }
        len++;
    }

    encoded = (char *)malloc(len + 1 /*NUL*/);
    if (encoded) {
        char *dest = encoded;
        while (*str) {
            if (*str == '%' || !predicate(*str)) {
                *dest++ = '%';
                *dest++ = hex[(*str >> 4) & 0xF];
                *dest++ = hex[ *str       & 0xF];
                str++;
            } else {
                *dest++ = *str++;
            }
        }
        *dest = '\0';
    }

    return encoded;
}
