#pragma once

// Private API.
// Test support.

struct uri;

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

/// Assert that @c errno equals @c err.
void assert_api_error(int err, const char *scenario, const char *input);

/// Assert that @c expected matches @c actual.
void string_must_match(const char *scenario, const char *component, const char *expected, const char *actual);

/// Assert that @c uri_new fails with @c err given @c input.
void uri_new_must_fail(int err, const char *input);

/// Assert that @c uri_str produces @c expected output.
void uri_str_must_match(const struct uri *u, const char *expected);

/// To improve readability of test cases.
#define NO_SCHEME NULL
#define NO_USERINFO NULL
#define NO_HOST NULL
#define NO_PORT NULL
#define NO_PATH NULL
#define NO_QUERY NULL
#define NO_FRAGMENT NULL

/// Assert that URI components match the given strings.
void uri_components_must_match(const struct uri *u, const char *scheme, const char *userinfo, const char *host, const char *port, const char *path, const char *query, const char *fragment);

/// Assert that @c input URI is successfully parsed into then given components.
/// @see uri_new.
void ingest_match_components(const char *input, const char *scheme, const char *userinfo, const char *host, const char *port, const char *path, const char *query, const char *fragment);

/// Assert that @c input URI is successfully parsed into then given components, and that the output URI matches @c expected.
/// @see ingest_match_components.
/// @see uri_str.
void ingest_match_components_output(const char *input, const char *scheme, const char *userinfo, const char *host, const char *port, const char *path, const char *query, const char *fragment, const char *expected);

/// Assert getter @c api returns @c expected string.
void getter_must_match(const char *scenario, const char *component, const char * (*api)(const struct uri *), const struct uri *u, const char *expected);

/// Assert that setter @c api fails with @c err given @c input.
void setter_must_fail(const char *scenario, int (*api)(struct uri *, const char *), int err, struct uri *u, const char *input);

/// Assert that URI set operation fails.
/// @see uri_set.
void set_must_fail(int err, struct uri *u, const char *input);

/// Assert that scheme set operation fails.
/// @see uri_scheme_set.
void scheme_set_must_fail(int err, struct uri *u, const char *input);

/// Assert that userinfo set operation fails.
/// @see uri_userinfo_set.
void userinfo_set_must_fail(int err, struct uri *u, const char *input);

/// Assert that host set operation fails.
/// @see uri_host_set.
void host_set_must_fail(int err, struct uri *u, const char *input);

/// Assert that port set operation fails.
/// @see uri_port_set.
void port_set_must_fail(int err, struct uri *u, const char *input);

/// Assert that path set operation fails.
/// @see uri_path_set.
void path_set_must_fail(int err, struct uri *u, const char *input);

/// Assert that query set operation fails.
/// @see uri_query_set.
void query_set_must_fail(int err, struct uri *u, const char *input);

/// Assert that fragment set operation fails.
/// @see uri_fragment_set.
void fragment_set_must_fail(int err, struct uri *u, const char *input);

/// Assert that changing the @c uri with string @c change succeeds, and that the output URI matches @c expected.
/// @see uri_set.
void uri_change_must_match(const char *uri, const char *change, const char *expected);

/// Assert that changing path of @c uri to @c path_in results in path component @c path_out and that the output URI matches @c expected.
/// @see uri_path_set.
void construct_set_path_expect(const char *uri, const char *path_in, const char *path_out, const char *expected);
