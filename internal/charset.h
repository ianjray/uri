#pragma once

// Private API.

#include <stdbool.h>

/// @return True if @c is alphabetic 'a-z' or 'A-Z'.
bool is_alpha(int c);

/// @return True if @c is a reserved URI character, per RFC 3986.
bool is_unreserved(int c);

/// @return True if @c is a RFC 3986 scheme character.
bool is_scheme(int c);

/// @return True if @c is a RFC 3986 port character.
bool is_port(int c);

/// @return True if @c is a RFC 3986 userinfo character.
bool is_userinfo(int c);

/// @return True if @c is a RFC 3986 host character.
bool is_host(int c);

/// @return True if @c is a RFC 3986 path character.
bool is_path(int c);

/// @return True if @c is a RFC 3986 query or fragment character.
bool is_query(int c);
