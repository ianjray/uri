#pragma once

// Private API.

#include <stdbool.h>

/// Normalize percent encodings.
/// Ensure percent encodings are uppercase of the form `%XX` and not redundantly encoding an unreserved character.
/// @note
/// `%00` (NUL) is disallowed.
/// @return True on success.
bool normalize_percent_encodings(char *str);

/// Decode percent-encodings.
/// @note
/// This function must be called after @c normalize_percent_encodings.
/// @return True on success.
bool percent_decode(char *str);

/// Encode unsafe characters in @c str as defined by @c predicate.
/// @return Copy of @c str.
char *percent_encode(const char *str, bool (*predicate)(int));
