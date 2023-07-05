#pragma once

#include <string>
#include <string_view>

namespace ccl2 {

struct url_t {
    std::string rawurl;    // never NULL
    std::string scheme;    // never NULL
    std::string userinfo;  // will be NULL if not specified
    std::string host;      // including colon and port
    std::string hostname;  // name only, will be "" if not specified
    int port;              // port, will be <nport> if not specified
    std::string requri;    // includes query and fragment
    std::string path;      // path, will be "" if not specified
    std::string query;     // without '?', will be NULL if not specified
    std::string fragment;  // without '#', will be NULL if not specified

    static const int nport;
};

int url_default_port(std::string_view scheme);
bool url_parse(std::string_view url, url_t& out);
bool url_parse_requri(std::string_view requri, url_t& out);

}  // namespace ccl2
