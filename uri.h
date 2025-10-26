#pragma once

#define PUBLIC __attribute__ ((visibility("default")))

struct uri;

/// Create URI object from given string.
/// @see RFC 3986, §Appendix B.
/// @discussion
/// The characters in the string are validated against the RFC 3986 character set.
/// Characters outside the allowed set (or that would be interpreted as a delimiter) must be percent-encoded.
/// (This also means that a raw pecent symbol must be percent-encoded.)
/// The string is normalized according to RFC 3986.
/// @note
/// The scheme has no particular significance.
/// The host is opaque.
/// The path is simplified to remove dot segments.
/// Query parameters are not sorted.
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -EINVAL invalid character or percent-encoding.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller must uri_delete() the returned pointer.
int uri_new(const char *, struct uri **) PUBLIC;

/// Duplicate URI object.
/// @return -EFAULT bad pointer.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller must uri_delete() the returned pointer.
int uri_dup(const struct uri *, struct uri **) PUBLIC;

/// Modify URI object with given string.
/// @discussion
/// The string is either an absolute or relative URI.
/// Validation is performed as described in @c uri_new.
/// @see RFC 3986, §4.2.
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -EINVAL invalid character or percent-encoding.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_set(struct uri *, const char *) PUBLIC;

/// Component interface.
///
/// Components are extracted from URIs of the following form:
/// ```
/// [scheme ":"] "//" [userinfo "@"] [host] [":" port] ["/" [path]] ["?" query] ["#" fragment]
/// [scheme ":"]                                            [path]  ["?" query] ["#" fragment]
/// ```
/// @see RFC 3986 Appendix B.
///
/// Components are NULL if not present in the URI, and the empty string if present but empty.
///
/// Components always are "natural" sequences of characters with no special reserved characters.
///
/// @warning
/// The caller *must* parse components for correctness before using them.
/// (This module checks character set only.)
///
/// @see uri_scheme
/// @see uri_userinfo
/// @see uri_host
/// @see uri_port
/// @see uri_path
/// @see uri_query
/// @see uri_fragment

/// Get scheme.
/// @return Reference to component (or NULL if not present in URI).
/// @note Memory ownership: Reference is valid until URI is modified or deleted.
const char *uri_scheme(const struct uri *) PUBLIC;

/// Get userinfo.
/// @return Reference to component (or NULL if not present in URI).
/// @note Memory ownership: Reference is valid until URI is modified or deleted.
const char *uri_userinfo(const struct uri *) PUBLIC;

/// Get host.
/// @return Reference to component (or NULL if not present in URI).
/// @note Memory ownership: Reference is valid until URI is modified or deleted.
const char *uri_host(const struct uri *) PUBLIC;

/// Get port.
/// @return Reference to component (or NULL if not present in URI).
/// @note Memory ownership: Reference is valid until URI is modified or deleted.
const char *uri_port(const struct uri *) PUBLIC;

/// Get path.
/// @return Reference to component (or NULL if not present in URI).
/// @note Memory ownership: Reference is valid until URI is modified or deleted.
const char *uri_path(const struct uri *) PUBLIC;

/// Get query.
/// @return Reference to component (or NULL if not present in URI).
/// @note Memory ownership: Reference is valid until URI is modified or deleted.
const char *uri_query(const struct uri *) PUBLIC;

/// Get fragment.
/// @return Reference to component (or NULL if not present in URI).
/// @note Memory ownership: Reference is valid until URI is modified or deleted.
const char *uri_fragment(const struct uri *) PUBLIC;

/// Set scheme component.
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -EINVAL invalid character.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_scheme_set(struct uri *, const char *) PUBLIC;

/// Set userinfo component.
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_userinfo_set(struct uri *, const char *) PUBLIC;

/// Set host component.
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_host_set(struct uri *, const char *) PUBLIC;

/// Set port component.
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -EINVAL invalid character.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_port_set(struct uri *, const char *) PUBLIC;

/// Set path component.
/// @discussion
/// This function either replaces existing path with a new absolute path, or modifies existing path with a relative path.
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -EINVAL attempting to set relative path to URI with authority but no path.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_path_set(struct uri *, const char *) PUBLIC;

/// Set query component.
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_query_set(struct uri *, const char *) PUBLIC;

/// Set fragment component.
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_fragment_set(struct uri *, const char *) PUBLIC;

/// Get sequence of characters as a string.
/// The resultant string conforms to RFC 3986, including the use of percent-encoding for reserved characters.
/// @note
/// Care is taken to maintain semantics and disambiguate problematic paths when there is no scheme or authority.
/// @see https://url.spec.whatwg.org/#url-serializing
/// @return Zero on success, negative errno otherwise.
/// @return -EFAULT bad pointer.
/// @return -ENOMEM out of memory.
/// @note Memory ownership: Caller must free() the returned pointer.
int uri_str(const struct uri *, char **) PUBLIC;

/// Destructor.
/// @note Memory ownership: Takes ownership of the given pointer.
void uri_delete(struct uri *) PUBLIC;
