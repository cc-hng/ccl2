#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <ccl2/service.h>
#include <fmt/core.h>

using ServiceThreadSafe = ccl2::ConcurrentServiceProvider;

std::string get_name() {
    return "hello";
}

void print(std::string s) {
    fmt::print("print: {} \n", s);
}

int sub(int a, int b) {
    return a - b;
}

int add(int a, int b) {
    return a + b;
}

struct algorithm_t {
    int mul(int a, int b) { return a * b; }

    int div(int a, int b) { return a / b; }
};

void print_buf(int* buf, int len) {
    for (int i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\n");
}

int main() {
    auto& mb = ServiceThreadSafe::get();
    {
        auto algorithm = std::make_shared<algorithm_t>();
        mb.register_handler("/algorithm/add", add);
        mb.register_handler("/algorithm/sub", sub);
        mb.register_handler("/algorithm/mul", &algorithm_t::mul, algorithm);
        mb.register_handler("/algorithm/div", &algorithm_t::div, algorithm);
    }
    mb.register_handler("/log", print);


    mb.register_handler("/print_buf", print_buf);
    int buf[8] = {1};
    mb.call<void>("/print_buf", buf, 8);

    int a = mb.call<int>("/algorithm/add", 3, 4);
    a     = mb.call<int>("/algorithm/add", 3, 4);
    fmt::print("a+b={}, expect:{}\n", a, 7);

    int b = mb.call<int>("/algorithm/sub", 4, 3);
    b     = mb.call<int>("/algorithm/sub", 4, 3);
    fmt::print("a-b={}, expect:{}\n", b, 1);

    int c = mb.call<int>("/algorithm/mul", 4, 3);
    c     = mb.call<int>("/algorithm/mul", 4, 3);
    fmt::print("a*b={}, expect:{}\n", c, 12);

    int d = mb.call<int>("/algorithm/div", 12, 3);
    d     = mb.call<int>("/algorithm/div", 12, 3);
    fmt::print("a/b={}, expect:{}\n", d, 4);

    {
        std::string m = "hello,world";
        mb.call<void>("/log", m);
        mb.call<void>("/log", std::move(m));
    }

    mb.register_handler("/get_name", get_name);
    std::string name = mb.call<std::string>("/get_name");
    fmt::print("name: {}\n", name);

    return 0;
}
