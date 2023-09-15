#include "support.h"
#include "uri.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void test_control_characters(void)
{
    uri_new_must_fail(EINVAL, "http://host\t/path");

    // CVE-2022-0691
    uri_new_must_fail(EINVAL, "\bhttp://google.com");
}

/// Notes:
/// %2E is always decoded (unreserved).
/// %2F is never decoded (reserved).
/// Dot-segment removal operates only on literal '/'.

/// CVE-2019-9740: urllib CRLF injection via percent-encoding
/// https://bugs.python.org/issue36276
/// CVE-2020-27782: Host parsing injection (curl)
/// https://curl.se/docs/vulnerability.html
static void test_crlf_injection(void)
{
    uri_new_must_fail(EINVAL, "scheme:/%0d%0a");
    uri_new_must_fail(EINVAL, "scheme:/?query%0d%0ainjected");
    uri_new_must_fail(EINVAL, "/path?x=%0d%0a");
    uri_new_must_fail(EINVAL, "/path?x=\r\n");
    uri_new_must_fail(EINVAL, "/path?x=%00");

    uri_new_must_fail(EINVAL, "http://host%0D%0A/path");
    uri_new_must_fail(EINVAL, "http://host/path%0D%0A");
    uri_new_must_fail(EINVAL, "http://host?query%0D%0Ainjected");
    uri_new_must_fail(EINVAL, "http://host#frag%0D%0Ainjected");
}

static void test_percent_encoding_confusion(void)
{
    // Uppercase vs lowercase hex (should normalize)
    struct uri *u = uri_new("http://host/%2a");
    uri_str_must_match(u, "http://host/%2A");
    uri_delete(u);

    // Unreserved characters should not be encoded
    u = uri_new("http://host/a-b_c.d~e");
    assert(!strcmp(uri_path(u), "/a-b_c.d~e"));
    uri_str_must_match(u, "http://host/a-b_c.d~e");
    uri_delete(u);

    {
        // CVE-2021-44227: Path normalization bypass (Log4Shell-adjacent)
        // https://nvd.nist.gov/vuln/detail/CVE-2021-44227
        // Double-encoded dots should NOT decode to "..".
        const char *input = "scheme:/%252e%252e/etc/passwd";
        const char *output = input;
        ingest_match_components_output(
                input,
                "scheme", NO_USERINFO, NO_HOST, NO_PORT, "/%252e%252e/etc/passwd", NO_QUERY, NO_FRAGMENT,
                output);
    }

    {
        const char *input = "/%2e%2e/secret";
        const char *output = "/secret";
        ingest_match_components_output(input, NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, output, NO_QUERY, NO_FRAGMENT, output);
    }

    {
        const char *input = "/%2Fetc/passwd";
        const char *output = input;
        ingest_match_components_output(input, NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, output, NO_QUERY, NO_FRAGMENT, output);
    }

    {
        const char *input = "http://example.com%2F@evil.com/";
        const char *output = input;
        ingest_match_components_output(input, "http", "example.com%2F", "evil.com", NO_PORT, "/", NO_QUERY, NO_FRAGMENT, output);
    }

    {
        // CVE-2022-27780
        const char *input = "http://example.com%2F127.0.0.1";
        const char *output = input;
        ingest_match_components_output(
                input,
                "http", NO_USERINFO, "example.com%2F127.0.0.1", NO_PORT, NO_PATH, NO_QUERY, NO_FRAGMENT,
                output);
    }

    {
        const char *input = "http://example.com%252f127.0.0.1";
        const char *output = input;
        ingest_match_components_output(
                input,
                "http", NO_USERINFO, "example.com%252f127.0.0.1", NO_PORT, NO_PATH, NO_QUERY, NO_FRAGMENT,
                output);
    }
}

static void test_dot_segment_traversal(void)
{
    {
        const char *input = "/a/b/../../c";
        const char *output = "/c";
        ingest_match_components_output(input, NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, output, NO_QUERY, NO_FRAGMENT, output);
    }

    {
        const char *input = "a/b/../../c";
        const char *output = "c";
        ingest_match_components_output(input, NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, output, NO_QUERY, NO_FRAGMENT, output);
    }

    {
        const char *input = "/a/b/%2e%2e/%2e%2e/c";
        const char *output = "/c";
        ingest_match_components_output(input, NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, output, NO_QUERY, NO_FRAGMENT, output);
    }

    {
        const char *input = "/a/b/%2e%2e%2f../c";
        const char *output = "/a/b/..%2F../c";
        ingest_match_components_output(input, NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, output, NO_QUERY, NO_FRAGMENT, output);
    }
}

static void test_authority_path_smuggling(void)
{
    uri_new_must_fail(EINVAL, "http:/\\evil.com/");

    // userinfo => attacker@
    // host     => legitimate.com@victim.com
    //
    // The host has an invalid character "@".
    uri_new_must_fail(EINVAL, "http://attacker@legitimate.com@victim.com/phishing");

    {
        const char *input = "http://user%40@www.example.com";
        const char *output = input;
        ingest_match_components_output(
                input,
                "http", "user%40", "www.example.com", NO_PORT, NO_PATH, NO_QUERY, NO_FRAGMENT,
                output);
    }

    // CVE-2022-0512
    uri_new_must_fail(EINVAL, "http://admin:password123@@127.0.0.1");
    uri_new_must_fail(EINVAL, "http://user@@www.example.com/");

    {
        // CVE-2022-0639
        const char *input = "http:@/127.0.0.1";
        const char *output = input;
        ingest_match_components_output(
                input,
                "http", NO_USERINFO, NO_HOST, NO_PORT, "@/127.0.0.1", NO_QUERY, NO_FRAGMENT,
                output);
    }

    {
        // Recipients of http URIs with an empty host should reject them as invalid to prevent "host-header injection" or redirection attacks.
        // (This is beyond the scope of liburi.)
        const char *input = "http:/@evil.com/";
        const char *output = input;
        ingest_match_components_output(input, "http", NO_USERINFO, NO_HOST, NO_PORT, "/@evil.com/", NO_QUERY, NO_FRAGMENT, output);
    }

    {
        const char *input = "http:////evil.com/";
        const char *output = input;
        ingest_match_components_output(input, "http", NO_USERINFO, "", NO_PORT, "//evil.com/", NO_QUERY, NO_FRAGMENT, output);
    }
}

static void test_utf8_overlong_sequences(void)
{
    // overlong '.'
    uri_new_must_fail(EILSEQ, "\xC0\xAE");

    // overlong '/'
    uri_new_must_fail(EILSEQ, "\xE0\x80\xAF");

    // UTF-16 surrogate
    uri_new_must_fail(EILSEQ, "\xED\xA0\x80");
}

static void test_nul_injection(void)
{
    uri_new_must_fail(EINVAL, "http://example.com%00.evil.com/");
}

static void test_host_case_normalization(void)
{
    {
        const char *input = "HTTP://EXAMPLE.COM";
        const char *output = "http://example.com";
        ingest_match_components_output(input, "http", NO_USERINFO, "example.com", NO_PORT, NO_PATH, NO_QUERY, NO_FRAGMENT, output);
    }
}

static void test_port_normalization(void)
{
    {
        const char *input = "http://example.com:00080";
        const char *output = "http://example.com:80";
        ingest_match_components_output(input, "http", NO_USERINFO, "example.com", "80", NO_PATH, NO_QUERY, NO_FRAGMENT, output);
    }

    uri_new_must_fail(EINVAL, "http://example.com:-1");
    uri_new_must_fail(ERANGE, "http://example.com:65536");

    {
        // CVE-2022-0686
        const char *input = "http://example.com:";
        const char *output = input;
        ingest_match_components_output(
                input,
                "http", NO_USERINFO, "example.com", "", NO_PATH, NO_QUERY, NO_FRAGMENT,
                output);
    }
}


static void test_path_query_smuggling(void)
{
    {
        const char *input = "/path%3Fsecret";
        const char *output = input;
        ingest_match_components_output(input, NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, output, NO_QUERY, NO_FRAGMENT, output);
    }

    {
        const char *input = "/path%23frag";
        const char *output = input;
        ingest_match_components_output(input, NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, output, NO_QUERY, NO_FRAGMENT, output);
    }
}

static void test_double_normalization(void)
{
    const char *input = "/a/b/../c/%7euser";
    struct uri *u1 = uri_new(input);
    char *s1 = uri_str(u1);
    struct uri *u2 = uri_new(s1);
    char *s2 = uri_str(u2);
    assert(!strcmp(s1, s2));
    free(s1);
    free(s2);
    uri_delete(u1);
    uri_delete(u2);
}

#define CYRILLIC_SMALL_LETTER_IE "\xd0\xb5" // U+0435

static void test_mixed_scripts(void)
{
    // Looks like 'e', but has a different code point.
    struct uri *u = uri_new("http://" CYRILLIC_SMALL_LETTER_IE "xample.com/path");
    uri_str_must_match(u, "http://%D0%B5xample.com/path");
    uri_delete(u);

    u = uri_new("http:///" CYRILLIC_SMALL_LETTER_IE "arth");
    uri_str_must_match(u, "http:///%D0%B5arth");
    uri_delete(u);
}

static void test_javascript(void)
{
    // CVE-2019-14809
    // Note: liburi has no special affordances for Javascript.
    ingest_match_components("javascript://%250aalert(1)+'aa@google.com/a'a",
        "javascript",
        "%250aalert(1)+'aa",
        "google.com",
        NO_PORT,
        "/a'a",
        NO_QUERY,
        NO_FRAGMENT);
}

int main(void)
{
    test_control_characters();
    test_crlf_injection();
    test_percent_encoding_confusion();
    test_dot_segment_traversal();
    test_authority_path_smuggling();
    test_utf8_overlong_sequences();
    test_nul_injection();
    test_host_case_normalization();
    test_port_normalization();
    test_path_query_smuggling();
    test_double_normalization();
    test_mixed_scripts();
    test_javascript();

    printf("Testing completed.\n");
    return 0;
}
