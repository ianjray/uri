#include "percent.h"

#include "rfc_charset.h"
#include "text_validate.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

char *percent_encode_non_ascii_characters(const char *input)
{
    size_t len = 0;
    char *output = NULL;
    char *dest = NULL;

    // Two-pass string assembler.
    // Calculate size in first pass, then allocate memory and populate string in second pass.
    for (int pass = 0; pass < 2; ++pass) {
        // §2.1
        // For consistency, URI producers and normalizers should use uppercase hexadecimal digits for all percent-encodings.
        static const char *hex = "0123456789ABCDEF";

        for (const char *src = input; *src; ++src) {
            if (!is_ascii((unsigned char)*src)) {
                len += 3;
                if (dest) {
                    *dest++ = '%';
                    *dest++ = hex[(*src >> 4) & 0xF];
                    *dest++ = hex[ *src       & 0xF];
                }

            } else {
                len++;
                if (dest) {
                    *dest++ = *src;
                }
            }
        }

        if (pass == 0) {
            output = malloc(len + 1 /*NUL*/);
            if (!output) {
                return NULL;
            }

            dest = output;
        }
    }

    *dest = '\0';
    return output;
}

/// @return ASCII hex character @c c converted to an integer.
static int asciihex_to_int(int c)
{
    if ('0' <= c && c <= '9') {
        return c - '0';

    } else if ('a' <= c && c <= 'f') {
        return 0xA + c - 'a';

    } else if ('A' <= c && c <= 'F') {
        return 0xA + c - 'A';
    }

    return 0;
}

int percent_encoded_string_normalize(char *str)
{
    for (; *str; ++str) {
        unsigned char c = (unsigned char)*str;

        if (str[0] == '%') {
            if (!isxdigit(str[1]) || !isxdigit(str[2])) {
                // Invalid HEXDIG.
                errno = EINVAL;
                return -1;
            }

            c = (unsigned char)((asciihex_to_int(str[1]) << 4) + asciihex_to_int(str[2]));

            if (is_unreserved(c)) {
                // §2.3
                // For consistency, percent-encoded octets in the ranges of ALPHA (%41-%5A and %61-%7A), DIGIT (%30-%39), hyphen (%2D), period (%2E), underscore (%5F), or tilde (%7E) should
                // not be created by URI producers and, when found in a URI, should be decoded to their corresponding unreserved characters by URI normalizers.
                str[0] = (char)c;
                memmove(&str[1], &str[3], strlen(str + 3) + 1 /*NUL*/);

            } else {
                // §3.1
                // For consistency, URI producers and normalizers should use uppercase hexadecimal digits for all percent-encodings.
                str[1] = (char)toupper(str[1]);
                str[2] = (char)toupper(str[2]);
            }
        }

        if (control_c0(c)) {
            // §7.3
            // Reject NUL since it could truncate an otherwise valid string.
            //
            // Reject ASCII C0 control characters (U+0000–U+001F, U+007F).
            // This is more strict than RFC 3986.
            errno = EINVAL;
            return -1;
        }
    }

    return 0;
}

void percent_aware_lowercase(char *encoded_str)
{
    for (; *encoded_str; ++encoded_str) {
        if (encoded_str[0] == '%') {
            encoded_str += 2;
        } else {
            *encoded_str = (char)tolower(*encoded_str);
        }
    }
}
