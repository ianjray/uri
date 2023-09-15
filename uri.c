#include "uri.h"

#include "percent.h"
#include "rfc_charset.h"
#include "str.h"
#include "text_validate.h"

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/// Port component is parsed as a number 0..65535.
#define PORT_MAX 65535

/// Defer path resolving for relative reference.
/// @see uri_set.
#define FLAG_DEFER_PATH_RESOLVE 0x01u

/// Models a URI as described by RFC 3986.
struct uri {
    /// Flags used during construction.
    unsigned flags;
    /// Scheme.
    char *scheme;
    /// Authority.
    struct {
        char *userinfo;
        char *host;
        char *port;
    } authority;
    /// Path.
    char *path;
    /// Query.
    char *query;
    /// Fragment.
    char *fragment;
};

/// @return True if URI @c u has an authority.
static bool has_authority(const struct uri *u)
{
    return u->authority.userinfo || u->authority.host || u->authority.port;
}

/// The components of a URI.
enum component {
    COMPONENT_SCHEME,
    COMPONENT_USERINFO,
    COMPONENT_HOST,
    COMPONENT_PORT,
    COMPONENT_PATH,
    COMPONENT_QUERY,
    COMPONENT_FRAGMENT
};

/// @return Validation function pointer for @c component.
static bool (*component_predicate_get(enum component component))(unsigned char)
{
    switch (component) {
        default: // UNREACHABLE
            assert(component >= COMPONENT_SCHEME && component <= COMPONENT_FRAGMENT); // UNREACHABLE
            /* flow through */
        case COMPONENT_SCHEME:
            return is_scheme;
        case COMPONENT_USERINFO:
            return is_userinfo;
        case COMPONENT_HOST:
            return is_host;
        case COMPONENT_PORT:
            return is_port;
        case COMPONENT_PATH:
            return is_path;
        case COMPONENT_QUERY:
            return is_query;
        case COMPONENT_FRAGMENT:
            return is_fragment;
    }
}

/// @return True if @c str is comprised entirely of valid characters.
static bool component_character_set_validate(enum component component, const char *str)
{
    bool (*predicate)(unsigned char) = component_predicate_get(component);

    if (predicate == is_scheme) {
        // Explicitly reject the empty scheme.
        if (!*str) {
            return false;
        }

        if (!is_alpha((unsigned char)*str)) {
            return false;
        }
    }

    for (; *str; ++str) {
        if (!predicate((unsigned char)*str)) {
            return false;
        }
    }

    return true;
}

/// @return Pointer-to field pointer for @c component.
static char *const *component_reference_get_const(const struct uri *u, enum component component)
{
    switch (component) {
        default: // UNREACHABLE
            assert(component >= COMPONENT_SCHEME && component <= COMPONENT_FRAGMENT); // UNREACHABLE
            /* flow through */
        case COMPONENT_SCHEME:
            return &u->scheme;
        case COMPONENT_USERINFO:
            return &u->authority.userinfo;
        case COMPONENT_HOST:
            return &u->authority.host;
        case COMPONENT_PORT:
            return &u->authority.port;
        case COMPONENT_PATH:
            return &u->path;
        case COMPONENT_QUERY:
            return &u->query;
        case COMPONENT_FRAGMENT:
            return &u->fragment;
    }
}

/// Non-const variant of @c component_reference_get_const.
/// @see component_reference_get_const.
static char **component_reference_get(struct uri *u, enum component component)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"

    // Const cast away is safe because @c u is non-const.
    return (char **)component_reference_get_const(u, component);

#pragma GCC diagnostic pop
}

/// Get component value.
/// @return Pointer to string.
/// @return NULL on failure.
/// @note Memory ownership: Reference is valid until URI is modified or deleted.
static const char *component_get(const struct uri *u, enum component component)
{
    if (!u) {
        return NULL;
    }

    char * const *field = component_reference_get_const(u, component);
    return *field;
}

/// Remove the special "." and ".." complete path segments from a referenced path.
/// This implementation follows RFC 3986 §5.2.4, however the absolute-vs-relative nature of the input path is preserved.
/// Meaning that a leading '/' is only retained if the input path was absolute.
/// @return 0 on success
/// @return -1 on failure, and errno is set to:
///   - ENOMEM: Insufficient memory.
///   - EINVAL: Invalid relative path when URI has authority but no existing path.
static int remove_dot_segments(struct uri *u, char *input)
{
    size_t output_capacity = strlen(input) + 1 /*NUL*/;
    char *output = malloc(output_capacity);
    if (!output) {
        return -1;
    }

    // Initially empty.
    *output = '\0';

    bool is_absolute = *input == '/';

    while (*input) {
        size_t len = str_begins(input, "../");
        if (len == 0) {
            len = str_begins(input, "./");
        }
        if (len) {
            // A. If the input buffer begins with a prefix of "../" or "./", then remove that prefix from the input buffer; otherwise,
            input += len;
            continue;
        }

        len = str_begins(input, "/./");
        if (len) {
            // B. if the input buffer begins with a prefix of "/./" or "/.", where "." is a complete path segment, then replace that prefix with "/" in the input buffer; otherwise,
            input += 2;
            continue;
        } else {
            len = str_equal(input, "/.");
            if (len) {
                input++;
                *input = '/';
                continue;
            }
        }

        len = str_begins(input, "/../");
        if (len) {
            // C. if the input buffer begins with a prefix of "/../" or "/..", where ".." is a complete path segment, then replace that prefix with "/" in the input buffer;
            //
            // Note: Putting "/" into the input buffer ensures that removing .. must not “escape” above the root.
            input += 3;
        } else {
            len = str_equal(input, "/..");
            if (len) {
                input += 2;
                *input = '/';
            }
        }

        if (len) {
            // and remove the last segment and its preceding "/" (if any) from the output buffer; otherwise,
            char *last = strrchr(output, '/');
            if (!last) {
                last = output;
            }
            *last = '\0';
            continue;
        }

        // D. if the input buffer consists only of "." or "..", then remove that from the input buffer; otherwise,
        len = str_equal(input, "..");
        if (len == 0) {
            len = str_equal(input, ".");
        }
        if (len) {
            input += len;
            continue;
        }

        // E. move the first path segment in the input buffer to the end of the output buffer,
        // including the initial "/" character (if any) and any subsequent characters up to, but not including, the next "/" character or the end of the input buffer.
        char *next = strchr(input + 1, '/');
        if (!next) {
            next = strchr(input, '\0');
        }

        size_t next_len = (size_t)(next - input);
        str_append(output, output_capacity, input, next_len);
        input = next;
    }

    // Only allow a leading / in the final output if the input path was absolute.
    if (!is_absolute && *output == '/') {
        memmove(output, output + 1, strlen(output + 1) + 1 /*NUL*/);
    }

    if (has_authority(u)) {
        // If Authority is present, path must be absolute or empty.
        if (!(*output == '\0' || *output == '/')) {
            free(output);
            errno = EINVAL;
            return -1;
        }
    }

    free(u->path);
    u->path = output;
    return 0;
}

static int path_set(struct uri *u, char *str)
{
    char *tmp = NULL;

    if (*str == '/') {
        // Absolute.
        tmp = str;

    } else {
        // Relative.
        //
        // Merge paths prior to dot segment removal.
        // /a/b + ../../c = /a/../../c
        size_t base_len = 0;

        if (u->path) {
            char *separator = strrchr(u->path, '/');
            if (separator) {
                separator++;
                base_len = (size_t)(separator - u->path);
            }
        }

        tmp = malloc(base_len + strlen(str) + 1 /*NUL*/);
        if (!tmp) {
            return -1;
        }

        if (base_len > 0) {
            memcpy(tmp, u->path, base_len);
        }

        strcpy(tmp + base_len, str);
    }

    int r = remove_dot_segments(u, tmp);
    if (tmp != str) {
        free(tmp);
    }
    return r;
}

/// Parse port and normalize in-place.
/// @return Formatted port number on success.
/// @return NULL on failure, and errno is set to:
///   - ERANGE: Port is not in the range 0-65535.
///   - ENOMEM: Insufficient memory.
/// @note Memory ownership: Caller must free() the returned pointer.
static char *parse_port(char *str)
{
    // §3.2.3
    // Optional port number in decimal.
    //
    // Note: This library enforces range 0..65535 and drops leading zeroes.
    errno = 0;

    unsigned long port = strtoul(str, NULL, 10);
    if (errno == ERANGE) {
        // Too long.
        return NULL;
    }

    if (port > PORT_MAX) {
        // Too large.
        errno = ERANGE;
        return NULL;
    }

    // Drop leading zeroes.
    char *start = str;
    while (start[0] == '0' && start[1] != '\0') {
        start++;
    }

    memmove(str, start, strlen(start) + 1/*NUL*/);
    return str;
}

/// Set component value.
/// The string shall comply to characters allowed by the RFC.
///
/// For scheme and host:
///  * Value is converted to lowercase (except percent-encoded values which are uppercase).
///
/// For port:
///  * Reject non-digits.
///  * Ignore leading zeroes.
///  * Require port number in the range 0..65535.
///  * @note This is intentionally stricter than RFC 3986.
///
/// For setting authority:
///  * Reject if path is relative.
///
/// @return 0 on success, -1 otherwise (with errno set).
static int component_set_common(struct uri *u, enum component component, char *value)
{
    // Reject invalid characters.
    if (!component_character_set_validate(component, value)) {
        errno = EINVAL;
        return -1;
    }

    // §2.3
    // Validate and normalize percent-encoded characters.
    //
    // Percent encoded C0 characters are rejected (since they can lead to injection attacks).
    // (C0 includes U+0000 which is rejected because it would truncate the input.)
    // Note: This is intentionally stricter than RFC 3986.
    if (component_predicate_get(component)('%')) {
        if (percent_encoded_string_normalize(value) < 0) {
            return -1;
        }
    }

    // §3.1
    // An implementation should accept uppercase letters as equivalent to lowercase ... for the sake of robustness but should only produce lowercase scheme names for consistency.
    //
    // §3.2.2
    // Although host is case-insensitive, producers and normalizers should use lowercase for registered names and hexadecimal addresses for the sake of uniformity.
    if (component == COMPONENT_SCHEME || component == COMPONENT_HOST) {
        percent_aware_lowercase(value);
    }

    // §3.3.
    // When setting Authority, path must be absolute or empty.
    if (component == COMPONENT_USERINFO || component == COMPONENT_HOST || component == COMPONENT_PORT) {
        if (u->path && *u->path && *u->path != '/') {
            errno = EINVAL;
            return -1;
        }
    }

    // §3.2.3
    // Optional port number in decimal.
    // Note: This library enforces range 0..65535 and drops leading zeroes.
    if (component == COMPONENT_PORT) {
        if (*value) {
            value = parse_port(value);
            if (!value) {
                return -1;
            }
        }
    }

    // §3.3.
    // §5.2.
    // Set the path and perform relative resolution.
    if (component == COMPONENT_PATH) {
        // uri_set constructs a URI object from a (possibly) relative reference.
        // Defer path resolve during construction (until uri_set is ready).
        if ((u->flags & FLAG_DEFER_PATH_RESOLVE) == 0) {
            return path_set(u, value);
        }
    }

    // Update component in URI object.
    char **field = component_reference_get(u, component);
    free(*field);
    *field = value;
    return 0;
}

/// Validate input string.
/// The string must be valid UTF-8.
/// The string must not contain C0 and C1 code points.
/// Any UTF-8 characters in the string are automatically percent-encoded to ensure RFC compliance.
/// @return Copy of string on success.
/// @note Memory ownership: Caller must free() the returned pointer.
static char *input_string_validate(const char *utf8_str)
{
    if (!utf8_str) {
        errno = EFAULT;
        return NULL;
    }

    // Reject invalid UTF-8.
    if (!utf8_validate(utf8_str)) {
        errno = EILSEQ;
        return NULL;
    }

    // Reject UTF-8 encoded control characters.
    // Note: Percent-encoded ASCII C0 controls are handled in component_set_common.
    if (contains_c0_c1_controls_utf8(utf8_str)) {
        errno = EINVAL;
        return NULL;
    }

    // Automatically percent-encode non-ASCII characters on input, for nicer ergonomics.
    return percent_encode_non_ascii_characters(utf8_str);
}

/// Set component value.
/// @see component_set_common.
/// @return 0 on success, -1 otherwise (with errno set).
static int component_set(struct uri *u, enum component component, const char *utf8_str)
{
    if (!u) {
        errno = EFAULT;
        return -1;
    }

    char **field = component_reference_get(u, component);

    if (!utf8_str) {
        // Remove component from URI object.
        free(*field);
        *field = NULL;
        return 0;
    }

    char *copy = input_string_validate(utf8_str);
    if (!copy) {
        return -1;
    }

    int r = component_set_common(u, component, copy);
    if (*field != copy) {
        free(copy);
    }
    return r;
}

static size_t accept_scheme(const char *scheme)
{
    const char *str = scheme;

    if (is_alpha((unsigned char)*str)) {
        while (is_scheme((unsigned char)*str)) {
            str++;
        }

        if (*str == ':') {
            return (size_t)(str - scheme);
        }
    }

    return 0;
}

/// Tokenise the input string into URI components following the rules of RFC 3986.
/// @return 0 on success, -1 otherwise (with errno set).
static int tokenise(char *str, struct uri *u)
{
    // Clear, so that memory allocation errors can be detected.
    errno = 0;

    // [scheme ":"] "//" [userinfo "@"] [host] [":" port] ["/" [path]] ["?" query] ["#" fragment]
    // [scheme ":"]                                            [path]  ["?" query] ["#" fragment]

    // §3.5
    // A fragment identifier component is indicated by the presence of a number sign ("#") character and terminated by the end of the URI.
    u->fragment = str_take_from(str, '#', 1);

    // §3.4
    // The query component is indicated by the first question mark ("?") character and terminated by a number sign ("#") character or by the end of the URI.
    u->query = str_take_from(str, '?', 1);

    // §4.1
    // URI-reference = URI / relative-ref
    //
    // §3.1
    // URI begins with a scheme name that refers to a specification for assigning identifiers within that scheme.
    //
    // Note: A string like "file:..." looks like a scheme and must be prefixed with "./" to treat as a relative reference.
    size_t len = accept_scheme(str);
    if (len) {
        u->scheme = str_take_until_pivot(&str, str + len);
        str++;
    }

    if (!u->scheme) {
        // §4.2
        // relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
        // relative-part = "//" authority path-abempty
        //               / path-absolute
        //               / path-noscheme
        //               / path-empty
        for (char *iter = str; *iter && *iter != '/'; ++iter) {
            if (*iter == ':') {
                // §3.3
                // A relative-path reference may not contain a colon (":") character in the first segment.
                // (This is due to potential ambiguity with "scheme:".)
                errno = EINVAL;
                return -1;
            }
        }
    }

    // §3.2
    // The authority component is preceded by a double slash ("//") and is terminated by the next slash ("/"), question mark ("?"), or number sign ("#") character, or by the end of the URI.
    if (str_consume(&str, "//")) {

        // [userinfo "@"] [host] [":" port] ["/" [path]]

        // §3.3
        // If a URI contains an authority component, then the path component must either be empty or begin with a slash ("/") character.
        // Note: Authority may be empty.
        u->path = str_take_from(str, '/', 0);

        // §3.2.1
        // The user information, if present, is followed by a commercial at-sign ("@") that delimits it from the host.
        u->authority.userinfo = str_take_until(&str, '@');
        str_consume(&str, "@");

        // §3.2.2
        // host        = IP-literal / IPv4address / reg-name
        // IP-literal  = "[" ( IPv6address / IPvFuture  ) "]"
        if (*str == '[') {
            char *close = strchr(str, ']');
            if (!close) {
                // Unbalanced.
                errno = EINVAL;
                return -1;
            }

            u->authority.host = str_take_until_pivot(&str, close + 1);

        } else {
            if ((u->authority.host = str_take_until(&str, ':')) == NULL) {
                u->authority.host = strdup(str);
            }
        }

        // §3.2.3
        // The port subcomponent of authority is designated by an optional port number in decimal following the host and delimited from it by a single colon (":") character.
        if (str_consume(&str, ":")) {
            u->authority.port = strdup(str);
        }

    } else if (str && *str) {
        u->path = strdup(str);
    }

    if (errno) {
        // Memory allocation failed.
        return -1;
    }

    return 0;
}

/// Construct a new URI object.
/// @see uri_new
static struct uri *construct(const char *utf8_str, unsigned flags)
{
    char *copy = input_string_validate(utf8_str);
    if (!copy) {
        return NULL;
    }

    struct uri *u = (struct uri *)calloc(1, sizeof(*u));
    if (u) {
        u->flags = flags;

        int r = tokenise(copy, u);
        if (r == 0) {
            for (enum component component = COMPONENT_SCHEME; component <= COMPONENT_FRAGMENT; ++component) {
                char **field = component_reference_get(u, component);
                char *input = *field;
                if (input) {
                    // Tokenise assigns to components directly for convenience.
                    // The pointer is moved from field to input for setting purposes.
                    *field = NULL;

                    r = component_set_common(u, component, input);
                    if (*field != input) {
                        free(input);
                    }

                    if (r < 0) {
                        break;
                    }
                }
            }
        }

        if (r < 0) {
            uri_delete(u);
            u = NULL;
        }
    }

    free(copy);
    return u;
}

struct uri *uri_new(const char *utf8_str)
{
    return construct(utf8_str, 0 /* No flags. */);
}

int uri_set(struct uri *u, const char *utf8_str)
{
    if (!u) {
        errno = EFAULT;
        return -1;
    }

    // Construct a URI object to hold the input string.
    struct uri *ingest = construct(utf8_str, FLAG_DEFER_PATH_RESOLVE);
    if (!ingest) {
        return -1;
    }

    u->flags &= ~FLAG_DEFER_PATH_RESOLVE;

#define SWAP(src, dest, field) do { \
    char *tmp = (src)->field; \
    (src)->field = (dest)->field; \
    (dest)->field = tmp; \
    } while (0)

    int r = 0;

    // §5.2.2
    // Apply changes from the input string to object, according to RFC algorithm.

    if (ingest->scheme) {
        SWAP(u, ingest, scheme);
        SWAP(u, ingest, authority.userinfo);
        SWAP(u, ingest, authority.host);
        SWAP(u, ingest, authority.port);
        SWAP(u, ingest, path);
        SWAP(u, ingest, query);
        SWAP(u, ingest, fragment);

    } else {
        if (has_authority(ingest)) {
            SWAP(u, ingest, authority.userinfo);
            SWAP(u, ingest, authority.host);
            SWAP(u, ingest, authority.port);
            SWAP(u, ingest, path);
            SWAP(u, ingest, query);
            SWAP(u, ingest, fragment);

        } else {
            if (!ingest->path || !*ingest->path) {
                if (ingest->query) {
                    SWAP(u, ingest, query);
                }
                SWAP(u, ingest, fragment);

            } else {
                // Resolve possible relative path.
                r = uri_path_set(u, ingest->path);
                if (r == 0) {
                    SWAP(u, ingest, query);
                    SWAP(u, ingest, fragment);
                }
            }
        }
    }

#undef SWAP

    uri_delete(ingest);
    return r;
}

void uri_delete(struct uri *u)
{
    if (!u) {
        return;
    }

    free(u->scheme);
    free(u->authority.userinfo);
    free(u->authority.host);
    free(u->authority.port);
    free(u->path);
    free(u->query);
    free(u->fragment);
    free(u);
}

const char *uri_scheme(const struct uri *u)
{
    return component_get(u, COMPONENT_SCHEME);
}

const char *uri_userinfo(const struct uri *u)
{
    return component_get(u, COMPONENT_USERINFO);
}

const char *uri_host(const struct uri *u)
{
    return component_get(u, COMPONENT_HOST);
}

const char *uri_port(const struct uri *u)
{
    return component_get(u, COMPONENT_PORT);
}

const char *uri_path(const struct uri *u)
{
    return component_get(u, COMPONENT_PATH);
}

const char *uri_query(const struct uri *u)
{
    return component_get(u, COMPONENT_QUERY);
}

const char *uri_fragment(const struct uri *u)
{
    return component_get(u, COMPONENT_FRAGMENT);
}

int uri_scheme_set(struct uri *u, const char *rfc_str)
{
    return component_set(u, COMPONENT_SCHEME, rfc_str);
}

int uri_userinfo_set(struct uri *u, const char *utf8_str)
{
    return component_set(u, COMPONENT_USERINFO, utf8_str);
}

int uri_host_set(struct uri *u, const char *ascii_str)
{
    return component_set(u, COMPONENT_HOST, ascii_str);
}

int uri_port_set(struct uri *u, const char *digit_str)
{
    return component_set(u, COMPONENT_PORT, digit_str);
}

int uri_path_set(struct uri *u, const char *utf8_str)
{
    return component_set(u, COMPONENT_PATH, utf8_str);
}

int uri_query_set(struct uri *u, const char *utf8_str)
{
    return component_set(u, COMPONENT_QUERY, utf8_str);
}

int uri_fragment_set(struct uri *u, const char *utf8_str)
{
    return component_set(u, COMPONENT_FRAGMENT, utf8_str);
}

/// Two-pass string assembler.
/// Calculate size in first pass, then allocate memory and populate string in second pass.
struct assemble {
    int pass;
    size_t len;
    char *str;
};

static void assemble_string(struct assemble *a, const char *s)
{
    if (a->str == NULL) {
        a->len += strlen(s);

    } else {
        strcat(a->str, s);
    }
}

static void assemble_field(struct assemble *a, const struct uri *u, enum component component)
{
    char * const *field = component_reference_get_const(u, component);
    assemble_string(a, *field);
}

char *uri_str(const struct uri *u)
{
    if (!u) {
        errno = EFAULT;
        return NULL;
    }

    struct assemble a;
    a.pass = 0;
    a.len = 0;
    a.str = NULL;

    for (a.pass = 0; a.pass < 2; ++a.pass) {
        if (u->scheme) {
            assemble_field(&a, u, COMPONENT_SCHEME);
            assemble_string(&a, ":");
        }

        if (has_authority(u)) {
            assemble_string(&a, "//");

            if (u->authority.userinfo) {
                assemble_field(&a, u, COMPONENT_USERINFO);
                assemble_string(&a, "@");
            }

            if (u->authority.host) {
                assemble_field(&a, u, COMPONENT_HOST);
            }

            if (u->authority.port) {
                assemble_string(&a, ":");
                assemble_field(&a, u, COMPONENT_PORT);
            }
        }

        // https://url.spec.whatwg.org/#url-serializing
        if (u->path) {
            if (!has_authority(u)) {
                if (!strncmp(u->path, "//", 2)) {
                    // Path looks like an authority.
                    assemble_string(&a, "/.");

                } else if (!u->scheme) {
                    if (accept_scheme(u->path)) {
                        // Path looks like a scheme.
                        assemble_string(&a, "./");
                    }
                }
            }

            assemble_field(&a, u, COMPONENT_PATH);
        }

        if (u->query) {
            assemble_string(&a, "?");
            assemble_field(&a, u, COMPONENT_QUERY);
        }

        if (u->fragment) {
            assemble_string(&a, "#");
            assemble_field(&a, u, COMPONENT_FRAGMENT);
        }

        if (a.pass == 0) {
            a.str = malloc(a.len + 1 /* NUL */);
            if (!a.str) {
                break;
            }

            *a.str = '\0';
        }
    }

    return a.str;
}
