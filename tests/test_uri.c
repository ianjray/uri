#include "memory_shim.h"
#include "support.h"
#include "uri.h"

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PE_SOLIDUS "%2f" // U+002F "/"

#define LATIN_SMALL_LETTER_U_WITH_DIAERESIS "\xc3\xbc" // U+00FC
#define PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS "%C3%BC" // U+00FC

static void test_uri_new(void)
{
    // Invalid usage.
    uri_new_must_fail(EFAULT, NULL);

    // Malformed UTF-8 shall be rejected.
    uri_new_must_fail(EILSEQ, "bad-utf8\x80");
    uri_new_must_fail(EILSEQ, "overlong-utf8\xC0\xB1"); // 11000000 10110001 = 110(2-byte) + 10(continuation) + 110001 = 0x31 = "1"

    // Control characters shall be rejected.
    uri_new_must_fail(EINVAL, "bad-c0\r");       // U+000d
    uri_new_must_fail(EINVAL, "bad-c0%0d");      // U+000d
    uri_new_must_fail(EINVAL, "bad-c0\x0d");     // U+000d
    uri_new_must_fail(EINVAL, "bad-c1\xc2\x80"); // U+0080

    // No percent encoding.
    uri_new_must_fail(EINVAL, "schemepercent%41:");
    uri_new_must_fail(EINVAL, "scheme://:%31");

    // Invalid percent-encoding.
    uri_new_must_fail(EINVAL, "scheme://user:%fgpass@host:123/path/to?query#fragment");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host%fg:123/path/to?query#fragment");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:123/path%fg/to?query#fragment");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:123/path/to?query%fg#fragment");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:123/path/to?query#fragment%fg");

    // Percent-encoded control character.
    uri_new_must_fail(EINVAL, "scheme://user:%0apass@host:123/path/to?query#fragment");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host%0a:123/path/to?query#fragment");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:123/path%0a/to?query#fragment");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:123/path/to?query%0a#fragment");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:123/path/to?query#fragment%0a");

    // Non-allowed characters shall be rejected.
    uri_new_must_fail(EINVAL, "sch@me:");
    uri_new_must_fail(EINVAL, "scheme://user^invalid@");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host^");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:a");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:1/^");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:1/?^");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:1/?#^");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:1/?##");

    uri_new_must_fail(EINVAL, "scheme://user invalid@");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host ");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:a");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:1/ ");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:1/? ");
    uri_new_must_fail(EINVAL, "scheme://user:pass@host:1/?# ");

    // Not an "empty scheme", but rather a relative reference which has an illegal colon in first path segment.
    uri_new_must_fail(EINVAL, ":file");
    uri_new_must_fail(EINVAL, "://host");

    // Host: unterminated IP-literal.
    uri_new_must_fail(EINVAL, "//[2001:db8::1");

    // Invalid port character.
    uri_new_must_fail(EILSEQ, "//host:1\x80");
    uri_new_must_fail(EINVAL, "//host:123:456");

    // Empty components.
    ingest_match_components("?#", NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, NO_PATH, "", "");
    // The presence of authority means that path is absolute.
    ingest_match_components("///?#", NO_SCHEME, NO_USERINFO, "", NO_PORT, "/", "", "");
    ingest_match_components("//:/?#", NO_SCHEME, NO_USERINFO, "", "", "/", "", "");
    ingest_match_components("//@:/?#", NO_SCHEME, "", "", "", "/", "", "");

    // Lowercase.
    ingest_match_components("Ascheme:", "ascheme", NO_USERINFO, NO_HOST, NO_PORT, NO_PATH, NO_QUERY, NO_FRAGMENT);
    ingest_match_components("//HOST%41", NO_SCHEME, NO_USERINFO, "hosta", NO_PORT, NO_PATH, NO_QUERY, NO_FRAGMENT);

    // Drop leading zeroes.
    ingest_match_components("//:00080", NO_SCHEME, NO_USERINFO, "", "80", NO_PATH, NO_QUERY, NO_FRAGMENT);

    // IPv6 and port.
    ingest_match_components("//[2001:db8::1]:123", NO_SCHEME, NO_USERINFO, "[2001:db8::1]", "123", NO_PATH, NO_QUERY, NO_FRAGMENT);
}

static void test_uri_set(void)
{
    set_must_fail(EFAULT, NULL, NULL);
    set_must_fail(EFAULT, NULL, "foo");

    // Full example URI.
    struct uri *u = uri_new("scheme://user:pass@host:123/path/to?query#fragment");
    assert(u);

    assert(0 == uri_set(u, "#F"));
    uri_str_must_match(u, "scheme://user:pass@host:123/path/to?query#F");

    assert(0 == uri_set(u, "?Q#f"));
    uri_str_must_match(u, "scheme://user:pass@host:123/path/to?Q#f");

    uri_query_set(u, "");
    uri_str_must_match(u, "scheme://user:pass@host:123/path/to?#f");

    uri_fragment_set(u, "");
    uri_str_must_match(u, "scheme://user:pass@host:123/path/to?#");

    uri_query_set(u, NULL);
    uri_str_must_match(u, "scheme://user:pass@host:123/path/to#");

    uri_fragment_set(u, NULL);
    uri_str_must_match(u, "scheme://user:pass@host:123/path/to");

    assert(0 == uri_set(u, "?Q"));
    uri_str_must_match(u, "scheme://user:pass@host:123/path/to?Q");

    assert(0 == uri_set(u, "/path/from/other"));
    uri_str_must_match(u, "scheme://user:pass@host:123/path/from/other");

    // Set relative path.
    assert(0 == uri_set(u, "../sub"));
    uri_str_must_match(u, "scheme://user:pass@host:123/path/sub");

    assert(0 == uri_set(u, "//x/P"));
    uri_str_must_match(u, "scheme://x/P");

    assert(0 == uri_set(u, "//x/P?q"));
    uri_str_must_match(u, "scheme://x/P?q");

    // Make no changes.
    assert(0 == uri_set(u, ""));
    uri_str_must_match(u, "scheme://x/P?q");

    // Set absolute path which contains "..".
    assert(0 == uri_set(u, "/./path/./to/file/../other"));
    uri_str_must_match(u, "scheme://x/path/to/other");
    uri_components_must_match(
            u,
            "scheme",
            NO_USERINFO,
            "x",
            NO_PORT,
            "/path/to/other",
            NO_QUERY,
            NO_FRAGMENT);

    uri_delete(u);
}

static void test_uri_delete(void)
{
    uri_delete(NULL);
}

/// Tests common input validation.
static void test_setter_getter_common(const char *scenario, int (*setter)(struct uri *, const char *), const char *(*getter)(const struct uri *), struct uri *u)
{
    setter_must_fail(scenario, setter, EFAULT, NULL, NULL);
    assert(NULL == getter(NULL));

    assert( 0 == setter(u, NULL));
    assert(NULL == getter(u));

    setter_must_fail(scenario, setter, EFAULT, NULL, "this-input-is-unused");
    assert(NULL == getter(u));

    // Malformed UTF-8 shall be rejected.
    setter_must_fail(scenario, setter, EILSEQ, u, "bad-utf8\x80");
    assert(NULL == getter(u));

    setter_must_fail(scenario, setter, EILSEQ, u, "overlong-utf8\xC0\xB1"); // 11000000 10110001 = 110(2-byte) + 10(continuation) + 110001 = 0x31 = "1"
    assert(NULL == getter(u));

    // Control characters shall be rejected.
    setter_must_fail(scenario, setter, EINVAL, u, "bad-c0\r");       // U+000d
    assert(NULL == getter(u));

    setter_must_fail(scenario, setter, EINVAL, u, "bad-c0%0d");      // U+000d
    assert(NULL == getter(u));

    setter_must_fail(scenario, setter, EINVAL, u, "bad-c0\x0d");     // U+000d
    assert(NULL == getter(u));

    setter_must_fail(scenario, setter, EINVAL, u, "bad-c1\xc2\x80"); // U+0080
    assert(NULL == getter(u));

    if (getter == uri_scheme) {
        // No percent encoding.
        // Lowercase.
        assert(0 == setter(u, "Ascheme"));
        string_must_match(scenario, "L" STRINGIFY(__LINE__), getter(u), "ascheme");

    } else if (getter == uri_port) {
        // No percent encoding.
        // Leading zeroes dropped.
        assert(0 == setter(u, "00080"));
        string_must_match(scenario, "L" STRINGIFY(__LINE__), getter(u), "80");

    } else {
        // Redundant percent-encoding is replaced with literals.
        // Percent encoding is normalized (uppercase ASCIIHEX).
        if (setter(u, "%41%c3%bc%42")) {
            printf("FAIL: %s %s\n", scenario, "L" STRINGIFY(__LINE__));
            assert(0);
        }

        if (getter == uri_host) {
            // Lowercase.
            string_must_match(scenario, "L" STRINGIFY(__LINE__), getter(u), "a%C3%BCb");
        } else {
            string_must_match(scenario, "L" STRINGIFY(__LINE__), getter(u), "A%C3%BCB");
        }
    }

    if (getter == uri_scheme) {
        // No UTF-8.

    } else if (getter == uri_port) {
        // No UTF-8.

    } else {
        // Valid UTF-8 shall be automatically percent-encoded.
        assert( 0 == setter(u, "s" LATIN_SMALL_LETTER_U_WITH_DIAERESIS));
        string_must_match(scenario, "L" STRINGIFY(__LINE__), getter(u), "s" PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS);

        if (getter == uri_userinfo) {
            uri_str_must_match(u, "//s" PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS "@");

        } else if (getter == uri_host) {
            uri_str_must_match(u, "//s" PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS);
        }
    }

    assert( 0 == setter(u, NULL));
    assert(NULL == getter(u));
}

static void test_uri_scheme_component(void)
{
    struct uri *u = uri_new("");
    assert(u);

    test_setter_getter_common("L" STRINGIFY(__LINE__), uri_scheme_set, uri_scheme, u);

    // Non-allowed characters shall be rejected.
    scheme_set_must_fail(EINVAL, u, "5cheme");
    scheme_set_must_fail(EINVAL, u, "sc@m");
    scheme_set_must_fail(EINVAL, u, "sc%41m");

    // Empty scheme is illegal (use NULL to set no scheme).
    scheme_set_must_fail(EINVAL, u, "");
    assert(NULL == uri_scheme(u));

    assert( 0 == uri_scheme_set(u, NULL));
    assert(NULL == uri_scheme(u));

    // String shall be automatically converted to lowercase.
    assert( 0 == uri_scheme_set(u, "SCHeme"));
    assert(!strcmp(uri_scheme(u), "scheme"));

    uri_delete(u);
}

static void test_uri_userinfo_component(void)
{
    struct uri *u = uri_new("");
    assert(u);

    test_setter_getter_common("L" STRINGIFY(__LINE__), uri_userinfo_set, uri_userinfo, u);

    // Non-allowed characters shall be rejected.
    userinfo_set_must_fail(EINVAL, u, "@");
    userinfo_set_must_fail(EINVAL, u, "/");
    userinfo_set_must_fail(EINVAL, u, "?");
    userinfo_set_must_fail(EINVAL, u, "#");

    // Empty userinfo is legal.
    assert( 0 == uri_userinfo_set(u, ""));
    assert(!strcmp(uri_userinfo(u), ""));

    assert( 0 == uri_userinfo_set(u, NULL));
    assert(NULL == uri_userinfo(u));

    assert( 0 == uri_userinfo_set(u, "user%23pass"));
    assert(!strcmp(uri_userinfo(u), "user%23pass"));

    // When setting Authority, path must be absolute or empty.
    assert( 0 == uri_userinfo_set(u, NULL));
    assert( 0 == uri_path_set(u, "relative"));
    userinfo_set_must_fail(EINVAL, u, "user%23pass");
    assert(NULL == uri_userinfo(u));

    uri_delete(u);
}

static void test_uri_host_component(void)
{
    struct uri *u = uri_new("");
    assert(u);

    test_setter_getter_common("L" STRINGIFY(__LINE__), uri_host_set, uri_host, u);

    // Non-allowed characters shall be rejected.
    host_set_must_fail(EINVAL, u, "@");
    host_set_must_fail(EINVAL, u, "/");
    host_set_must_fail(EINVAL, u, "?");
    host_set_must_fail(EINVAL, u, "#");

    // Empty host is legal.
    assert( 0 == uri_host_set(u, ""));
    assert(!strcmp(uri_host(u), ""));

    // RFC 3986: Percent-encoding is used to represent characters that are not allowed literally in a given component or would otherwise be interpreted as delimiters.
    // For DNS-style hosts, there are no practical host characters that must be percent-encoded.
    // We use SOLIDUS (slash) in this test case.
    assert(0 == uri_host_set(u, "h" PE_SOLIDUS));
    string_must_match(__FUNCTION__, "L" STRINGIFY(__LINE__), "h%2F", uri_host(u));
    uri_str_must_match(u, "//h%2F");

    // String shall be automatically converted to lowercase.
    assert( 0 == uri_host_set(u, "hoST"));
    assert(!strcmp(uri_host(u), "host"));

    assert( 0 == uri_host_set(u, "[2001:db8::1]"));
    assert(!strcmp(uri_host(u), "[2001:db8::1]"));

    // When setting Authority, path must be absolute or empty.
    assert( 0 == uri_host_set(u, NULL));
    assert( 0 == uri_path_set(u, "relative"));
    host_set_must_fail(EINVAL, u, "host");
    assert(NULL == uri_host(u));

    uri_delete(u);
}

static void test_uri_port_component(void)
{
    struct uri *u = uri_new("");
    assert(u);

    test_setter_getter_common("L" STRINGIFY(__LINE__), uri_port_set, uri_port, u);

    // Non-allowed characters shall be rejected.
    port_set_must_fail(EINVAL, u, "#");
    port_set_must_fail(EINVAL, u, "a");
    port_set_must_fail(EINVAL, u, "-");

    // Empty port is legal.
    assert( 0 == uri_port_set(u, ""));
    assert(!strcmp(uri_port(u), ""));

    assert(0 == uri_port_set(u, NULL));
    assert(NULL == uri_port(u));

    // Leading digits are dropped.
    assert( 0 == uri_port_set(u, "00080"));
    assert(!strcmp(uri_port(u), "80"));

    // 0..65535
    port_set_must_fail(EINVAL, u, "-1");
    assert(!strcmp(uri_port(u), "80"));

    assert( 0 == uri_port_set(u, "0"));
    assert(!strcmp(uri_port(u), "0"));

    assert( 0 == uri_port_set(u, "0000065535"));
    assert(!strcmp(uri_port(u), "65535"));

    port_set_must_fail(ERANGE, u, "65536");
    assert(!strcmp(uri_port(u), "65535"));

    // Number too big to convert.
    {
        char buf[64];
        snprintf(buf, sizeof buf, "%lu0", ULONG_MAX);
        port_set_must_fail(ERANGE, u, buf);
    }

    uri_delete(u);

    u = uri_new("relative");

    // When setting Authority, path must be absolute or empty.
    port_set_must_fail(EINVAL, u, "1");
    assert(NULL == uri_port(u));

    assert( 0 == uri_path_set(u, "/absolute"));
    assert( 0 == uri_port_set(u, "1"));

    assert( 0 == uri_path_set(u, ""));
    assert( 0 == uri_port_set(u, "1"));

    assert( 0 == uri_path_set(u, NULL));
    assert( 0 == uri_port_set(u, "1"));

    uri_delete(u);
}

static void test_uri_path_component(void)
{
    struct uri *u = uri_new("");
    assert(u);

    test_setter_getter_common("L" STRINGIFY(__LINE__), uri_path_set, uri_path, u);

    // Non-allowed characters shall be rejected.
    path_set_must_fail(EINVAL, u, "?");
    path_set_must_fail(EINVAL, u, "#");

    // Empty path is legal.
    assert( 0 == uri_path_set(u, ""));
    assert(!strcmp(uri_path(u), ""));

    // "./" removal.
    assert( 0 == uri_path_set(u, NULL));
    assert( 0 == uri_path_set(u, "./path/./sub"));
    assert(!strcmp(uri_path(u), "path/sub"));

    // "../" removal.
    assert( 0 == uri_path_set(u, NULL));
    assert( 0 == uri_path_set(u, "../path/../sub"));
    assert(!strcmp(uri_path(u), "sub"));

    // "/." removal.
    assert( 0 == uri_path_set(u, NULL));
    assert( 0 == uri_path_set(u, "/./path/."));
    assert(!strcmp(uri_path(u), "/path/"));

    // "/.." removal.
    assert( 0 == uri_path_set(u, NULL));
    assert( 0 == uri_path_set(u, "/a/../b/c/.."));
    assert(!strcmp(uri_path(u), "/b/"));

    // "." removal.
    assert( 0 == uri_path_set(u, NULL));
    assert( 0 == uri_path_set(u, "."));
    assert(!strcmp(uri_path(u), ""));

    // ".." removal.
    assert( 0 == uri_path_set(u, NULL));
    assert( 0 == uri_path_set(u, ".."));
    assert(!strcmp(uri_path(u), ""));

    // If Authority is present, path must be absolute or empty.
    assert( 0 == uri_path_set(u, NULL));
    assert( 0 == uri_host_set(u, "host"));
    path_set_must_fail(EINVAL, u, "relative");

    assert( 0 == uri_path_set(u, "/absolute"));
    assert( 0 == uri_path_set(u, ""));
    assert( 0 == uri_path_set(u, NULL));

    uri_delete(u);

    // Path looks like a scheme.
    construct_set_path_expect("", "file:", "file:", "./file:");

    // Path looks like an authority.
    construct_set_path_expect("", "//file", "//file", "/.//file");

    // Percent-encoded period (%2E) is always normalized and scrutinized.
    construct_set_path_expect("//host", "/a/%2e%2E/b", "/b", "//host/b");
}

static void test_uri_query_component(void)
{
    struct uri *u = uri_new("");
    assert(u);

    test_setter_getter_common("L" STRINGIFY(__LINE__), uri_query_set, uri_query, u);

    // Non-allowed characters shall be rejected.
    query_set_must_fail(EINVAL, u, "#");

    // Empty query is legal.
    assert( 0 == uri_query_set(u, ""));
    assert(!strcmp(uri_query(u), ""));

    assert( 0 == uri_query_set(u, "query"));
    assert(!strcmp(uri_query(u), "query"));

    uri_delete(u);
}

static void test_uri_fragment_component(void)
{
    struct uri *u = uri_new("");
    assert(u);

    test_setter_getter_common("L" STRINGIFY(__LINE__), uri_fragment_set, uri_fragment, u);

    // Non-allowed characters shall be rejected.
    fragment_set_must_fail(EINVAL, u, "#");

    // Empty fragment is legal.
    assert( 0 == uri_fragment_set(u, ""));
    assert(!strcmp(uri_fragment(u), ""));

    assert( 0 == uri_fragment_set(u, "fragment"));
    assert(!strcmp(uri_fragment(u), "fragment"));

    uri_delete(u);
}

static void test_uri_str(void)
{
    errno = 0;
    assert(NULL == uri_str(NULL));
    assert(EFAULT == errno);
}

static bool match(const struct uri *u, const char *expected)
{
    char *actual = uri_str(u);
    if (!actual) {
        return false;
    }
    int r = strcmp(actual, expected);
    if (r) {
        printf("FAIL: %s; expected %s actual %s\n", __FUNCTION__, expected, actual);
        assert(!r);
    }
    free(actual);
    return true;
}

static void test_full_api(void)
{
    struct uri *u = uri_new("scheme://user:password@host:123/path/to?query?q#fragment?q");
    if (!u) {
        return;
    }

    if (!match(u, "scheme://user:password@host:123/path/to?query?q#fragment?q")) {
        goto out;
    }

    // Exercise uri_xxx_set.

    // Change scheme.
    int r = uri_scheme_set(u, "other");
    if (r == 0) {
        assert(!strcmp("other", uri_scheme(u)));
        if (!match(u, "other://user:password@host:123/path/to?query?q#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("scheme", uri_scheme(u)));
        goto out;
    }

    // Remove scheme.
    assert(0 == uri_scheme_set(u, NULL));
    assert(NULL == uri_scheme(u));
    if (!match(u, "//user:password@host:123/path/to?query?q#fragment?q")) {
        goto out;
    }

    // Change userinfo.
    r = uri_userinfo_set(u, "different");
    if (r == 0) {
        assert(!strcmp("different", uri_userinfo(u)));
        if (!match(u, "//different@host:123/path/to?query?q#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("user:password", uri_userinfo(u)));
        goto out;
    }

    // Remove userinfo.
    assert(0 == uri_userinfo_set(u, NULL));
    assert(NULL == uri_userinfo(u));
    if (!match(u, "//host:123/path/to?query?q#fragment?q")) {
        goto out;
    }

    // Change host.
    r = uri_host_set(u, "changed");
    if (r == 0) {
        assert(!strcmp("changed", uri_host(u)));
        if (!match(u, "//changed:123/path/to?query?q#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("host", uri_host(u)));
        goto out;
    }

    // Empty host.
    r = uri_host_set(u, "");
    if (r == 0) {
        assert(!strcmp("", uri_host(u)));
        if (!match(u, "//:123/path/to?query?q#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("changed", uri_host(u)));
        goto out;
    }

    // Remove host.
    assert(0 == uri_host_set(u, NULL));
    assert(NULL == uri_host(u));
    if (!match(u, "//:123/path/to?query?q#fragment?q")) {
        goto out;
    }

    // Change port.
    r = uri_port_set(u, "456");
    if (r == 0) {
        assert(!strcmp("456", uri_port(u)));
        if (!match(u, "//:456/path/to?query?q#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("123", uri_port(u)));
        goto out;
    }

    // Empty port.
    r = uri_port_set(u, "");
    if (r == 0) {
        assert(!strcmp("", uri_port(u)));
        if (!match(u, "//:/path/to?query?q#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("456", uri_port(u)));
        goto out;
    }

    // Remove port.
    assert(0 == uri_port_set(u, NULL));
    assert(NULL == uri_port(u));
    if (!match(u, "/path/to?query?q#fragment?q")) {
        goto out;
    }

    // Change path.
    r = uri_path_set(u, "sub");
    if (r == 0) {
        assert(!strcmp("/path/sub", uri_path(u)));
        if (!match(u, "/path/sub?query?q#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("/path/to", uri_path(u)));
        goto out;
    }

    // Empty path.
    r = uri_path_set(u, "");
    if (r == 0) {
        assert(!strcmp("/path/", uri_path(u)));
        if (!match(u, "/path/?query?q#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("/path/sub", uri_path(u)));
        goto out;
    }

    // Remove path.
    assert(0 == uri_path_set(u, NULL));
    assert(NULL == uri_path(u));
    if (!match(u, "?query?q#fragment?q")) {
        goto out;
    }

    // Change query.
    r = uri_query_set(u, "new");
    if (r == 0) {
        assert(!strcmp("new", uri_query(u)));
        if (!match(u, "?new#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("query?q", uri_query(u)));
        goto out;
    }

    // Empty query.
    r = uri_query_set(u, "");
    if (r == 0) {
        assert(!strcmp("", uri_query(u)));
        if (!match(u, "?#fragment?q")) {
            goto out;
        }
    } else {
        assert(!strcmp("new", uri_query(u)));
        goto out;
    }

    // Remove query.
    assert(0 == uri_query_set(u, NULL));
    assert(NULL == uri_query(u));
    if (!match(u, "#fragment?q")) {
        goto out;
    }

    // Change fragment.
    r = uri_fragment_set(u, "where");
    if (r == 0) {
        assert(!strcmp("where", uri_fragment(u)));
        if (!match(u, "#where")) {
            goto out;
        }
    } else {
        assert(!strcmp("fragment?q", uri_fragment(u)));
        goto out;
    }

    // Empty fragment.
    r = uri_fragment_set(u, "");
    if (r == 0) {
        assert(!strcmp("", uri_fragment(u)));
        if (!match(u, "#")) {
            goto out;
        }
    } else {
        assert(!strcmp("where", uri_fragment(u)));
        goto out;
    }

    // Remove fragment.
    assert(0 == uri_fragment_set(u, NULL));
    assert(NULL == uri_fragment(u));
    if (!match(u, "")) {
        goto out;
    }

    // Exercise uri_set.

    // Ingest full URI.
    r = uri_set(u, "scheme://user:password@host:123/path/to?query?q#fragment?q");
    if (r == 0) {
        if (!match(u, "scheme://user:password@host:123/path/to?query?q#fragment?q")) {
            goto out;
        }
    } else {
        match(u, "");
        goto out;
    }

    // Change fragment.
    r = uri_set(u, "#F");
    if (r == 0) {
        if (!match(u, "scheme://user:password@host:123/path/to?query?q#F")) {
            goto out;
        }
    } else {
        match(u, "scheme://user:password@host:123/path/to?query?q#fragment?q");
        goto out;
    }

    // Change query.
    r = uri_set(u, "?Q");
    if (r == 0) {
        if (!match(u, "scheme://user:password@host:123/path/to?Q")) {
            goto out;
        }
    } else {
        match(u, "scheme://user:password@host:123/path/to?query?q#F");
        goto out;
    }

    // Relative path, no query.
    r = uri_set(u, "relative/node");
    if (r == 0) {
        if (!match(u, "scheme://user:password@host:123/path/relative/node")) {
            goto out;
        }
    } else {
        match(u, "scheme://user:password@host:123/path/to?Q");
        goto out;
    }

    // Relative path and query.
    r = uri_set(u, "../there?q");
    if (r == 0) {
        if (!match(u, "scheme://user:password@host:123/path/there?q")) {
            goto out;
        }
    } else {
        match(u, "scheme://user:password@host:123/path/relative/node");
        goto out;
    }

    // Absolute path.
    r = uri_set(u, "/absolute/place");
    if (r == 0) {
        if (!match(u, "scheme://user:password@host:123/absolute/place")) {
            goto out;
        }
    } else {
        match(u, "scheme://user:password@host:123/path/there?q");
        goto out;
    }

    // Full.
    r = uri_set(u, "scheme://user%40:pass%2fword@%40host:123/path%40/to?query%40?q#fragment%40?q");
    if (r == 0) {
        if (!match(u, "scheme://user%40:pass%2Fword@%40host:123/path%40/to?query%40?q#fragment%40?q")) {
            goto out;
        }
    } else {
        match(u, "scheme://user:password@host:123/absolute/place");
        goto out;
    }

out:
    uri_delete(u);
}

static void test_memory_failure_handling(void)
{
    // Count total required number of allocations.
    memory_shim_reset();

    test_full_api();

    unsigned count = memory_shim_count_get();
    printf("%u allocations total.\n", count);

    // Fail Nth allocation to test error handling code paths.
    for (unsigned n = 1; n <= count; ++n) {
        memory_shim_fail_at(n);
        test_full_api();
    }

    // Reset.
    memory_shim_reset();
}

int main(void)
{
    test_uri_new();
    test_uri_set();
    test_uri_delete();
    test_uri_scheme_component();
    test_uri_userinfo_component();
    test_uri_host_component();
    test_uri_port_component();
    test_uri_path_component();
    test_uri_query_component();
    test_uri_fragment_component();
    test_uri_str();

    test_memory_failure_handling();

    printf("Testing completed.\n");
    return 0;
}
