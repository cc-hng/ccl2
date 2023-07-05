#include <ccl2/url.h>
#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(Url, unix_domain_socket) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("unix:///tmp/test.sock", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "unix");
    EXPECT_TRUE(u.path == "/tmp/test.sock");
}

TEST(Url, host) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://www.google.com", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "http");
    EXPECT_TRUE(u.host == "www.google.com");
    EXPECT_TRUE(u.hostname == "www.google.com");
    EXPECT_TRUE(u.port == 80);
    EXPECT_TRUE(u.path == "");
    EXPECT_TRUE(u.requri == "");
    EXPECT_TRUE(u.userinfo == "");
    EXPECT_TRUE(u.fragment == "");
    EXPECT_TRUE(u.query == "");
}

TEST(Url, host_port_path) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://www.google.com:1234/somewhere", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "http");
    EXPECT_TRUE(u.host == "www.google.com:1234");
    EXPECT_TRUE(u.hostname == "www.google.com");
    EXPECT_TRUE(u.port == 1234);
    EXPECT_TRUE(u.requri == "/somewhere");
    EXPECT_TRUE(u.path == "/somewhere");
    EXPECT_TRUE(u.userinfo == "");
    EXPECT_TRUE(u.fragment == "");
    EXPECT_TRUE(u.query == "");
}

TEST(Url, user_info) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://garrett@www.google.com:1234/somewhere", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "http");
    EXPECT_TRUE(u.host == "www.google.com:1234");
    EXPECT_TRUE(u.hostname == "www.google.com");
    EXPECT_TRUE(u.port == 1234);
    EXPECT_TRUE(u.requri == "/somewhere");
    EXPECT_TRUE(u.path == "/somewhere");
    EXPECT_TRUE(u.userinfo == "garrett");
    EXPECT_TRUE(u.fragment == "");
    EXPECT_TRUE(u.query == "");
}

TEST(Url, path_query_param) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://www.google.com/somewhere?result=yes", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "http");
    EXPECT_TRUE(u.host == "www.google.com");
    EXPECT_TRUE(u.hostname == "www.google.com");
    EXPECT_TRUE(u.port == 80);
    EXPECT_TRUE(u.requri == "/somewhere?result=yes");
    EXPECT_TRUE(u.path == "/somewhere");
    EXPECT_TRUE(u.userinfo == "");
    EXPECT_TRUE(u.fragment == "");
    EXPECT_TRUE(u.query == "result=yes");
}

TEST(Url, path_query_param_anchor) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://www.google.com/somewhere?result=yes#chapter1", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "http");
    EXPECT_TRUE(u.host == "www.google.com");
    EXPECT_TRUE(u.hostname == "www.google.com");
    EXPECT_TRUE(u.port == 80);
    EXPECT_TRUE(u.requri == "/somewhere?result=yes#chapter1");
    EXPECT_TRUE(u.path == "/somewhere");
    EXPECT_TRUE(u.userinfo == "");
    EXPECT_TRUE(u.fragment == "chapter1");
    EXPECT_TRUE(u.query == "result=yes");
}

TEST(Url, path_anchor) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://www.google.com/somewhere#chapter1", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "http");
    EXPECT_TRUE(u.host == "www.google.com");
    EXPECT_TRUE(u.hostname == "www.google.com");
    EXPECT_TRUE(u.port == 80);
    EXPECT_TRUE(u.requri == "/somewhere#chapter1");
    EXPECT_TRUE(u.path == "/somewhere");
    EXPECT_TRUE(u.userinfo == "");
    EXPECT_TRUE(u.fragment == "chapter1");
    EXPECT_TRUE(u.query == "");
}

TEST(Url, anchor) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://www.google.com#chapter1", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "http");
    EXPECT_TRUE(u.host == "www.google.com");
    EXPECT_TRUE(u.hostname == "www.google.com");
    EXPECT_TRUE(u.port == 80);
    EXPECT_TRUE(u.requri == "#chapter1");
    EXPECT_TRUE(u.path == "");
    EXPECT_TRUE(u.userinfo == "");
    EXPECT_TRUE(u.fragment == "chapter1");
    EXPECT_TRUE(u.query == "");
}

TEST(Url, query_param) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://www.google.com?color=red", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "http");
    EXPECT_TRUE(u.host == "www.google.com");
    EXPECT_TRUE(u.hostname == "www.google.com");
    EXPECT_TRUE(u.port == 80);
    EXPECT_TRUE(u.requri == "?color=red");
    EXPECT_TRUE(u.path == "");
    EXPECT_TRUE(u.userinfo == "");
    EXPECT_TRUE(u.fragment == "");
    EXPECT_TRUE(u.query == "color=red");
}

TEST(Url, tcp_port) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("tcp://:9876/", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "tcp");
    EXPECT_TRUE(u.host == ":9876");
    EXPECT_TRUE(u.hostname == "");
    EXPECT_TRUE(u.port == 9876);
    EXPECT_TRUE(u.requri == "/");
    EXPECT_TRUE(u.path == "/");
    EXPECT_TRUE(u.userinfo == "");
    EXPECT_TRUE(u.fragment == "");
    EXPECT_TRUE(u.query == "");
}

TEST(Url, ssh) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("ssh://user@127.0.0.1:9876", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "ssh");
    EXPECT_TRUE(u.host == "127.0.0.1:9876");
    EXPECT_TRUE(u.hostname == "127.0.0.1");
    EXPECT_TRUE(u.port == 9876);
    EXPECT_TRUE(u.requri == "");
    EXPECT_TRUE(u.path == "");
    EXPECT_TRUE(u.userinfo == "user");
    EXPECT_TRUE(u.fragment == "");
    EXPECT_TRUE(u.query == "");
}

TEST(Url, path_resolve) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://www.x.com//abc/def/./x/..///./../y", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.scheme == "http");
    EXPECT_TRUE(u.host == "www.x.com");
    EXPECT_TRUE(u.hostname == "www.x.com");
    EXPECT_TRUE(u.port == 80);
    EXPECT_TRUE(u.path == "/abc/y");
    EXPECT_TRUE(u.userinfo == "");
    EXPECT_TRUE(u.fragment == "");
    EXPECT_TRUE(u.query == "");
}

TEST(Url, requri_only) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse_requri("/somewhere?a=b&c=d#e", u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.requri == "/somewhere?a=b&c=d#e");
    EXPECT_TRUE(u.path == "/somewhere");
    EXPECT_TRUE(u.fragment == "e");
    EXPECT_TRUE(u.query == "a=b&c=d");
}

TEST(Url, requri_only2) {
    std::string raw = "/abc";
    ccl2::url_t u;
    auto ok = ccl2::url_parse_requri(std::string_view(raw.data(), 2), u);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(u.requri == "/a");
    EXPECT_TRUE(u.path == "/a");
    EXPECT_TRUE(u.fragment == "");
    EXPECT_TRUE(u.query == "");
}

TEST(Url, bad_scheme) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("www.google.com", u);
    EXPECT_TRUE(!ok);
    ok = ccl2::url_parse("http:www.google.com", u);
    EXPECT_TRUE(!ok);
}

TEST(Url, bad_port) {
    ccl2::url_t u;
    auto ok = ccl2::url_parse("http://www.google.com:axda", u);
    EXPECT_TRUE(!ok);
}
