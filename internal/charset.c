#include "charset.h"

#include <ctype.h>
#include <string.h>

bool is_alpha(int c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

bool is_unreserved(int c)
{
    // unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
    return is_alpha(c) || isdigit(c) || (c && strchr("-._~", c));
}

bool is_scheme(int c)
{
    // scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    return is_alpha(c) || isdigit(c) || (c && strchr("+-.", c));
}

bool is_port(int c)
{
    // port          = *DIGIT
    return isdigit(c);
}

bool is_userinfo(int c)
{
    // userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
    return is_unreserved(c) || (c && strchr("%!$&'()*+,;=:", c));
}

bool is_host(int c)
{
    // host          = IP-literal / IPv4address / reg-name
    // IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"
    // IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    // reg-name      = *( unreserved / pct-encoded / sub-delims )
    return is_unreserved(c) || (c && strchr("%[]!$&'()*+,;=:", c));
}

bool is_path(int c)
{
    // pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
    // plus "/"
    return is_unreserved(c) || (c && strchr("%!$&'()*+,;=:@/", c));
}

bool is_query(int c)
{
    // query         = *( pchar / "/" / "?" )
    // fragment      = *( pchar / "/" / "?" )
    return is_path(c) || c == '?';
}
