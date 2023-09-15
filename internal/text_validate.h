#pragma once

// Private API.
// UTF-8 + control character validation.
// "Is this safe, well-formed text?"

#include <stdbool.h>

/// @return True if character @c is in the range 0x20..0x7F.
bool is_ascii(unsigned char c);

/// Check that string @c ascii_str is valid ASCII.
/// @return True if string is valid ASCII, false otherwise.
bool ascii_validate(const char *ascii_str);

/// Check that string @c utf8_str is valid UTF-8.
/// @return True if string is valid UTF-8, false otherwise.
bool utf8_validate(const char *utf8_str);

/// Detect ASCII C0 control characters (U+0000–U+001F, U+007F).
/// @see https://en.wikipedia.org/wiki/C0_and_C1_control_codes
/// @return True if @c is an ASCII C0 control character.
bool control_c0(unsigned char c);

/// @return True if string contains a C0 control character.
/// @see control_c0.
bool contains_c0_controls_ascii(const char *ascii_str);

/// Detect Unicode code points corresponding to C0 or C1 controls.
/// @see https://en.wikipedia.org/wiki/C0_and_C1_control_codes
/// @return True if string contains a C0 or C1 code point.
bool contains_c0_c1_controls_utf8(const char *utf8_str);
