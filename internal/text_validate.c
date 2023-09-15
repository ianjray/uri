#include "text_validate.h"

bool is_ascii(unsigned char c)
{
    return c >= 0x20 && c <= 0x7F;
}

bool ascii_validate(const char *ascii_str)
{
    const unsigned char *p = (const unsigned char *)ascii_str;

    while (*p) {
        unsigned char c = *p++;

        if (c >= 0x80) {
            return false;
        }
    }

    return true;
}

bool utf8_validate(const char *utf8_str)
{
    const unsigned char *p = (const unsigned char *)utf8_str;

    while (*p) {
        unsigned char c = *p;

        if (c <= 0x7F) {
            p += 1;

        } else if ((c & 0xE0) == 0xC0) {
            if (
                    // Truncated.
                    p[1] == 0

                    // Invalid continuation.
                 || (p[1] & 0xC0) != 0x80

                    // C0 80 → U+0000
                    // C1 xx → U+0001–U+007F
                    // C2 80 → U+0080 (first valid 2-byte character)
                    // Overlong sequence.
                 || c < 0xC2

                ) {
                return false;
            }

            p += 2;

        } else if ((c & 0xF0) == 0xE0) {
            if (
                    // Truncated.
                    p[1] == 0
                 || p[2] == 0

                    // Invalid continuation.
                 || (p[1] & 0xC0) != 0x80
                 || (p[2] & 0xC0) != 0x80

                    // E0 A0 80 → U+0800
                    // Overlong sequence.
                 || (c == 0xE0 && p[1] < 0xA0)

                    // ED A0 80 → U+D800
                    // ED BF BF → U+DFFF
                    // Unicode code points U+D800 to U+DFFF are surrogate halves used in UTF-16 encoding.
                    // UTF-8 must never encode these; they are forbidden.
                    // Note: No check for 0xBF since all valid UTF-8 second bytes are ≤ 0xBF.
                 || (c == 0xED && p[1] >= 0xA0)

                ) {
                return false;
            }

            p += 3;

        } else if ((c & 0xF8) == 0xF0) {
            if (
                    // Truncated.
                    p[1] == 0
                 || p[2] == 0
                 || p[3] == 0

                    // Invalid continuation.
                 || (p[1] & 0xC0) != 0x80
                 || (p[2] & 0xC0) != 0x80
                 || (p[3] & 0xC0) != 0x80

                    // F0 90 80 80 → U+10000
                    // Overlong sequence.
                || (c == 0xF0 && p[1] < 0x90)

                    // Exceeds maximum code point U+10FFFF.
                || (c == 0xF4 && p[1] >= 0x90)
                || (c > 0xF4)

                ) {
                return false;
            }

            p += 4;

        } else {
            // Invalid first byte.
            return false;
        }
    }

    return true;
}

/// @return True if character is a  C0 control (U+0000 to U+001F) and U+007F.
bool control_c0(unsigned char c)
{
    return c <= 0x1F || c == 0x7F;
}

bool contains_c0_controls_ascii(const char *ascii_str)
{
    const unsigned char *p = (const unsigned char *)ascii_str;

    while (*p) {
        unsigned char c = *p++;

        if (control_c0(c)) {
            return true;
        }
    }

    return false;
}

/// @return True if string contains a Unicode code point corresponding to C1 controls.
static bool control_c1(const unsigned char *utf8_str)
{
    // C1 controls (U+0080 to U+009F).
    // Byte 1: 0xC2
    // Byte 2: 0x80 to 0x9F
    return *utf8_str++ == 0xC2 && *utf8_str >= 0x80 && *utf8_str <= 0x9F;
}

bool contains_c0_c1_controls_utf8(const char *utf8_str)
{
    const unsigned char *p = (const unsigned char *)utf8_str;

    while (*p) {
        unsigned char c = *p++;

        if (control_c0(c)) {
            return true;
        }

        if (control_c1(p)) {
            return true;
        }
    }

    return false;
}
