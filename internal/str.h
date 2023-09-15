#pragma once

// Private API.
// String utilities.

#include <stdbool.h>
#include <stddef.h>

/// Remove prefix.
/// @return True if string @c prefix consumed from @c s1.
int str_consume(char **s1, const char *prefix);

/// Extract, and advance to, prefix.
/// @return Copy of string from @c s1 to @c pivot.
/// @note Memory ownership: Caller must free() the returned pointer.
char *str_take_until_pivot(char **s1, char *pivot);

/// Extract, and advance to, prefix.
/// @return Copy of string from @c s1 to @c c.
/// @note Memory ownership: Caller must free() the returned pointer.
char *str_take_until(char **s1, int c);

/// Extract, and truncate.
/// @return Copy of string from first occurrence of character @c until end-of-string.
/// @note Memory ownership: Caller must free() the returned pointer.
char *str_take_from(char *s, int c, size_t skip);

/// @return Length of @c prefix if @c str begins with it, 0 otherwise.
size_t str_begins(const char *str, const char *prefix);

/// @return Length of @c other if @c str is equal to it, 0 otherwise.
size_t str_equal(const char *str, const char *other);

/// Append string @c str with length @c len to buffer @c buffer whose capacity is @c cap.
/// @note Capacity includes the NUL terminator.
/// This function will return failure if there is unsufficient space.
/// @return 0 on success.
/// @return -1 on failure, and errno is set to:
///   - ENOSPC: Insufficient space in buffer.
/// @note Memory ownership: Caller retains ownership of the @c buffer parameter.
/// @note Memory ownership: Caller retains ownership of the @c str parameter.
int str_append(char *buffer, size_t cap, const char *str, size_t len);
