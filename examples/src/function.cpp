#include <iostream>
#include <ccl2/function.h>

/// usage
inline int add(int x, int y) {
    return x + y;
}

inline int add2(const int& x, const int& y) {
    return x + y;
}

struct Algorithm {
    int add(int x, int y) { return x + y; }
};

constexpr auto lambda_add = [](int x, int y) {
    return x + y;
};

struct functor_add {
    int operator()(int x, int y) { return x + y; }
};

int main() {
    {
        auto f     = ccl2::bind(add);
        int result = ccl2::invoke(f, 1, 2);
        std::cout << "common function result: " << result << std::endl;

        int a       = 3;
        const int b = 5;

        auto f2      = ccl2::bind(add2);
        auto result2 = ccl2::invoke(f2, std::ref(a), std::ref(b));
        std::cout << "common function result2: " << result2 << std::endl;

        // struct {
        // } c;
        // auto r3 = ccl2::invoke(f, c, a);
    }

    {
        auto f     = ccl2::bind(lambda_add);
        int result = ccl2::invoke(f, 1, 2);
        std::cout << "lambda function result: " << result << std::endl;
    }

    {
        auto f     = ccl2::bind(functor_add{});
        int result = ccl2::invoke(f, 1, 2);
        std::cout << "functor function result: " << result << std::endl;
    }

    using Args  = ct::args_t<decltype(&Algorithm::add), hana::tuple>;
    using Args2 = ccl2::detail::tuple_pop_front<Args>::type;
    static_assert(std::is_same<Args, hana::tuple<Algorithm&, int, int>>::value, "");
    static_assert(std::is_same<Args2, hana::tuple<int, int>>::value, "");
    {
        Algorithm algorithm;
        auto f     = ccl2::bind(&Algorithm::add, &algorithm);
        int result = ccl2::invoke(f, 1, 2);
        std::cout << "struct function result: " << result << std::endl;
    }

    {
        auto algorithm = std::shared_ptr<Algorithm>(new Algorithm);
        auto f         = ccl2::bind(&Algorithm::add, algorithm);
        int result     = ccl2::invoke(f, 1, 2);
        std::cout << "shared_ptr function result: " << result << std::endl;
    }

    return 0;
}
