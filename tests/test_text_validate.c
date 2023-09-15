#include "text_validate.h"

#include <assert.h>
#include <stdio.h>

static void valid(const char *description, const char *s)
{
    (void)description;
    assert(utf8_validate(s));
}

static void invalid(const char *description, const char *s)
{
    (void)description;
    assert(!utf8_validate(s));
}

int main(void)
{
    valid("empty", "");

    // ASCII: Single byte (0x00–0x7F)
    valid("ASCII: NUL", "\x00");
    valid("ASCII: space", " ");
    valid("ASCII: all printable", "!\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~");
    valid("ASCII: DEL", "\x7F");

    // Two-byte sequences (0xC2–0xDF + continuation)
    valid("2-byte: U+0080 (©)", "\xC2\x80");
    valid("2-byte: U+00FF (ÿ)", "\xC3\xBF");
    valid("2-byte: U+0100 (Ā)", "\xC4\x80");
    valid("2-byte: U+07FF", "\xDF\xBF");
    valid("2-byte: multiple", "\xC2\x80\xC3\x80");

    // Three-byte sequences (0xE0–0xEF + 2 continuations)
    valid("3-byte: U+0800 (ࠀ)", "\xE0\xA0\x80");
    valid("3-byte: U+0900 (਀)", "\xE0\xA4\x80");
    valid("3-byte: U+1000 (က)", "\xE1\x80\x80");
    valid("3-byte: U+FFFD (replacement)", "\xEF\xBF\xBD");
    valid("3-byte: U+FFFE (last BMP)", "\xEF\xBF\xBE");
    valid("3-byte: U+FFFF (max)", "\xEF\xBF\xBF");
    valid("3-byte: before surrogates U+D7FF", "\xED\x9F\xBF");
    valid("3-byte: after surrogates U+E000", "\xEE\x80\x80");
    valid("3-byte: multiple", "\xE0\xA0\x80\xE1\x80\x80");

    // Four-byte sequences (0xF0–0xF4 + 3 continuations)
    valid("4-byte: U+10000 (𐀀)", "\xF0\x90\x80\x80");
    valid("4-byte: U+10FFFF (max)", "\xF4\x8F\xBF\xBF");
    valid("4-byte: multiple", "\xF0\x90\x80\x80\xF0\x9F\x98\x80");

    // Mixed valid sequences
    valid("mixed ASCII+2byte", "A\xC2\x80Z");
    valid("mixed ASCII+3byte", "A\xE0\xA0\x80Z");
    valid("mixed ASCII+4byte", "A\xF0\x9F\x98\x80Z");
    valid("mixed all types", "A\xC2\x80\xE0\xA0\x80\xF0\x90\x80\x80Z");

    // Invalid first byte
    invalid("invalid: 0x80 start", "\x80");
    invalid("invalid: 0x81 start", "\x81");
    invalid("invalid: 0xBF start", "\xBF");
    invalid("invalid: 0xC0 start", "\xC0");
    invalid("invalid: 0xFE start", "\xFE");
    invalid("invalid: 0xFF start", "\xFF");

    // Two-byte: Overlong sequences (C0–C1)
    invalid("2-byte overlong: C0 80 (U+0000)", "\xC0\x80");
    invalid("2-byte overlong: C1 80 (U+0040)", "\xC1\x80");
    invalid("2-byte overlong: C1 BF (U+007F)", "\xC1\xBF");

    // Two-byte: Missing continuation
    invalid("2-byte truncated: C2 EOF", "\xC2");
    invalid("2-byte bad cont: C2 00", "\xC2\x00");
    invalid("2-byte bad cont: C2 7F", "\xC2\x7F");  // Continuation byte must have high bit set
    invalid("2-byte bad cont: C2 C0", "\xC2\xC0");  // Not a valid continuation
    invalid("2-byte EOF in middle", "A\xC2");

    // Three-byte: Overlong sequences (E0 with second byte < 0xA0)
    invalid("3-byte overlong: E0 80 80 (U+0000)", "\xE0\x80\x80");
    invalid("3-byte overlong: E0 9F BF (U+07FF)", "\xE0\x9F\xBF");
    invalid("3-byte overlong: E0 99 80", "\xE0\x99\x80");

    // Three-byte: Surrogate pairs (ED A0 80 – ED BF BF, U+D800–U+DFFF)
    invalid("3-byte surrogate: ED A0 80 (U+D800)", "\xED\xA0\x80");
    invalid("3-byte surrogate: ED AD BF (U+DB7F)", "\xED\xAD\xBF");
    invalid("3-byte surrogate: ED AE 80 (U+DB80)", "\xED\xAE\x80");
    invalid("3-byte surrogate: ED AF BF (U+DBFF)", "\xED\xAF\xBF");
    invalid("3-byte surrogate: ED B0 80 (U+DC00)", "\xED\xB0\x80");
    invalid("3-byte surrogate: ED BE BF (U+DF7F)", "\xED\xBE\xBF");
    invalid("3-byte surrogate: ED BF BF (U+DFFF)", "\xED\xBF\xBF");
    invalid("surrogate pair", "\xED\xA0\xBD\xED\xB0\x80");

    // Three-byte: Missing/invalid continuation
    invalid("3-byte truncated: E0 A0 EOF", "\xE0\xA0");
    invalid("3-byte no second: E0 EOF", "\xE0");
    invalid("3-byte bad cont 1: E0 00 80", "\xE0\x00\x80");
    invalid("3-byte bad cont 2: E0 A0 00", "\xE0\xA0\x00");
    invalid("3-byte bad cont 2: E0 A0 7F", "\xE0\xA0\x7F");
    invalid("3-byte bad cont 2: E0 A0 C0", "\xE0\xA0\xC0");

    // Four-byte: Overlong sequences (F0 with second byte < 0x90)
    invalid("4-byte overlong: F0 80 80 80 (U+00000)", "\xF0\x80\x80\x80");
    invalid("4-byte overlong: F0 8F BF BF (U+FFFF)", "\xF0\x8F\xBF\xBF");
    invalid("4-byte overlong: F0 8E 80 80", "\xF0\x8E\x80\x80");

    // Four-byte: Exceeds max codepoint (F4 90–FF or F5–FF)
    invalid("4-byte overflow: F4 90 80 80 (U+110000)", "\xF4\x90\x80\x80");
    invalid("4-byte overflow: F4 BF BF BF (too high)", "\xF4\xBF\xBF\xBF");
    invalid("4-byte overflow: F5 80 80 80", "\xF5\x80\x80\x80");
    invalid("4-byte overflow: FF FF FF FF", "\xFF\xFF\xFF\xFF");

    // Four-byte: Missing/invalid continuation
    invalid("4-byte truncated: F0 90 80 EOF", "\xF0\x90\x80");
    invalid("4-byte truncated: F0 90 EOF", "\xF0\x90");
    invalid("4-byte no second: F0 EOF", "\xF0");
    invalid("4-byte bad cont 1: F0 00 80 80", "\xF0\x00\x80\x80");
    invalid("4-byte bad cont 2: F0 90 00 80", "\xF0\x90\x00\x80");
    invalid("4-byte bad cont 3: F0 90 80 00", "\xF0\x90\x80\x00");

    // Mixed valid then invalid
    invalid("valid then invalid 1", "A\x80");
    invalid("valid then invalid 2", "A\xC0\x80");
    invalid("valid then invalid 3", "A\xED\xA0\x80");

    // Continuation byte appearing alone
    invalid("lone continuation: 0x80", "\x80");
    invalid("lone continuation: 0x9F", "\x9F");
    invalid("lone continuation: 0xA0", "\xA0");
    invalid("lone continuation: 0xBF", "\xBF");
    invalid("lone continuation in middle", "A\x80Z");
    invalid("lone continuation at end", "A\x9F");

    // Incomplete sequences at end of string
    invalid("2-byte incomplete at end", "A\xDF");
    invalid("3-byte incomplete 1 byte at end", "A\xEF");
    invalid("3-byte incomplete 2 bytes at end", "A\xEF\xBF");
    invalid("4-byte incomplete 1 byte at end", "A\xF0");
    invalid("4-byte incomplete 2 bytes at end", "A\xF0\x90");
    invalid("4-byte incomplete 3 bytes at end", "A\xF0\x90\x80");

    printf("Testing completed.\n");
    return 0;
}
