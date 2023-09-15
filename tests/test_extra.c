#include "memory_shim.h"
#include "support.h"
#include "uri.h"

#include <errno.h>
#include <stdio.h>

#define LATIN_SMALL_LETTER_U_WITH_DIAERESIS "\xc3\xbc" // U+00FC
#define PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS "%C3%BC" // U+00FC

static void test_uri_instantiation(void)
{
    ingest_match_components_output("http://host/%2fetc/passwd",
            "http", NO_USERINFO, "host", NO_PORT, "/%2Fetc/passwd", NO_QUERY, NO_FRAGMENT,
            "http://host/%2Fetc/passwd");

    ingest_match_components("path",                   NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "path",      NO_QUERY, NO_FRAGMENT);
    ingest_match_components("./path:",                NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "path:",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/path",                  NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/path",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("path//to",               NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "path//to",  NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/path//to",              NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/path//to", NO_QUERY, NO_FRAGMENT);
    ingest_match_components("//host",                 NO_SCHEME, NO_USERINFO, "host",  NO_PORT, NO_PATH,     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("//host:",                NO_SCHEME, NO_USERINFO, "host",  "",      NO_PATH,     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("//host:123",             NO_SCHEME, NO_USERINFO, "host",  "123",   NO_PATH,     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme:",                "scheme",  NO_USERINFO, NO_HOST, NO_PORT, NO_PATH,     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme:path",            "scheme",  NO_USERINFO, NO_HOST, NO_PORT, "path",      NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme:/path",           "scheme",  NO_USERINFO, NO_HOST, NO_PORT, "/path",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme://host",          "scheme",  NO_USERINFO, "host",  NO_PORT, NO_PATH,     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme://host:123",      "scheme",  NO_USERINFO, "host",  "123",   NO_PATH,     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme://host:123/path", "scheme",  NO_USERINFO, "host",  "123",   "/path",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme://host/path",     "scheme",  NO_USERINFO, "host",  NO_PORT, "/path",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme://user@:/path",   "scheme",  "user",      "",      "",      "/path",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme://user@:4/path",  "scheme",  "user",      "",      "4",     "/path",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("scheme://@:/?q#f",       "scheme",  "",          "",      "",      "/",         "q",      "f");
    ingest_match_components("//@:/?q#f",              NO_SCHEME, "",          "",      "",      "/",         "q",      "f");

    ingest_match_components("scheme://user:pass:word@host123:456/path/to?query?q#fragment?q",
        "scheme",
        "user:pass:word",
        "host123",
        "456",
        "/path/to",
        "query?q",
        "fragment?q");

    ingest_match_components("?qu://ery", NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, NO_PATH, "qu://ery", NO_FRAGMENT);

    // Unclosed host IPv6 address.
    uri_new_must_fail(EINVAL, "http://[2001:db8:85a3:8d3:1319:8a2e:370:7348/path");

    ingest_match_components("http://[2001:db8:85a3:8d3:1319:8a2e:370:7348]:443",
        "http",
        NO_USERINFO,
        "[2001:db8:85a3:8d3:1319:8a2e:370:7348]",
        "443",
        NO_PATH,
        NO_QUERY,
        NO_FRAGMENT);

    // §2.3 - percent-encoding normalization
    // §3.1 - scheme lowercase
    // §3.1 - percent-encoding uppercase
    // §3.2.2 - host lowercase
    // port - dropping leading zeroes
    // UTF-8 translated to percent-encoding
    ingest_match_components_output(
            "scHEme://user:M" LATIN_SMALL_LETTER_U_WITH_DIAERESIS "nchen@hoST:00123/path/%41%2d%5a%2e%61%2d%7a%5f%30%7e%39:%3f?query#fragment",
            "scheme",
            "user:M" PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS "nchen",
            "host",
            "123",
            "/path/A-Z.a-z_0~9:%3F",
            "query",
            "fragment",
            "scheme://user:M" PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS "nchen@host:123/path/A-Z.a-z_0~9:%3F?query#fragment");

    ingest_match_components_output(
            "scHEme://user:M" PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS "nchen@hoST:00123/path/%41%2d%5a%2e%61%2d%7a%5f%30%7e%39:%3f?query#fragment",
            "scheme",
            "user:M" PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS "nchen",
            "host",
            "123",
            "/path/A-Z.a-z_0~9:%3F",
            "query",
            "fragment",
            "scheme://user:M" PE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS "nchen@host:123/path/A-Z.a-z_0~9:%3F?query#fragment");

    // RFC 3987 — IRI
    // An IRI is a Unicode-based identifier that can be losslessly represented as a URI through defined encoding rules.
    //  * scheme   - same for IRI and URI
    //  * userinfo - unicode IRI, percent encoded URI
    //  * host     - unicode IRI, IDNA (punycode) URI
    //  * port     - same for IRI and URI
    //  * path     - unicode IRI, percent encoded URI
    //  * query    - unicode IRI, percent encoded URI
    //  * fragment - unicode IRI, percent encoded URI
    // https://en.wikipedia.org/wiki/Internationalized_Resource_Identifier

    ingest_match_components_output(
            "https://en.wiktionary.org/hiki/../wiki/%E1%BF%AC%CF%8C%CE%B4%CE%BF%cf%82",
            "https",
            NO_USERINFO,
            "en.wiktionary.org",
            NO_PORT,
            "/wiki/%E1%BF%AC%CF%8C%CE%B4%CE%BF%CF%82",
            NO_QUERY,
            NO_FRAGMENT,
            "https://en.wiktionary.org/wiki/%E1%BF%AC%CF%8C%CE%B4%CE%BF%CF%82");

    ingest_match_components_output(
            "https://en.wiktionary.org/wiki/Ῥόδος",
            "https",
            NO_USERINFO,
            "en.wiktionary.org",
            NO_PORT,
            "/wiki/%E1%BF%AC%CF%8C%CE%B4%CE%BF%CF%82",
            NO_QUERY,
            NO_FRAGMENT,
            "https://en.wiktionary.org/wiki/%E1%BF%AC%CF%8C%CE%B4%CE%BF%CF%82");

    ingest_match_components_output(
            "./file:",
            NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "file:", NO_QUERY, NO_FRAGMENT,
            "./file:");

    ingest_match_components_output(
            "/.//file",
            NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "//file", NO_QUERY, NO_FRAGMENT,
            "/.//file");

    // §7.3
    uri_new_must_fail(EINVAL, "scheme:%00");

    uri_new_must_fail(EINVAL, "scheme:/[]");
    ingest_match_components(
            "scheme:/%5B%5D/", "scheme", NO_USERINFO, NO_HOST, NO_PORT, "/%5B%5D/", NO_QUERY, NO_FRAGMENT);
    ingest_match_components(
            "scheme://[]/",    "scheme", NO_USERINFO, "[]",    NO_PORT, "/",        NO_QUERY, NO_FRAGMENT);

    ingest_match_components("SCheME://user%40@HoSt%2D%2d:123/%7e/path:1%2f%2F//@2/!$&'()*+,;=/more?qu%23ery%7e#%7efragment%7e",
        "scheme",
        "user%40",
        "host--",
        "123",
        "/~/path:1%2F%2F//@2/!$&'()*+,;=/more",
        "qu%23ery~",
        "~fragment~");

    // §4.2
    // A path segment that contains a colon character (e.g., "this:that") cannot be used as the first segment of a relative-path reference, as it would be mistaken for a scheme name.

    ingest_match_components_output("this:that",        "this",    NO_USERINFO, NO_HOST, NO_PORT, "that",      NO_QUERY, NO_FRAGMENT, "this:that");
    ingest_match_components_output("./this:that",      NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "this:that", NO_QUERY, NO_FRAGMENT, "./this:that");
    ingest_match_components_output("scheme:this:that", "scheme",  NO_USERINFO, NO_HOST, NO_PORT, "this:that", NO_QUERY, NO_FRAGMENT, "scheme:this:that");

    // §6.2.2

    ingest_match_components("eXAMPLE://a/./b/../b/%63/%7bfoo%7d",
        "example",
        NO_USERINFO,
        "a",
        NO_PORT,
        "/b/c/%7Bfoo%7D",
        NO_QUERY,
        NO_FRAGMENT);

    // RFC 2397
    // https://datatracker.ietf.org/doc/html/rfc2397
    // Note: this tokenizer has no special affordances for data URLs.
    ingest_match_components("data:text/html,%3Cscript%3Ealert%28%27hi%27%29%3B%3C%2Fscript%3e",
        "data",
        NO_USERINFO,
        NO_HOST,
        NO_PORT,
        "text/html,%3Cscript%3Ealert%28%27hi%27%29%3B%3C%2Fscript%3E",
        NO_QUERY,
        NO_FRAGMENT);

    // https://www.erlang.org/doc/apps/stdlib/uri_string_usage
    ingest_match_components_output("http://%6C%6Fcal%23host/%F6re%26bro%20",
        "http",
        NO_USERINFO,
        "local%23host",
        NO_PORT,
        "/%F6re%26bro%20",
        NO_QUERY,
        NO_FRAGMENT,
        "http://local%23host/%F6re%26bro%20");

    // https://url.spec.whatwg.org

    // backslash
    uri_new_must_fail(EINVAL, "https:/\\attacker.com");

    // space
    uri_new_must_fail(EINVAL, "https://attacker.com/bad path");

    // round-trip of scheme:path avoiding issue with double-slash being interpreted as authority
    ingest_match_components_output("web+demo:/.//not-a-host/",
        "web+demo",
        NO_USERINFO,
        NO_HOST,
        NO_PORT,
        "//not-a-host/",
        NO_QUERY,
        NO_FRAGMENT,
        "web+demo:/.//not-a-host/");
}

// §5.4
static void test_reference_resolution(void)
{
    // remove_dot_segments algorithm testing
    ingest_match_components("a/..",         NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "",       NO_QUERY, NO_FRAGMENT);
    ingest_match_components("a/../",        NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "",       NO_QUERY, NO_FRAGMENT);
    ingest_match_components("a/../b",       NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "b",      NO_QUERY, NO_FRAGMENT);
    ingest_match_components("./b",          NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "b",      NO_QUERY, NO_FRAGMENT);
    ingest_match_components("./",           NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "",       NO_QUERY, NO_FRAGMENT);
    ingest_match_components(".",            NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "",       NO_QUERY, NO_FRAGMENT);
    ingest_match_components("..",           NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "",       NO_QUERY, NO_FRAGMENT);
    ingest_match_components("../b",         NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "b",      NO_QUERY, NO_FRAGMENT);
    ingest_match_components("a/b/..",       NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "a/",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("a/b/../",      NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "a/",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("a/b/../c",     NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "a/c",    NO_QUERY, NO_FRAGMENT);

    ingest_match_components("/a/..",        NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/",      NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/a/../",       NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/",      NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/a/../b",      NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/b",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/./b",         NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/b",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/../b",        NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/b",     NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/../",         NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/",      NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/a/b/../../c", NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/c",     NO_QUERY, NO_FRAGMENT);

    ingest_match_components("",             NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, NO_PATH,  NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/",            NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/",      NO_QUERY, NO_FRAGMENT);
    ingest_match_components("////a///b/..", NO_SCHEME, NO_USERINFO, "",      NO_PORT, "//a///", NO_QUERY, NO_FRAGMENT);
    ingest_match_components("/a/././b/.",   NO_SCHEME, NO_USERINFO, NO_HOST, NO_PORT, "/a/b/",  NO_QUERY, NO_FRAGMENT);

    // §5.4.1
    uri_change_must_match("http://a/b/c/d;p?q", "g:h"           , "g:h");
    uri_change_must_match("http://a/b/c/d;p?q", "g"             , "http://a/b/c/g");
    uri_change_must_match("http://a/b/c/d;p?q", "./g"           , "http://a/b/c/g");
    uri_change_must_match("http://a/b/c/d;p?q", "g/"            , "http://a/b/c/g/");
    uri_change_must_match("http://a/b/c/d;p?q", "/g"            , "http://a/g");
    uri_change_must_match("http://a/b/c/d;p?q", "//g"           , "http://g");
    uri_change_must_match("http://a/b/c/d;p?q", "?y"            , "http://a/b/c/d;p?y");
    uri_change_must_match("http://a/b/c/d;p?q", "g?y"           , "http://a/b/c/g?y");
    uri_change_must_match("http://a/b/c/d;p?q", "#s"            , "http://a/b/c/d;p?q#s");
    uri_change_must_match("http://a/b/c/d;p?q", "g#s"           , "http://a/b/c/g#s");
    uri_change_must_match("http://a/b/c/d;p?q", "g?y#s"         , "http://a/b/c/g?y#s");
    uri_change_must_match("http://a/b/c/d;p?q", ";x"            , "http://a/b/c/;x");
    uri_change_must_match("http://a/b/c/d;p?q", "g;x"           , "http://a/b/c/g;x");
    uri_change_must_match("http://a/b/c/d;p?q", "g;x?y#s"       , "http://a/b/c/g;x?y#s");
    uri_change_must_match("http://a/b/c/d;p?q", ""              , "http://a/b/c/d;p?q");
    uri_change_must_match("http://a/b/c/d;p?q", "."             , "http://a/b/c/");
    uri_change_must_match("http://a/b/c/d;p?q", "./"            , "http://a/b/c/");
    uri_change_must_match("http://a/b/c/d;p?q", ".."            , "http://a/b/");
    uri_change_must_match("http://a/b/c/d;p?q", "../"           , "http://a/b/");
    uri_change_must_match("http://a/b/c/d;p?q", "../g"          , "http://a/b/g");
    uri_change_must_match("http://a/b/c/d;p?q", "../.."         , "http://a/");
    uri_change_must_match("http://a/b/c/d;p?q", "../../"        , "http://a/");
    uri_change_must_match("http://a/b/c/d;p?q", "../../g"       , "http://a/g");

    // §5.4.2
    uri_change_must_match("http://a/b/c/d;p?q", "../../../g"    , "http://a/g");
    uri_change_must_match("http://a/b/c/d;p?q", "../../../../g" , "http://a/g");
    uri_change_must_match("http://a/b/c/d;p?q", "/./g"          , "http://a/g");
    uri_change_must_match("http://a/b/c/d;p?q", "/../g"         , "http://a/g");
    uri_change_must_match("http://a/b/c/d;p?q", "g."            , "http://a/b/c/g.");
    uri_change_must_match("http://a/b/c/d;p?q", ".g"            , "http://a/b/c/.g");
    uri_change_must_match("http://a/b/c/d;p?q", "g.."           , "http://a/b/c/g..");
    uri_change_must_match("http://a/b/c/d;p?q", "..g"           , "http://a/b/c/..g");
    uri_change_must_match("http://a/b/c/d;p?q", "./../g"        , "http://a/b/g");
    uri_change_must_match("http://a/b/c/d;p?q", "./g/."         , "http://a/b/c/g/");
    uri_change_must_match("http://a/b/c/d;p?q", "g/./h"         , "http://a/b/c/g/h");
    uri_change_must_match("http://a/b/c/d;p?q", "g/../h"        , "http://a/b/c/h");
    uri_change_must_match("http://a/b/c/d;p?q", "g;x=1/./y"     , "http://a/b/c/g;x=1/y");
    uri_change_must_match("http://a/b/c/d;p?q", "g;x=1/../y"    , "http://a/b/c/y");
    uri_change_must_match("http://a/b/c/d;p?q", "g?y/./x"       , "http://a/b/c/g?y/./x");
    uri_change_must_match("http://a/b/c/d;p?q", "g?y/../x"      , "http://a/b/c/g?y/../x");
    uri_change_must_match("http://a/b/c/d;p?q", "g#s/./x"       , "http://a/b/c/g#s/./x");
    uri_change_must_match("http://a/b/c/d;p?q", "g#s/../x"      , "http://a/b/c/g#s/../x");
    uri_change_must_match("http://a/b/c/d;p?q", "http:g"        , "http:g");

    // Additions.
    ingest_match_components("http://a/b/c/g?y/./x",  "http", NO_USERINFO, "a", NO_PORT, "/b/c/g", "y/./x",  NO_FRAGMENT);
    ingest_match_components("http://a/b/c/g?y/../x", "http", NO_USERINFO, "a", NO_PORT, "/b/c/g", "y/../x", NO_FRAGMENT);
    ingest_match_components("http://a/b/c/g#s/./x",  "http", NO_USERINFO, "a", NO_PORT, "/b/c/g", NO_QUERY, "s/./x");
    ingest_match_components("http://a/b/c/g#s/../x", "http", NO_USERINFO, "a", NO_PORT, "/b/c/g", NO_QUERY, "s/../x");

    // URI re-writing example.
    // https://stackoverflow.com/questions/10161177/url-with-multiple-forward-slashes-does-it-break-anything
    uri_change_must_match("http://host/a/b/c/d",  "../../e", "http://host/a/e");
    uri_change_must_match("http://host/a/b/c//d", "../../e", "http://host/a/b/e");
}

int main(void)
{
    test_uri_instantiation();
    test_reference_resolution();

    printf("Testing completed.\n");
    return 0;
}
