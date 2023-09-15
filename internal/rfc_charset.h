#pragma once

// Private API.
// UTF-8 + control character validation.
// "Is this allowed by the RFC grammar?"

#include <stdbool.h>

/// @return True if @c is alphabetic 'a-z' or 'A-Z'.
bool is_alpha(unsigned char c);

/// @return True if @c is a reserved URI character, per RFC 3986.
bool is_unreserved(unsigned char c);

/// @return True if @c is a RFC 3986 scheme character.
bool is_scheme(unsigned char c);

/// @return True if @c is a RFC 3986 port character.
bool is_port(unsigned char c);

/// @return True if @c is a RFC 3986 userinfo character.
bool is_userinfo(unsigned char c);

/// @return True if @c is a RFC 3986 host character.
/// @note The tokenisation is quite relaxed.
/// Users must parse for correctness.
bool is_host(unsigned char c);

/// @return True if @c is a RFC 3986 path character.
bool is_path(unsigned char c);

/// @return True if @c is a RFC 3986 query character.
bool is_query(unsigned char c);

/// @return True if @c is a RFC 3986 fragment character.
bool is_fragment(unsigned char c);
