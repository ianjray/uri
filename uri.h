#pragma once

/// URIs have the following structure.
/// ```
/// [scheme ":"] "//" [userinfo "@"] [host] [":" port] ["/" [path]] ["?" query] ["#" fragment]
/// [scheme ":"]                                            [path]  ["?" query] ["#" fragment]
/// ```
/// @see RFC 3986 Appendix B.
///
/// @note This library intentionally rejects many URIs accepted by browsers.
///
/// All input strings must be valid UTF-8 formatted according to the RFC.
/// As a convenience, UTF-8 characters are automatically percent-encoded by @c uri_new and @c uri_set.
/// Characters outside the allowed set (or that would be interpreted as a delimiter) must be percent-encoded by the caller.
/// Unicode C1 code points (U+0080–U+009F) are rejected on input.
///
/// All output strings are guaranteed to be ASCII and guaranteed not to contain ASCII control characters (U+0000–U+001F, U+007F) either literally or via percent-encoding.
/// Any inputs that would result in such characters are always rejected.
///
/// Callers embedding components into text-based protocols (HTTP, SMTP, logs) must always apply context-appropriate escaping.

/// Functions in the API are grouped by three return type conventions:
///
/// 1. Functions that *produce or return an object pointer* return that pointer on success, or NULL on failure with errno set to indicate the error.
///    For example uri_new, uri_dup.
///
/// 2. Functions that *perform an operation* return zero on success, or -1 on failure with errno set to indicate the failure.
///    For example uri_scheme_set.
///
/// 3. Functions that *return simple values* return the value directly, and silently accept invalid arguments.
///    For example uri_scheme.
///
/// When @c errno is set, then it is used consistently as follows:
/// - EFAULT: NULL pointer argument.
/// - EILSEQ: Invalid UTF-8 encoding.
/// - EINVAL: Syntactically or semantically invalid URI data (such as disallowed characters or invalid percent-encoding).
/// - ENOMEM: Insufficient memory.
/// - ERANGE: Out of range URI data (such as port outside range 0-65535).

#define PUBLIC __attribute__ ((visibility("default")))

struct uri;

/// Constructor.
/// Create a new URI object from the @c utf8_str.
/// The string is normalized according to RFC 3986.
/// @see RFC 3986, §Appendix B.
/// @note The scheme has no particular significance.
///       The host is opaque.
///       The path is simplified to remove dot segments.
///       Query parameters are not sorted.
/// @return Pointer to object on success.
/// @return NULL on failure, and errno is set.
/// @note Memory ownership: Caller must uri_delete() the returned pointer.
struct uri *uri_new(const char *utf8_str) PUBLIC;

/// Modify URI.
/// Interprets @c utf8_ref as a URI reference and resolves it against the base URI stored in @c u.
/// If @c utf8_ref is an absolute URI, it replaces the contents of @c u.
/// Otherwise, the components of @c utf8_ref are merged with those of @c u according to the rules of RFC 3986 §5.2, including path merging and dot-segment removal.
/// @return 0 on success.
/// @return -1 on failure, and errno is set.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_set(struct uri *u, const char *utf8_ref) PUBLIC;

/// Destructor.
/// @note Memory ownership: Takes ownership of the given pointer.
void uri_delete(struct uri *) PUBLIC;

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
/// @note Applications that apply port-based security policies MUST treat an explicitly empty port as a distinct case and must not assume a default port.
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

/// Set scheme.
/// Requires valid ASCII that conforms to RFC.
/// @return 0 on success.
/// @return -1 on failure, and errno is set.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_scheme_set(struct uri *, const char *rfc_str) PUBLIC;

/// Set userinfo.
/// Requires valid UTF-8 that conforms to RFC.
/// @return 0 on success.
/// @return -1 on failure, and errno is set.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_userinfo_set(struct uri *, const char *utf8_str) PUBLIC;

/// Set host.
/// Requires an ASCII string; UTF-8 is not permitted in the host component.
/// Percent-encoding is permitted but UTF-8 is not.
/// @note This function fails if the string contains control characters.
/// @return 0 on success.
/// @return -1 on failure, and errno is set.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_host_set(struct uri *, const char *ascii_str) PUBLIC;

/// Set port.
/// Requires zero, one, or more ASCII digits.
/// @note Leading zeroes are silently dropped.
/// @return 0 on success.
/// @return -1 on failure, and errno is set.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_port_set(struct uri *, const char *rfc_str) PUBLIC;

/// Set path.
/// Requires valid UTF-8 that conforms to RFC.
/// @return 0 on success.
/// @return -1 on failure, and errno is set.
/// @note EINVAL is returned if attempting to set a relative path when the URI has authority but no existing path [RFC §3.3].
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_path_set(struct uri *, const char *utf8_str) PUBLIC;

/// Set query.
/// Requires valid UTF-8 that conforms to RFC.
/// @return 0 on success.
/// @return -1 on failure, and errno is set.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_query_set(struct uri *, const char *utf8_str) PUBLIC;

/// Set fragment.
/// Requires valid UTF-8 that conforms to RFC.
/// @return 0 on success.
/// @return -1 on failure, and errno is set.
/// @note Memory ownership: Caller retains ownership of the string parameter.
int uri_fragment_set(struct uri *, const char *utf8_str) PUBLIC;

/// Get URI string.
/// The returned string is ASCII-only and suitable for use on the wire.
/// @note Care is taken to maintain semantics and disambiguate problematic paths when there is no scheme or authority.
/// @see https://url.spec.whatwg.org/#url-serializing
/// @return Pointer on success.
/// @return -1 on failure, and errno is set.
/// @note Memory ownership: Caller must free() the returned pointer.
char *uri_str(const struct uri *) PUBLIC;
