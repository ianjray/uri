#include "support.h"
#include "uri.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void assert_api_error(int err, const char *scenario, const char *input)
{
    int r = err == errno;
    if (!r) {
        printf("FAIL: %s: %s; expected %d (%s), actual %d (%s)\n", scenario, input, err, strerror(err), errno, strerror(errno));
        assert(r);
    }
}

void string_must_match(const char *scenario, const char *component, const char *expected, const char *actual)
{
    if (expected) {
        if (actual) {
            int r = strcmp(expected, actual);
            if (r) {
                printf("FAIL: %s: %s; expected (%s), actual (%s)\n", scenario, component, expected, actual);
                assert(!r);
            }
        } else {
            printf("FAIL: %s: %s; expected (%s), actual (%s)\n", scenario, component, expected, "null");
            assert(actual);
        }
    } else {
        if (actual) {
            printf("FAIL: %s: %s; expected (%s), actual (%s)\n", scenario, component, "null", actual);
            assert(!actual);
        }
    }
}

void uri_new_must_fail(int err, const char *input)
{
    errno = 0;
    struct uri *u = uri_new(input);
    if (u) {
        printf("FAIL: %s: %s; succeeded (%s)\n", __FUNCTION__, input, uri_str(u));
        uri_delete(u);
        assert(0);
    }
    assert_api_error(err, __FUNCTION__, input);
}

void uri_str_must_match(const struct uri *u, const char *expected)
{
    char *actual = uri_str(u);
    assert(actual);
    string_must_match(__FUNCTION__, "str", expected, actual);
    free(actual);
}

void uri_components_must_match(const struct uri *u, const char *scheme, const char *userinfo, const char *host, const char *port, const char *path, const char *query, const char *fragment)
{
    assert(u);
    getter_must_match(__FUNCTION__, "scheme", uri_scheme, u, scheme);
    getter_must_match(__FUNCTION__, "userinfo", uri_userinfo, u, userinfo);
    getter_must_match(__FUNCTION__, "host", uri_host, u, host);
    getter_must_match(__FUNCTION__, "port", uri_port, u, port);
    getter_must_match(__FUNCTION__, "path", uri_path, u, path);
    getter_must_match(__FUNCTION__, "query", uri_query, u, query);
    getter_must_match(__FUNCTION__, "fragment", uri_fragment, u, fragment);
}

void ingest_match_components(const char *input, const char *scheme, const char *userinfo, const char *host, const char *port, const char *path, const char *query, const char *fragment)
{
    struct uri *u = uri_new(input);
    if (!u) {
        printf("FAIL: %s: %s\n", __FUNCTION__, input);
    }
    uri_components_must_match(u, scheme, userinfo, host, port, path, query, fragment);
    uri_delete(u);
}

void ingest_match_components_output(const char *input, const char *scheme, const char *userinfo, const char *host, const char *port, const char *path, const char *query, const char *fragment, const char *expected)
{
    struct uri *u = uri_new(input);
    if (!u) {
        printf("FAIL: %s: %s\n", __FUNCTION__, input);
    }
    uri_components_must_match(u, scheme, userinfo, host, port, path, query, fragment);
    uri_str_must_match(u, expected);
    uri_delete(u);
}

void getter_must_match(const char *scenario, const char *component, const char * (*api)(const struct uri *), const struct uri *u, const char *expected)
{
    const char *output = api(u);
    string_must_match(scenario, component, expected, output);
}

void setter_must_fail(const char *scenario, int (*api)(struct uri *, const char *), int err, struct uri *u, const char *input)
{
    errno = 0;
    assert(-1 == api(u, input));
    assert_api_error(err, scenario, input);
}

void set_must_fail(int err, struct uri *u, const char *input)
{
    setter_must_fail(__FUNCTION__, uri_set, err, u, input);
}

void scheme_set_must_fail(int err, struct uri *u, const char *input)
{
    setter_must_fail(__FUNCTION__, uri_scheme_set, err, u, input);
}

void userinfo_set_must_fail(int err, struct uri *u, const char *input)
{
    setter_must_fail(__FUNCTION__, uri_userinfo_set, err, u, input);
}

void host_set_must_fail(int err, struct uri *u, const char *input)
{
    setter_must_fail(__FUNCTION__, uri_host_set, err, u, input);
}

void port_set_must_fail(int err, struct uri *u, const char *input)
{
    setter_must_fail(__FUNCTION__, uri_port_set, err, u, input);
}

void path_set_must_fail(int err, struct uri *u, const char *input)
{
    setter_must_fail(__FUNCTION__, uri_path_set, err, u, input);
}

void query_set_must_fail(int err, struct uri *u, const char *input)
{
    setter_must_fail(__FUNCTION__, uri_query_set, err, u, input);
}

void fragment_set_must_fail(int err, struct uri *u, const char *input)
{
    setter_must_fail(__FUNCTION__, uri_fragment_set, err, u, input);
}

void uri_change_must_match(const char *uri, const char *change, const char *expected)
{
    struct uri *u = uri_new(uri);
    assert(u);
    assert(0 == uri_set(u, change));
    uri_str_must_match(u, expected);
    uri_delete(u);
}

void construct_set_path_expect(const char *uri, const char *path_in, const char *path_out, const char *expected)
{
    struct uri *u = uri_new(uri);
    assert(u);
    assert(0 == uri_path_set(u, path_in));
    string_must_match(__FUNCTION__, uri, uri_path(u), path_out);
    uri_str_must_match(u, expected);
    uri_delete(u);
}
