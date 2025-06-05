#include "uri.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void compare(const char *uri, const char *component, const char *s1, const char *s2)
{
    if (s1 == s2 || (s1 && s2 && !strcmp(s1, s2))) {
        return;
    }

    printf("error: tokenize: |%s| component %s:\n"
           "actual   |%s|\n"
           "expected |%s|\n"
           , uri
           , component
           , s1 ? s1 : "(null)"
           , s2 ? s2 : "(null)"
          );
    abort();
}

static struct uri *create(const char *seq, const char *scheme, const char *userinfo, const char *host, const char *port, const char *path, const char *query, const char *fragment)
{
    struct uri *u;
    int r = uri_new(seq, &u);
    if (r < 0) {
        printf("error: create: %s: |%s|\n", strerror(-r), seq);
        abort();
    }

    compare(seq, "scheme",   uri_scheme(u),   scheme);
    compare(seq, "userinfo", uri_userinfo(u), userinfo);
    compare(seq, "host",     uri_host(u),     host);
    compare(seq, "port",     uri_port(u),     port);
    compare(seq, "path",     uri_path(u),     path);
    compare(seq, "query",    uri_query(u),    query);
    compare(seq, "fragment", uri_fragment(u), fragment);

    return u;
}

static void good_uri(const char *seq, const char *scheme, const char *userinfo, const char *host, const char *port, const char *path, const char *query, const char *fragment)
{
    uri_delete(create(seq, scheme, userinfo, host, port, path, query, fragment));
}

static void bad_uri(const char *seq)
{
    struct uri *u;
    int r = uri_new(seq, &u);
    if (r == 0) {
        printf("error: bad_uri: unexpected pass: |%s| |%s|%s|%s|%s|%s|%s|%s|\n",
                seq,
                uri_scheme(u),
                uri_userinfo(u),
                uri_host(u),
                uri_port(u),
                uri_path(u),
                uri_query(u),
                uri_fragment(u));
        abort();
    }
}

static void roundtrip(const char *seq, const char *scheme, const char *userinfo, const char *host, const char *port, const char *path, const char *query, const char *fragment, const char *joined)
{
    struct uri *u = create(seq, scheme, userinfo, host, port, path, query, fragment);
    char *str;
    int r = uri_str(u, &str);
    if (r < 0) {
        printf("error: roundtrip: %s: |%s|\n", strerror(-r), seq);
        abort();
    } else {
        if (strcmp(str, joined)) {
            printf("error: roundtrip: |%s|\n"
                "actual   |%s|\n"
                "expected |%s|\n"
                , seq
                , str
                , joined);
            abort();
        }
    }

    free(str);

    uri_delete(u);
}

static void test_modify(const char *seq, const char *changed_path, const char *expected_seq)
{
    struct uri *u = NULL;
    char *str = NULL;
    int r;

    r = uri_new(seq, &u);
    if (r < 0) {
        printf("error: modify: new: %s: |%s|\n", strerror(-r), seq);
        abort();
    }

    r = uri_path_set(u, changed_path);
    if (r < 0) {
        if (expected_seq) {
            printf("error: modify: set: %s: |%s| |%s|\n", strerror(-r), seq, changed_path);
            abort();
        }

    } else {
        r = uri_str(u, &str);
        if (r < 0) {
            if (expected_seq) {
                printf("error: modify: str: %s: |%s|%s|%s|%s|%s|%s|%s|\n",
                        strerror(-r),
                        uri_scheme(u),
                        uri_userinfo(u),
                        uri_host(u),
                        uri_port(u),
                        uri_path(u),
                        uri_query(u),
                        uri_fragment(u));
                abort();
            }
        } else {
            if (!expected_seq) {
                printf("error: modify: unexpected pass: |%s|%s|\n"
                    "str |%s|\n"
                    , seq
                    , changed_path
                    , str);
                abort();
            }

            if (strcmp(str, expected_seq)) {
                printf("error: modify: |%s|%s|\n"
                    "actual   |%s|\n"
                    "expected |%s|\n"
                    , seq
                    , changed_path
                    , str
                    , expected_seq);
                abort();
            }
        }
    }

    free(str);

    uri_delete(u);
}

static void test_set(const char *seq, const char *set, const char *expected_seq)
{
    struct uri *u;
    char *str;
    int r = uri_new(seq, &u);
    assert(r == 0);

    r = uri_set(u, set);
    if (r < 0) {
        if (expected_seq) {
            printf("error: set: unexpected fail: |%s|\n"
                , seq);
            abort();
        }
        uri_delete(u);
        return;

    } else {
        if (!expected_seq) {
            printf("error: set: unexpected pass: |%s|\n"
                , seq);
            abort();
        }
    }

    r = uri_str(u, &str);
    assert(r == 0);

    if (strcmp(str, expected_seq)) {
        printf("error: set: |%s|: |%s|:\n"
            "actual   |%s|\n"
            "expected |%s|\n"
            , seq
            , set
            , str
            , expected_seq);
        abort();
    }

    free(str);
    uri_delete(u);
}

int main(void)
{
    {
        struct uri *u = NULL;
        struct uri *v = NULL;
        char *str;

        assert(-EFAULT == uri_new(NULL, NULL));
        assert(-EFAULT == uri_set(NULL, NULL));
        assert(-EFAULT == uri_set(NULL, "s://h/p"));
        assert(-EFAULT == uri_scheme_set(NULL, "s"));
        assert(-EFAULT == uri_port_set(NULL, "1"));
        assert(-EFAULT == uri_path_set(NULL, "p"));
        assert(-EFAULT == uri_query_set(NULL, "?"));
        assert(-EFAULT == uri_fragment_set(NULL, "#"));
        assert(-EFAULT == uri_str(NULL, NULL));
        assert(-EFAULT == uri_str(NULL, &str));

        assert(      0 == uri_new("p", &u));
        assert(-EFAULT == uri_set(u, NULL));
        assert(-EINVAL == uri_scheme_set(u, "5cheme"));
        assert(-EINVAL == uri_scheme_set(u, "s@"));
        assert(-EINVAL == uri_port_set(u, "a"));
        assert(-EFAULT == uri_str(u, NULL));
        assert(      0 == uri_str(u, &str));
        free(str);
        uri_delete(u);

        assert(-EINVAL == uri_new("//%Z0", &u));

        assert(NULL == uri_scheme(NULL));
        assert(NULL == uri_host(NULL));
        assert(NULL == uri_userinfo(NULL));
        assert(NULL == uri_port(NULL));
        assert(NULL == uri_path(NULL));
        assert(NULL == uri_query(NULL));
        assert(NULL == uri_fragment(NULL));

        assert(      0 == uri_new("foo://h%2fh", &u));
        assert(!strcmp(uri_scheme(u), "foo"));
        assert(      0 == uri_scheme_set(u, "s"));
        assert(!strcmp(uri_scheme(u), "s"));
        // Path must be absolute when authority is present.
        assert(-EINVAL == uri_path_set(u, "path here"));
        // Reserved characters will be percent-encoded.
        assert(      0 == uri_path_set(u, "/path here"));
        assert(!strcmp(uri_host(u), "h/h"));
        assert(!strcmp(uri_path(u), "/path here"));
        assert(0 == uri_str(u, &str));
        assert(!strcmp(str, "s://h%2Fh/path%20here"));
        free(str);
        uri_delete(u);

        assert(      0 == uri_new("p", &u));
        assert(      0 == strcmp(uri_path(u), "p"));
        assert(-EINVAL == uri_port_set(u, "1"));

        assert(      0 == uri_path_set(u, "/p"));
        assert(      0 == uri_port_set(u, "1"));
        assert(      0 == uri_str(u, &str));
        assert(      0 == strcmp(str, "//:1/p"));
        free(str);

        assert(      0 == uri_port_set(u, NULL));
        assert(      0 == uri_str(u, &str));
        assert(      0 == strcmp(str, "/p"));
        free(str);
        uri_delete(u);

        assert(0 == uri_new("s://h%2fh", &u));
        // Caller has escaped the slash.  Percent-encoding is used for convenience.
        assert(0 == uri_path_set(u, "/path%2fhere"));
        assert(!strcmp(uri_path(u), "/path%2fhere"));
        assert(0 == uri_str(u, &str));
        assert(!strcmp(str, "s://h%2Fh/path%252fhere"));
        free(str);
        uri_delete(u);

        assert(0 == uri_new("s://u@h:1/p", &u));
        assert(0 == uri_path_set(u, "/p?q#f"));
        assert(0 == uri_str(u, &str));
        assert(!strcmp(str, "s://u@h:1/p%3Fq%23f"));
        free(str);
        assert(0 == uri_query_set(u, "?q#f"));
        assert(0 == uri_str(u, &str));
        assert(!strcmp(str, "s://u@h:1/p%3Fq%23f??q%23f"));
        free(str);
        assert(0 == uri_fragment_set(u, "?q#f"));
        assert(0 == uri_str(u, &str));
        assert(!strcmp(str, "s://u@h:1/p%3Fq%23f??q%23f#?q%23f"));
        free(str);

        assert(-EFAULT == uri_dup(NULL, &v));
        assert(-EFAULT == uri_dup(u, NULL));
        assert(0 == uri_dup(u, &v));
        assert(NULL != v);
        assert(!strcmp(uri_scheme(u),   uri_scheme(v)));
        assert(!strcmp(uri_userinfo(u), uri_userinfo(v)));
        assert(!strcmp(uri_host(u),     uri_host(v)));
        assert(!strcmp(uri_port(u),     uri_port(v)));
        assert(!strcmp(uri_path(u),     uri_path(v)));
        assert(!strcmp(uri_query(u),    uri_query(v)));
        assert(!strcmp(uri_fragment(u), uri_fragment(v)));
        uri_delete(v);

        uri_delete(u);
    }

    test_set("s:",         "s%:",         NULL);

    test_set("s:",         "p",           "s:p");
    test_set("s:",         "/p",          "s:/p");

    test_set("s://h/p/q",  "/p/q?r",      "s://h/p/q?r");
    test_set("s://h/p/q",  "/p/q?r%23t",  "s://h/p/q?r%23t");
    test_set("s://h/p/q",  "/p/q#r",      "s://h/p/q#r");
    test_set("s://h/p/q",  "/p/q#r%23t",  "s://h/p/q#r%23t");

    // §5.4.1
    test_set("http://a/b/c/d;p?q",  "g:h"           ,  "g:h");
    test_set("http://a/b/c/d;p?q",  "g"             ,  "http://a/b/c/g");
    test_set("http://a/b/c/d;p?q",  "./g"           ,  "http://a/b/c/g");
    test_set("http://a/b/c/d;p?q",  "g/"            ,  "http://a/b/c/g/");
    test_set("http://a/b/c/d;p?q",  "/g"            ,  "http://a/g");
    test_set("http://a/b/c/d;p?q",  "//g"           ,  "http://g");
    test_set("http://a/b/c/d;p?q",  "?y"            ,  "http://a/b/c/d;p?y");
    test_set("http://a/b/c/d;p?q",  "g?y"           ,  "http://a/b/c/g?y");
    test_set("http://a/b/c/d;p?q",  "#s"            ,  "http://a/b/c/d;p?q#s");
    test_set("http://a/b/c/d;p?q",  "g#s"           ,  "http://a/b/c/g#s");
    test_set("http://a/b/c/d;p?q",  "g?y#s"         ,  "http://a/b/c/g?y#s");
    test_set("http://a/b/c/d;p?q",  ";x"            ,  "http://a/b/c/;x");
    test_set("http://a/b/c/d;p?q",  "g;x"           ,  "http://a/b/c/g;x");
    test_set("http://a/b/c/d;p?q",  "g;x?y#s"       ,  "http://a/b/c/g;x?y#s");
    test_set("http://a/b/c/d;p?q",  ""              ,  "http://a/b/c/d;p?q");
    test_set("http://a/b/c/d;p?q",  "."             ,  "http://a/b/c/");
    test_set("http://a/b/c/d;p?q",  "./"            ,  "http://a/b/c/");
    test_set("http://a/b/c/d;p?q",  ".."            ,  "http://a/b/");
    test_set("http://a/b/c/d;p?q",  "../"           ,  "http://a/b/");
    test_set("http://a/b/c/d;p?q",  "../g"          ,  "http://a/b/g");
    test_set("http://a/b/c/d;p?q",  "../.."         ,  "http://a/");
    test_set("http://a/b/c/d;p?q",  "../../"        ,  "http://a/");
    test_set("http://a/b/c/d;p?q",  "../../g"       ,  "http://a/g");

    // §5.4.2
    test_set("http://a/b/c/d;p?q",  "../../../g"    ,  "http://a/g");
    test_set("http://a/b/c/d;p?q",  "../../../../g" ,  "http://a/g");
    test_set("http://a/b/c/d;p?q",  "/./g"          ,  "http://a/g");
    test_set("http://a/b/c/d;p?q",  "/../g"         ,  "http://a/g");
    test_set("http://a/b/c/d;p?q",  "g."            ,  "http://a/b/c/g.");
    test_set("http://a/b/c/d;p?q",  ".g"            ,  "http://a/b/c/.g");
    test_set("http://a/b/c/d;p?q",  "g.."           ,  "http://a/b/c/g..");
    test_set("http://a/b/c/d;p?q",  "..g"           ,  "http://a/b/c/..g");
    test_set("http://a/b/c/d;p?q",  "./../g"        ,  "http://a/b/g");
    test_set("http://a/b/c/d;p?q",  "./g/."         ,  "http://a/b/c/g/");
    test_set("http://a/b/c/d;p?q",  "g/./h"         ,  "http://a/b/c/g/h");
    test_set("http://a/b/c/d;p?q",  "g/../h"        ,  "http://a/b/c/h");
    test_set("http://a/b/c/d;p?q",  "g;x=1/./y"     ,  "http://a/b/c/g;x=1/y");
    test_set("http://a/b/c/d;p?q",  "g;x=1/../y"    ,  "http://a/b/c/y");
    test_set("http://a/b/c/d;p?q",  "g?y/./x"       ,  "http://a/b/c/g?y/./x");
    test_set("http://a/b/c/d;p?q",  "g?y/../x"      ,  "http://a/b/c/g?y/../x");
    test_set("http://a/b/c/d;p?q",  "g#s/./x"       ,  "http://a/b/c/g#s/./x");
    test_set("http://a/b/c/d;p?q",  "g#s/../x"      ,  "http://a/b/c/g#s/../x");
    test_set("http://a/b/c/d;p?q",  "http:g"        ,  "http:g");


    good_uri("path",                    NULL,      NULL,    NULL,    NULL,   "path",       NULL,  NULL);
    good_uri("./path:",                 NULL,      NULL,    NULL,    NULL,   "path:",      NULL,  NULL);
    good_uri("/path",                   NULL,      NULL,    NULL,    NULL,   "/path",      NULL,  NULL);
    good_uri("path//to",                NULL,      NULL,    NULL,    NULL,   "path//to",   NULL,  NULL);
    good_uri("/path//to",               NULL,      NULL,    NULL,    NULL,   "/path//to",  NULL,  NULL);
    good_uri("///path",                 NULL,      NULL,    "",      NULL,   "/path",      NULL,  NULL);
    good_uri("///?#",                   NULL,      NULL,    "",      NULL,   "/",          "",    "");
    good_uri(":///?#",                  NULL,      NULL,    NULL,    NULL,   ":///",       "",    "");
    good_uri("//",                      NULL,      NULL,    "",      NULL,   NULL,         NULL,  NULL);
    good_uri("//host",                  NULL,      NULL,    "host",  NULL,   NULL,         NULL,  NULL);
    good_uri("//host:",                 NULL,      NULL,    "host",  "",     NULL,         NULL,  NULL);
    good_uri("//host:123",              NULL,      NULL,    "host",  "123",  NULL,         NULL,  NULL);
    good_uri("scheme:",                 "scheme",  NULL,    NULL,    NULL,   NULL,         NULL,  NULL);
    good_uri("scheme:path",             "scheme",  NULL,    NULL,    NULL,   "path",       NULL,  NULL);
    good_uri("scheme:/path",            "scheme",  NULL,    NULL,    NULL,   "/path",      NULL,  NULL);
    good_uri("scheme:///path",          "scheme",  NULL,    "",      NULL,   "/path",      NULL,  NULL);
    good_uri("scheme://host",           "scheme",  NULL,    "host",  NULL,   NULL,         NULL,  NULL);
    good_uri("scheme://host:123",       "scheme",  NULL,    "host",  "123",  NULL,         NULL,  NULL);
    good_uri("scheme://host:123/path",  "scheme",  NULL,    "host",  "123",  "/path",      NULL,  NULL);
    good_uri("scheme://host/path",      "scheme",  NULL,    "host",  NULL,   "/path",      NULL,  NULL);
    good_uri("scheme://user@:/path",    "scheme",  "user",  "",      "",     "/path",      NULL,  NULL);
    good_uri("scheme://user@:4/path",   "scheme",  "user",  "",      "4",    "/path",      NULL,  NULL);

    good_uri("scheme://user:password@host:123/path/to?query?q#fragment?q",
        "scheme",
        "user:password",
        "host",
        "123",
        "/path/to",
        "query?q",
        "fragment?q");

    good_uri("?qu://ery", NULL, NULL, NULL, NULL, NULL, "qu://ery", NULL);

    // Invalid percent-encodings.
    bad_uri("scheme:%%");
    bad_uri("scheme:%a");
    bad_uri("scheme:%a ");
    bad_uri("scheme:%ag");
    bad_uri("scheme:% 2e");

    // Strictly the '#' in the fragment is not in the allowed characters defined by RFC 3986, so it must be percent-encoded.
    bad_uri("#frag://me?nt#more");
    good_uri("#frag://me?nt%23more", NULL, NULL, NULL, NULL, NULL, NULL, "frag://me?nt#more");

    good_uri("http://[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443",
        "http",
        NULL,
        "[2001:db8:85a3:8d3:1319:8a2e:370:7348]",
        "443",
        NULL,
        NULL,
        NULL);

    // §7.3
    // Note, however, that the "%00" percent-encoding (NUL) may require special handling and should be rejected if the application is not expecting to receive raw data within a component.
    bad_uri("scheme:%00");

    bad_uri("scheme:/[]");
    good_uri("scheme:/%5B%5D/",  "scheme",  NULL,  NULL,  NULL,  "/[]/",  NULL,  NULL);
    good_uri("scheme://[]/",     "scheme",  NULL,  "[]",  NULL,  "/",     NULL,  NULL);

    good_uri("SCheME://user%40@host%2D%2d:123/%7e/path:1%2f%2F//@2/!$&'()*+,;=/more?qu%23ery%7e#%7efragment%7e",
        "scheme",
        "user@",
        "host--",
        "123",
        "/~/path:1////@2/!$&'()*+,;=/more",
        "qu#ery~",
        "~fragment~");

    // Additional resolution tests (see also §5.4)

    good_uri(                  "../",  NULL,    NULL,  NULL,  NULL,  "",         NULL,      NULL);
    good_uri(                   "./",  NULL,    NULL,  NULL,  NULL,  "",         NULL,      NULL);
    good_uri(                  "/./",  NULL,    NULL,  NULL,  NULL,  "/",        NULL,      NULL);
    good_uri(                   "/.",  NULL,    NULL,  NULL,  NULL,  "/",        NULL,      NULL);
    good_uri(                 "/../",  NULL,    NULL,  NULL,  NULL,  "/",        NULL,      NULL);
    good_uri(                  "/..",  NULL,    NULL,  NULL,  NULL,  "/",        NULL,      NULL);
    good_uri(                    ".",  NULL,    NULL,  NULL,  NULL,  "",         NULL,      NULL);
    good_uri(                   "..",  NULL,    NULL,  NULL,  NULL,  "",         NULL,      NULL);
    good_uri(                    "a",  NULL,    NULL,  NULL,  NULL,  "a",        NULL,      NULL);
    good_uri(                   "/a",  NULL,    NULL,  NULL,  NULL,  "/a",       NULL,      NULL);
    good_uri(                   "a/",  NULL,    NULL,  NULL,  NULL,  "a/",       NULL,      NULL);
    good_uri(                  "/a/",  NULL,    NULL,  NULL,  NULL,  "/a/",      NULL,      NULL);
    good_uri(                  "a/.",  NULL,    NULL,  NULL,  NULL,  "a/",       NULL,      NULL);
    good_uri(                 "/a/.",  NULL,    NULL,  NULL,  NULL,  "/a/",      NULL,      NULL);
    good_uri(                "/a/..",  NULL,    NULL,  NULL,  NULL,  "/",        NULL,      NULL);
    good_uri(                 "a/..",  NULL,    NULL,  NULL,  NULL,  "",         NULL,      NULL);
    good_uri(               "a/../b",  NULL,    NULL,  NULL,  NULL,  "b",        NULL,      NULL);
    good_uri(              "a//../b",  NULL,    NULL,  NULL,  NULL,  "a/b",      NULL,      NULL);
    good_uri(               "a/b/..",  NULL,    NULL,  NULL,  NULL,  "a/",       NULL,      NULL);
    good_uri(              "/a/b/..",  NULL,    NULL,  NULL,  NULL,  "/a/",      NULL,      NULL);
    good_uri(     "./a/./b/./../c/.",  NULL,    NULL,  NULL,  NULL,  "a/c/",     NULL,      NULL);
    good_uri( "./a/./b/./%2e%2E/c/.",  NULL,    NULL,  NULL,  NULL,  "a/c/",     NULL,      NULL);
    good_uri(    "./a/./b/./../c/..",  NULL,    NULL,  NULL,  NULL,  "a/",       NULL,      NULL);
    good_uri(    "./a/./b/./../c/.d",  NULL,    NULL,  NULL,  NULL,  "a/c/.d",   NULL,      NULL);
    good_uri(   "./a/./b/./../c/..d",  NULL,    NULL,  NULL,  NULL,  "a/c/..d",  NULL,      NULL);
    good_uri(             ".?a/../b",  NULL,    NULL,  NULL,  NULL,  "",         "a/../b",  NULL);

    // Additional resolution tests (see also §5.4.2)

    good_uri(  "http://a/b/c/g?y/./x",   "http",  NULL,  "a",   NULL,  "/b/c/g",   "y/./x",   NULL);
    good_uri( "http://a/b/c/g?y/../x",   "http",  NULL,  "a",   NULL,  "/b/c/g",   "y/../x",  NULL);
    good_uri(  "http://a/b/c/g#s/./x",   "http",  NULL,  "a",   NULL,  "/b/c/g",   NULL,      "s/./x");
    good_uri( "http://a/b/c/g#s/../x",   "http",  NULL,  "a",   NULL,  "/b/c/g",   NULL,      "s/../x");

    // §4.2
    // A path segment that contains a colon character (e.g., "this:that") cannot be used as the first segment of a relative-path reference, as it would be mistaken for a scheme name.

    roundtrip("./this:that",        NULL,     NULL,  NULL,  NULL,  "this:that",  NULL,  NULL,  "./this:that");
    roundtrip("scheme:this:that",  "scheme",  NULL,  NULL,  NULL,  "this:that",  NULL,  NULL,  "scheme:this:that");

    // §6.2.2

    good_uri("eXAMPLE://a/./b/../b/%63/%7bfoo%7d",
        "example",
        NULL,
        "a",
        NULL,
        "/b/c/{foo}",
        NULL,
        NULL);

    // UTF-8
    bad_uri("scheme:ßeta");
    roundtrip("scheme:%c3%9feta%25",  "scheme", NULL, NULL, NULL, "ßeta%",  NULL, NULL, "scheme:%C3%9Feta%25");

    // https://en.wikipedia.org/wiki/Internationalized_Resource_Identifier
    // https://en.wiktionary.org/wiki/Ῥόδος
    roundtrip("https://en.wiktionary.org/hiki/../wiki/%E1%BF%AC%CF%8C%CE%B4%CE%BF%cf%82",
        "https",
        NULL,
        "en.wiktionary.org",
        NULL,
        "/wiki/Ῥόδος",
        NULL,
        NULL,
        "https://en.wiktionary.org/wiki/%E1%BF%AC%CF%8C%CE%B4%CE%BF%CF%82");

    // RFC 2397
    // https://datatracker.ietf.org/doc/html/rfc2397
    // Note: this tokenizer has no special affordances for data URLs.
    good_uri("data:text/html,%3Cscript%3Ealert%28%27hi%27%29%3B%3C%2Fscript%3e",
        "data",
        NULL,
        NULL,
        NULL,
        "text/html,<script>alert('hi');</script>",
        NULL,
        NULL);

    // https://www.erlang.org/doc/apps/stdlib/uri_string_usage
    good_uri("http://%6C%6Fcal%23host/%F6re%26bro%20",
        "http",
        NULL,
        "local#host",
        NULL,
        "/\xF6re&bro ",
        NULL,
        NULL);

    test_modify("./p:",       "o",   "o");
    test_modify("./p:/",      "o",   "./p:/o");
    test_modify("./p",        "o:",  "./o:");
    test_modify("p",          "o",   "o");
    test_modify("p/",         "o",   "p/o");
    test_modify("/p",         "o",   "/o");
    test_modify("/p/",        "o",   "/p/o");
    test_modify("///p",       "o",   "///o");
    test_modify("///p/",      "o",   "///p/o");
    test_modify("s://h",      "p",   NULL);
    test_modify("s://h",      "/p",  "s://h/p");
    test_modify("s://h/p",    NULL,  "s://h");
    test_modify("s://h/p",    "o",   "s://h/o");
    test_modify("s://h/p/q",  "o",   "s://h/p/o");
    test_modify("s://h/p/q",  "/a",  "s://h/a");
    test_modify("s://h/p",    "%",   "s://h/%25");

    // URI re-writing example.
    // https://stackoverflow.com/questions/10161177/url-with-multiple-forward-slashes-does-it-break-anything
    test_modify("http://host/a/b/c/d",   "../../e",  "http://host/a/e");
    test_modify("http://host/a/b/c//d",  "../../e",  "http://host/a/b/e");

    // https://url.spec.whatwg.org

    // backslash
    bad_uri("https:/\\attacker.com");

    // space
    bad_uri("https://attacker.com/bad path");

    // round-trip of scheme:path avoiding issue with double-slash being interpreted as authority
    roundtrip("web+demo:/.//not-a-host/",
        "web+demo",
        NULL,
        NULL,
        NULL,
        "//not-a-host/",
        NULL,
        NULL,
        "web+demo:/.//not-a-host/");

    // The following test cases were taken from a seach of various CVEs.
    // While this library is focused on URI tokenization, it is important to avoid semantic changes when calling uri_str after uri_new.

    // CVE-2022-27780
    roundtrip("http://example.com%2F127.0.0.1", "http", NULL, "example.com/127.0.0.1", NULL, NULL, NULL, NULL, "http://example.com%2F127.0.0.1");

    good_uri("http://example.com%252f127.0.0.1", "http", NULL, "example.com%2f127.0.0.1", NULL, NULL, NULL, NULL);

    // CVE-2022-0691
    bad_uri("\bhttp://google.com");

    // CVE-2022-0686
    roundtrip("http://example.com:", "http", NULL, "example.com", "", NULL, NULL, NULL, "http://example.com");

    // CVE-2022-0639
    roundtrip("http:@/127.0.0.1", "http", NULL, NULL, NULL, "@/127.0.0.1", NULL, NULL, "http:@/127.0.0.1");

    // CVE-2022-0512
    bad_uri("http://admin:password123@@127.0.0.1");
    bad_uri("http://user@@www.example.com/");

    roundtrip("http://user%40@www.example.com", "http", "user@", "www.example.com", NULL, NULL, NULL, NULL, "http://user%40@www.example.com");

    // CVE-2019-14809
    // Note: this tokenizer has no special affordances for Javascript.
    good_uri("javascript://%250aalert(1)+'aa@google.com/a'a",
        "javascript",
        "%0aalert(1)+'aa",
        "google.com",
        NULL,
        "/a'a",
        NULL,
        NULL);
}
