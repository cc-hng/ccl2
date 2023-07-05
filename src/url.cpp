#include "ccl2/url.h"
#include <map>
#include <stdint.h>
#include <string.h>

namespace ccl2 {

const int url_t::nport = -1;

static uint8_t url_hex_val(char c) {
    if ((c >= '0') && (c <= '9')) {
        return (c - '0');
    }
    if ((c >= 'A') && (c <= 'F')) {
        return ((c - 'A') + 10);
    }
    if ((c >= 'a') && (c <= 'f')) {
        return ((c - 'a') + 10);
    }
    return (0);
}

// This returns either 0, or NNG_EINVAL, if the supplied input string
// is malformed UTF-8.  We consider UTF-8 malformed when the sequence
// is an invalid code point, not the shortest possible code point, or
// incomplete.
static int url_utf8_validate(void* arg) {
    uint8_t* s = (uint8_t*)arg;
    uint32_t v, minv;
    int nb;

    while (*s) {
        if ((s[0] & 0x80u) == 0) {
            s++;
            continue;
        }
        if ((s[0] & 0xe0u) == 0xc0) {
            // 0x80 thru 0x7ff
            v    = (s[0] & 0x1fu);
            minv = 0x80;
            nb   = 1;
        } else if ((s[0] & 0xf0u) == 0xe0) {
            v    = (s[0] & 0xfu);
            minv = 0x800;
            nb   = 2;
        } else if ((s[0] & 0xf8u) == 0xf0) {
            v    = (s[0] & 0x7u);
            minv = 0x10000;
            nb   = 3;
        } else {
            // invalid byte, either continuation, or too many
            // leading 1 bits.
            return -1;
        }
        s++;
        for (int i = 0; i < nb; i++) {
            if ((s[0] & 0xc0u) != 0x80) {
                return -1;  // not continuation
            }
            s++;
            v <<= 6u;
            v += s[0] & 0x3fu;
        }
        if (v < minv) {
            return -1;
        }
        if ((v >= 0xd800) && (v <= 0xdfff)) {
            return -1;
        }
        if (v > 0x10ffff) {
            return -1;
        }
    }
    return (0);
}

size_t nni_url_decode(uint8_t* out, const char* in, size_t max_len) {
    size_t len;
    uint8_t c;

    len = 0;
    while ((c = (uint8_t)*in) != '\0') {
        if (len >= max_len) {
            return ((size_t)-1);
        }
        if (c == '%') {
            in++;
            if ((!isxdigit(in[0])) || (!isxdigit(in[1]))) {
                return ((size_t)-1);
            }
            out[len] = url_hex_val(*in++);
            out[len] <<= 4u;
            out[len] += url_hex_val(*in++);
            len++;
        } else {
            out[len++] = c;
            in++;
        }
    }
    return (len);
}

static int url_canonify_uri(std::string& out, const char* in) {
    size_t src, dst;
    uint8_t c;
    int rv;
    bool skip;
    out = std::string(in);

    // First pass, convert '%xx' for safe characters to unescaped forms.
    src = dst = 0;
    while ((c = out[src]) != 0) {
        if (c == '%') {
            if ((!isxdigit(out[src + 1])) || (!isxdigit(out[src + 2]))) {
                return -1;
            }
            c = url_hex_val(out[src + 1]);
            c *= 16;
            c += url_hex_val(out[src + 2]);
            // If it's a safe character, decode, otherwise leave
            // it alone.  We also decode valid high-bytes for
            // UTF-8, which will let us validate them and use
            // those characters in file names later.
            if (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))
                || ((c >= '0') && (c <= '9')) || (c == '.') || (c == '~') || (c == '_')
                || (c == '-') || (c >= 0x80)) {
                out[dst++] = (char)c;
            } else {
                out[dst++] = '%';
                out[dst++] = toupper((uint8_t)out[src + 1]);
                out[dst++] = toupper((uint8_t)out[src + 2]);
            }
            src += 3;
            continue;
        } else {
            out[dst++] = out[src++];
        }
    }
    out[dst] = 0;

    // Second pass, eliminate redundant //.
    src = dst = 0;
    skip      = false;
    while ((c = out[src]) != 0) {
        if ((c == '/') && (!skip)) {
            out[dst++] = '/';
            while (out[src] == '/') {
                src++;
            }
            continue;
        }
        if ((c == '?') || (c == '#')) {
            skip = true;
        }
        out[dst++] = (char)c;
        src++;
    }
    out[dst] = 0;

    // Second pass, reduce /. and /.. elements, but only in the path.
    src = dst        = 0;
    skip             = false;
    const char* pout = &*out.begin();
    while ((c = out[src]) != 0) {
        if ((c == '/') && (!skip)) {
            if ((strncmp(pout + src, "/..", 3) == 0)
                && (out[src + 3] == 0 || out[src + 3] == '#' || out[src + 3] == '?'
                    || out[src + 3] == '/')) {
                if (dst > 0) {
                    do {
                        dst--;
                    } while ((dst) && (out[dst] != '/'));
                }
                src += 3;
                continue;
            }
            if ((strncmp(pout + src, "/.", 2) == 0)
                && (out[src + 2] == 0 || out[src + 2] == '#' || out[src + 2] == '?'
                    || out[src + 2] == '/')) {
                src += 2;  // just skip over it
                continue;
            }
            out[dst++] = '/';
            src++;
        } else {
            if ((c == '?') || (c == '#')) {
                skip = true;
            }
            out[dst++] = (char)c;
            src++;
        }
    }
    out[dst] = 0;

    // Finally lets make sure that the results are valid UTF-8.
    // This guards against using UTF-8 redundancy to break security.
    if ((rv = url_utf8_validate(out.data())) != 0) {
        return (rv);
    }

    out = std::string(pout);
    return 0;
}

int url_default_port(std::string_view scheme) {
    static std::map<std::string, int> kDefaultPorts = {
        {"git",    9418},
        {"gopher", 70  },
        {"http",   80  },
        {"https",  443 },
        {"ssh",    22  },
        {"telnet", 23  },
        {"ws",     80  },
        {"wss",    443 },
    };

    return kDefaultPorts.find(std::string(scheme)) == kDefaultPorts.end()
               ? url_t::nport
               : kDefaultPorts.at(std::string(scheme));
}

bool url_parse_requri(std::string_view requri_raw, url_t& out) {
    std::string requri(requri_raw);
    int len;
    char c;
    if (url_canonify_uri(out.requri, requri.data()) != 0) {
        return false;
    }

    const char* s = &*out.requri.begin();

    for (len = 0; (c = s[len]) != '\0'; len++) {
        if ((c == '?') || (c == '#')) {
            break;
        }
    }

    out.path = std::string(s, len);
    s += len;

    // Look for query info portion.
    if (s[0] == '?') {
        s++;

        for (len = 0; (c = s[len]) != '\0'; len++) {
            if (c == '#') {
                break;
            }
        }

        out.query = std::string(s, len);
        s += len;
    }

    // Look for fragment.  Will always be last, so we just use
    // strdup.
    if (s[0] == '#') {
        out.fragment = std::string(s + 1);
    }

    return true;
}

///
/// @url:    <scheme>://[user@]<hostname>[:port][/requri]
/// @requri: <path>[?query][#fragment]
/// @host:   <hostname>[:port]
///
bool url_parse(std::string_view raw, url_t& out) {
    out.rawurl    = raw;
    const char* s = &*out.rawurl.begin();
    int len;
    char c;

    // Grab the scheme
    for (len = 0; (c = s[len]) != ':'; len++) {
        if (c == 0) {
            break;
        }
    }

    if (strncmp(s + len, "://", 3) != 0) {
        return false;
    }

    out.scheme = std::string(s, len);
    s += len + 3;

    if (out.scheme == "unix") {
        out.path = std::string(s);
        return true;
    }

    // Look for host part (including colon).  Will be terminated by
    // a path, or NUL.  May also include an "@", separating a user
    // field.
    /// [user@]<host>
    for (len = 0; (c = s[len]) != '/'; len++) {
        if ((c == '\0') || (c == '#') || (c == '?')) {
            break;
        }

        if (c == '@') {
            if (!out.userinfo.empty()) {
                return false;
            }

            out.userinfo = std::string(s, len);
            s += len + 1;  // skip past user@ ...
            len = 0;
        }
    }

    // If the hostname part is just '*', skip over it.
    if (((len == 1) && s[len] == '*') || ((len > 1) && (strncmp(s, "*:", 2) == 0))) {
        s++;
        len--;
    }

    out.host = std::string(s, len);
    s += len;

    if (!url_parse_requri(s, out)) {
        return false;
    }

    // Now go back to the host portion, and look for a separate
    // port We also yank off the "[" part for IPv6 addresses.
    s = &*out.host.begin();
    if (s[0] == '[') {
        s++;
        for (len = 0; s[len] != ']'; len++) {
            if (s[len] == '\0') {
                return false;
            }
        }
        if ((s[len + 1] != ':') && (s[len + 1] != '\0')) {
            return false;
        }
    } else {
        for (len = 0; s[len] != ':'; len++) {
            if (s[len] == '\0') {
                break;
            }
        }
    }

    out.hostname = std::string(s, len);
    s += len;

    if (s[0] == ']') {
        s++;  // skip over ']', only used with IPv6 addresses
    }

    if (s[0] == ':') {
        // If a colon was present, but no port value present, then
        // that is an error.
        if (s[1] == '\0') {
            out.port = url_t::nport;
        } else {
            try {
                out.port = stoi(std::string(s + 1));
            } catch (...) {
                out.port = url_t::nport;
            }
        }
    } else {
        out.port = url_default_port(out.scheme);
    }

    if (out.port == url_t::nport) {
        return false;
    }

    return true;
}

}  // namespace ccl2
