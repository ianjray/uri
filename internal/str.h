#pragma once

// Private API.

#include <stdbool.h>

/// Safe duplicate
/// @return Copy of @c str, otherwise NULL if @c str NULL or out of memory
char *safe_strdup(const char *str);

/// Iterate over characters.
/// @return Reference to moved iterator if any characters matched, or NULL otherwise.
char *str_iterate(char *iter, bool (*first)(int), bool (*next)(int));

/// Reverse iterator that skips over digits.
void str_reverse_iterate_digits(char *str, char **iter);

/// Reverse iterator that skips over @c c.
/// @return bool True if @c matched.
bool str_reverse_iterate_one(char *str, char **iter, int c);

/// Split string at iterator.
/// @return Copy of lhs.
char *str_split_left(char *str, char *iter);

/// Delete matching string prefix @c seq.
/// @return bool True if matched.
bool str_prefix_delete(char *str, const char *seq);

/// Delete string contents if it matches @c pattern.
/// @return bool True if matched.
bool str_delete(char *str, const char *pattern);

/// Split string by NUL terminating at current position and returning remainder.
/// @return Copy of suffix.
char *str_split(char *str);

/// Detach string suffix beginning after first occurence of @c c.
/// @return Copy of suffix if @c c found, otherwise NULL.
char *str_suffix_detach(char *str, int c);

/// Detach string suffix beginning with first occurence of @c c.
/// @return Copy of suffix if @c c found, otherwise NULL.
char *str_suffix_detach_inclusive(char *str, int c);

/// Append @c suffix to @c str, reallocating @c str to have sufficient space.
void str_append(char **str, const char *suffix);

/// Convert string to lowercase.
/// @return Reference to @c str.
char *str_lowercase(char *str);

/// Remove first path segment including initial "/" (if any) up to (the end of) a sequence of "/".
char *str_remove_first_path_segment(char *str);

/// Filter out all occurences of @c c in @c str.
/// @return Reference to @c str.
char *str_filter_out(char *str, int c);
