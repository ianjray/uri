#include "uri.h"

#include "internal/charset.h"
#include "internal/percent.h"
#include "internal/str.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct uri {
    char *scheme;
    struct {
        bool has;
        char *userinfo;
        char *host;
        char *port;
    } authority;
    char *unresolved_path;
    char *path;
    char *query;
    char *fragment;
};

/// @return True if @c str meets the prediate.
static bool validate(const char *str, bool (*predicate)(int))
{
    if (str) {
        for (; *str; ++str) {
            if (!predicate(*str)) {
                return false;
            }
        }
    }

    return true;
}

/// @return Zero if scheme valid, negative errno otherwise.
static int validate_scheme(const char *scheme)
{
    if (scheme && !is_alpha(*scheme)) {
        return -EINVAL;
    }

    if (!validate(scheme, is_scheme)) {
        return -EINVAL;
    }

    return 0;
}

static void resolve_path(struct uri *u)
{
    if (u->unresolved_path) {
        char *in = strdup(u->unresolved_path);
        size_t n = 0;
        char *out = strdup("");
        const char *marker = "\x1E"; // ASCII RS record separator

        // §5.2.4
        //
        // Custom algorithm to work around issues with the RFC algorithm, including this strange behaviour:
        // `a/..`    -> `/`
        // `a/../b`  -> `/`
        // `a//../b` -> `a/b`

        while (!errno && out && *in) {
            if (str_delete(in, ".") || str_prefix_delete(in, "./")) {

            } else if (str_delete(in, "..") || str_prefix_delete(in, "../")) {
                if (n == 1 && out[1] == '/') {
                    // Do not go past root.
                } else if (n) {
                    // Backtrack.
                    n--;
                    *strrchr(out, *marker) = 0;
                }

            } else {
                char *segment;
                n++;
                str_append(&out, marker);
                segment = str_remove_first_path_segment(in);
                str_append(&out, segment);
                free(segment);
            }
        }

        free(in);
        free(u->path);
        u->path = str_filter_out(out, *marker);
    }
}

int uri_new(const char *str_in, struct uri **tokens_out)
{
    struct uri *u;
    char *str;
    char *iter;
    int r;

    if (!str_in || !tokens_out) {
        return -EFAULT;
    }

    u = (struct uri *)calloc(1, sizeof(*u));
    if (!u) {
        return -errno; // UNREACHABLE
    } // UNREACHABLE

    // RFC 3986
    // Appendix A.
    //
    // URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
    //
    // hier-part     = "//" authority path-abempty
    //               / path-absolute
    //               / path-rootless
    //               / path-empty
    //
    // authority     = [ userinfo "@" ] host [ ":" port ]

    errno = 0;
    str = safe_strdup(str_in);

    // §3.5
    // A fragment identifier component is indicated by the presence of a number sign ("#") character and terminated by the end of the URI.

    u->fragment = str_suffix_detach(str, '#');

    // §3.4
    // The query component is indicated by the first question mark ("?") character and terminated by a number sign ("#") character or by the end of the URI.

    u->query = str_suffix_detach(str, '?');

    // §3.1
    // Each URI begins with a scheme name that refers to a specification for assigning identifiers within that scheme.

    if ((iter = str_iterate(str, is_alpha, is_scheme)) && *iter == ':') {
        u->scheme = str_split_left(str, iter);
        str_prefix_delete(str, ":");
    }

    // §3.2
    // The authority component is preceded by a double slash ("//") and is terminated by the next slash ("/"), question mark ("?"), or number sign ("#") character, or by the end of the URI.

    if ((u->authority.has = str_prefix_delete(str, "//"))) {

        // §3.3
        // If a URI contains an authority component, then the path component must either be empty or begin with a slash ("/") character.

        u->unresolved_path = str_suffix_detach_inclusive(str, '/');

        // §3.2.3
        // The port subcomponent of authority is designated by an optional port number in decimal following the host and delimited from it by a single colon (":") character.

        str_reverse_iterate_digits(str, &iter);
        if (str_reverse_iterate_one(str, &iter, ':')) {
            u->authority.port = str_split(iter);
        }

        // §3.2.1
        // The user information, if present, is followed by a commercial at-sign ("@") that delimits it from the host.

        if ((u->authority.host = str_suffix_detach(str, '@'))) {
            u->authority.userinfo = safe_strdup(str);
        } else {
            u->authority.host = safe_strdup(str);
        }

    } else if (str && *str) {
        u->unresolved_path = safe_strdup(str);
    }

    free(str);

    if (!errno) {
        // §3.1
        // An implementation should accept uppercase letters as equivalent to lowercase ... for the sake of robustness but should only produce lowercase scheme names for consistency.

        str_lowercase(u->scheme);

        // §3.2.2
        // Although host is case-insensitive, producers and normalizers should use lowercase for registered names and hexadecimal addresses for the sake of uniformity, while only
        // using uppercase letters for percent-encodings.

        str_lowercase(u->authority.host);

        r = -EINVAL;

        // Require that all components are comprised of allowed characters.

        if (   validate(u->authority.userinfo, is_userinfo)
            && validate(u->authority.host, is_host)
            && validate(u->authority.port, is_port)
            && validate(u->unresolved_path, is_path)
            && validate(u->query, is_query)
            && validate(u->fragment, is_query)) {

            // §2.3
            // For consistency, percent-encoded octets in the ranges of ALPHA (%41-%5A and %61-%7A), DIGIT (%30-%39), hyphen (%2D), period (%2E), underscore (%5F), or tilde (%7E) should
            // not be created by URI producers and, when found in a URI, should be decoded to their corresponding unreserved characters by URI normalizers.
            //
            // Note! It is important to do this before path simplification, so that period (%2E) does not escape scrutiny.

            if (   normalize_percent_encodings(u->authority.userinfo)
                && normalize_percent_encodings(u->authority.host)
                && normalize_percent_encodings(u->unresolved_path)
                && normalize_percent_encodings(u->query)
                && normalize_percent_encodings(u->fragment)) {

                resolve_path(u);

                // Percent decode, so that the resulting strings (returned by the various getter functions) are "natural" sequences of characters with no special reserved characters.

                if (   percent_decode(u->authority.userinfo)
                    && percent_decode(u->authority.host)
                    && percent_decode(u->unresolved_path)
                    && percent_decode(u->path)
                    && percent_decode(u->query)
                    && percent_decode(u->fragment)) {
                    r = 0;
                }
            }
        }

        if (r < 0) {
            uri_delete(u);
            return r;
        }
    }

    *tokens_out = u;
    return -errno;
}

int uri_dup(const struct uri *u, struct uri **dst)
{
    struct uri *copy;

    if (!u || !dst) {
        return -EFAULT;
    }

    errno = 0;

    copy = (struct uri *)calloc(1, sizeof(*u));
    if (copy) {
        copy->scheme             = safe_strdup(u->scheme);
        copy->authority.has      = u->authority.has;
        copy->authority.userinfo = safe_strdup(u->authority.userinfo);
        copy->authority.host     = safe_strdup(u->authority.host);
        copy->authority.port     = safe_strdup(u->authority.port);
        copy->unresolved_path    = safe_strdup(u->unresolved_path);
        copy->path               = safe_strdup(u->path);
        copy->query              = safe_strdup(u->query);
        copy->fragment           = safe_strdup(u->fragment);
    }

    if (errno) {
        free(copy); // UNREACHABLE
        return -errno; // UNREACHABLE
    } // UNREACHABLE

    *dst = copy;
    return 0;
}

int uri_set(struct uri *u, const char *seq)
{
    struct uri *ingest;
    int r;

    if (!u || !seq) {
        return -EFAULT;
    }

    r = uri_new(seq, &ingest);
    if (r < 0) {
        return r;
    }

#define SWAP(src, dest, field) { char *tmp = src->field; src->field = dest->field; dest->field = tmp; }

    if (ingest->scheme) {
        // Absolute.
        SWAP(u, ingest, scheme);
        u->authority.has = ingest->authority.has;
        SWAP(u, ingest, authority.userinfo);
        SWAP(u, ingest, authority.host);
        SWAP(u, ingest, authority.port);
        SWAP(u, ingest, unresolved_path);
        SWAP(u, ingest, path);
        SWAP(u, ingest, query);

    } else {
        // Relative.
        // §5.2.2
        if (ingest->authority.has) {
            u->authority.has = ingest->authority.has;
            SWAP(u, ingest, authority.userinfo);
            SWAP(u, ingest, authority.host);
            SWAP(u, ingest, authority.port);
            SWAP(u, ingest, unresolved_path);
            SWAP(u, ingest, path);
            SWAP(u, ingest, query);

        } else {
            if (!ingest->unresolved_path || !*ingest->unresolved_path) {
                if (ingest->query) {
                    SWAP(u, ingest, query);
                }

            } else {
                if (ingest->unresolved_path[0] == '/') {
                    SWAP(u, ingest, unresolved_path);
                    SWAP(u, ingest, path);

                } else {
                    // Unresolved path used here to allow setting a relative path.
                    uri_path_set(u, ingest->unresolved_path);
                }

                SWAP(u, ingest, query);
            }
        }
    }

    SWAP(u, ingest, fragment);

    uri_delete(ingest);

    return 0;
}

const char *uri_scheme(const struct uri *u)
{
    if (u) {
        return u->scheme;
    }
    return NULL;
}

const char *uri_userinfo(const struct uri *u)
{
    if (u) {
        return u->authority.userinfo;
    }
    return NULL;
}

const char *uri_host(const struct uri *u)
{
    if (u) {
        return u->authority.host;
    }
    return NULL;
}

const char *uri_port(const struct uri *u)
{
    if (u) {
        return u->authority.port;
    }
    return NULL;
}

const char *uri_path(const struct uri *u)
{
    if (u) {
        return u->path;
    }
    return NULL;
}

const char *uri_query(const struct uri *u)
{
    if (u) {
        return u->query;
    }
    return NULL;
}

const char *uri_fragment(const struct uri *u)
{
    if (u) {
        return u->fragment;
    }
    return NULL;
}

int uri_scheme_set(struct uri *u, const char *str)
{
    int r;

    if (!u) {
        return -EFAULT;
    }

    r = validate_scheme(str);
    if (r < 0) {
        return r;
    }

    free(u->scheme);
    u->scheme = NULL;
    if (str) {
        u->scheme = strdup(str);
        if (!u->scheme) {
            return -ENOMEM; // UNREACHABLE
        } // UNREACHABLE
    }

    // §3.1
    // An implementation should accept uppercase letters as equivalent to lowercase ... for the sake of robustness but should only produce lowercase scheme names for consistency.

    str_lowercase(u->scheme);

    return 0;
}

int uri_port_set(struct uri *u, const char *str)
{
    if (!u) {
        return -EFAULT;
    }

    if (!validate(str, is_port)) {
        return -EINVAL;
    }

    free(u->authority.port);
    u->authority.port = NULL;
    if (str && *str) {
        u->authority.port = strdup(str);
        if (!u->authority.port) {
            return -ENOMEM; // UNREACHABLE
        } // UNREACHABLE
        u->authority.has = true;
    }

    return 0;
}

int uri_path_set(struct uri *u, const char *path)
{
    char *eos;
    char *tmp;

    if (!u) {
        return -EFAULT;
    }

    if (!path || !*path) {
        free(u->unresolved_path);
        u->unresolved_path = NULL;

        free(u->path);
        u->path = NULL;
        return 0;
    }

    if (u->authority.has && (!u->unresolved_path || !*u->unresolved_path) && *path != '/') {
        // URI has authority, but no path: new path must be absolute.
        return -EINVAL;
    }

    if (*path == '/') {
        free(u->unresolved_path);
        u->unresolved_path = strdup(path);

    } else {
        if (u->unresolved_path) {
            eos = strrchr(u->unresolved_path, '/');
            if (eos) {
                eos++;
            } else {
                eos = u->unresolved_path;
            }
        } else {
            eos = u->unresolved_path;
        }

        tmp = (char *)malloc((size_t)(eos - u->unresolved_path) + strlen(path) + 1 /*NUL*/);
        if (u->unresolved_path) {
            memcpy(tmp, u->unresolved_path, (size_t)(eos - u->unresolved_path));
        }
        tmp[(size_t)(eos - u->unresolved_path)] = 0;
        strcat(tmp, path);

        free(u->unresolved_path);
        u->unresolved_path = tmp;
    }

    resolve_path(u);
    return 0;
}

int uri_query_set(struct uri *u, const char *str)
{
    if (!u) {
        return -EFAULT;
    }

    free(u->query);
    u->query = NULL;
    if (str) {
        u->query = strdup(str);
        if (!u->query) {
            return -ENOMEM; // UNREACHABLE
        } // UNREACHABLE
    }

    return 0;
}

int uri_fragment_set(struct uri *u, const char *str)
{
    if (!u) {
        return -EFAULT;
    }

    free(u->fragment);
    u->fragment = NULL;
    if (str) {
        u->fragment = strdup(str);
        if (!u->fragment) {
            return -ENOMEM; // UNREACHABLE
        } // UNREACHABLE
    }

    return 0;
}

static void uri_free(struct uri *u)
{
    if (u) {
        free(u->scheme);
        free(u->authority.userinfo);
        free(u->authority.host);
        free(u->authority.port);
        free(u->unresolved_path);
        free(u->path);
        free(u->query);
        free(u->fragment);
        memset(u, 0, sizeof(*u));
    }
}

void uri_delete(struct uri *u)
{
    uri_free(u);
    free(u);
}

/// @return Length of @c str, if non-NULL, plus length of @c extra if non-NULL.
static size_t calculate_size(char *str, char *extra)
{
    size_t n = 0;
    if (str) {
        n = strlen(str);
        if (extra) {
            n += strlen(extra);
        }
    }
    return n;
}

int uri_str(const struct uri *u, char **str_out)
{
    struct uri copy;
    size_t len = 0;
    char *str;

    if (!u || !str_out) {
        return -EFAULT;
    }

    errno = 0;

    // Make a percent-encoded copy in preparation for joining the components together.
    copy.scheme             = safe_strdup(u->scheme);
    copy.authority.has      = u->authority.has;
    copy.authority.userinfo = percent_encode(u->authority.userinfo, is_userinfo);
    copy.authority.host     = percent_encode(u->authority.host,     is_host);
    copy.authority.port     = safe_strdup(u->authority.port);
    copy.unresolved_path    = NULL;
    copy.path               = percent_encode(u->path,               is_path);
    copy.query              = percent_encode(u->query,              is_query);
    copy.fragment           = percent_encode(u->fragment,           is_query);

    if (errno) {
        return -errno; // UNREACHABLE
    } // UNREACHABLE

    len += calculate_size(copy.scheme, ":");

    if (copy.authority.has) {
        len += 2; /* "//" */
    }

    len += calculate_size(copy.authority.userinfo, "@");
    len += calculate_size(copy.authority.host, NULL);
    len += calculate_size(copy.authority.port, ":");
    len += calculate_size(copy.path, NULL) + 2 /* possible "/.", see below */;
    len += calculate_size(copy.query, "?");
    len += calculate_size(copy.fragment, "#");

    str = (char *)malloc(len + 1 /*NUL*/);
    if (!str) {
        return -errno; // UNREACHABLE
    } // UNREACHABLE

    *str = 0;

    if (copy.scheme) {
        strcat(str, copy.scheme);
        strcat(str, ":");
    }

    if (copy.authority.has) {
        strcat(str, "//");

        if (copy.authority.userinfo) {
            strcat(str, copy.authority.userinfo);
            strcat(str, "@");
        }

        if (copy.authority.host) {
            strcat(str, copy.authority.host);
        }

        if (copy.authority.port && copy.authority.port[0]) {
            strcat(str, ":");
            strcat(str, copy.authority.port);
        }
    }

    if (copy.path) {
        if (!copy.authority.has) {
            // https://url.spec.whatwg.org/#url-serializing
            if (!strncmp(copy.path, "//", 2)) {
                // Path looks like an authority.
                strcat(str, "/.");

            } else if (!copy.scheme) {
                char *iter;
                if ((iter = str_iterate(copy.path, is_alpha, is_scheme)) && *iter == ':') {
                    // Path looks like a scheme.
                    strcat(str, "./");
                }
            }
        }

        strcat(str, copy.path);
    }

    if (copy.query) {
        strcat(str, "?");
        strcat(str, copy.query);
    }

    if (copy.fragment) {
        strcat(str, "#");
        strcat(str, copy.fragment);
    }

    uri_free(&copy);

    *str_out = str;
    return 0;
}
