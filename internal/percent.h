#pragma once

// Private API.
// Percent-encoding / decoding.

#include <stdbool.h>
#include <stddef.h>

/// Percent encode any non-ASCII characters in @c source.
/// @return Copy of @c source.
/// @note Memory ownership: Caller must free() the returned pointer.
char *percent_encode_non_ascii_characters(const char *source);

/// Normalize percent encodings.
/// Ensure percent encodings are uppercase of the form @c %XX and not redundantly encoding an unreserved character.
/// @return 0 on success.
/// @return -1 on failure, and errno is set to:
///   - EINVAL: Invalid encoding.
///   - EILSEQ: Rejected %00 (since future decode would produce a truncated string).
int percent_encoded_string_normalize(char *encoded_str);

/// Transform string to lowercase in-place.
/// This function ignores @c %XX which are left in the current case.
void percent_aware_lowercase(char *encoded_str);
