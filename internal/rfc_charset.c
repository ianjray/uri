#include "rfc_charset.h"

bool is_alpha(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool is_digit(unsigned char c)
{
    return (c >= '0' && c <= '9');
}

/* unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~" */
bool is_unreserved(unsigned char c)
{
    return is_alpha(c) || is_digit(c) || c == '-' || c == '.' || c == '_' || c == '~';
}

/* sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "=" */
static bool is_sub_delim(unsigned char c)
{
    switch (c) {
        case '!':
        case '$':
        case '&':
        case '\'':
        case '(':
        case ')':
        case '*':
        case '+':
        case ',':
        case ';':
        case '=':
            return true;
        default:
            return false;
    }
}

static bool is_pct_encoded(unsigned char c)
{
    return c == '%';
}

/* pchar = unreserved / pct-encoded / sub-delims / ":" / "@" */
static bool uri_is_pchar(unsigned char c)
{
    return is_unreserved(c) || is_pct_encoded(c) || is_sub_delim(c) || c == ':' || c == '@';
}

/* path = *( pchar / "/" ) */
bool is_path(unsigned char c)
{
    return uri_is_pchar(c) || c == '/';
}

/* scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) */
bool is_scheme(unsigned char c)
{
    return is_alpha(c) || is_digit(c) || c == '+' || c == '-' || c == '.';
}

/* userinfo = *( unreserved / pct-encoded / sub-delims / ":" ) */
bool is_userinfo(unsigned char c)
{
    return is_unreserved(c) || is_pct_encoded(c) || is_sub_delim(c) || c == ':';
}

static bool is_ip_literal(unsigned char c)
{
    /* All other character such as ASCIIHEX handled elsewhere. */
    return c == '[' || c == ':' || c == ']';
}

bool is_host(unsigned char c)
{
    // host          = IP-literal / IPv4address / reg-name
    // IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"
    // IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )

    // IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
    // reg-name      = *( unreserved / pct-encoded / sub-delims )
    return is_unreserved(c) || is_pct_encoded(c) || is_sub_delim(c) || is_ip_literal(c);
}

/* port = *digit */
bool is_port(unsigned char c)
{
    return is_digit(c);
}

/* query = *( pchar / "/" / "?" ) */
bool is_query(unsigned char c)
{
    return is_path(c) || c == '?';
}

/* fragment = *( pchar / "/" / "?" ) */
bool is_fragment(unsigned char c)
{
    return is_path(c) || c == '?';
}
