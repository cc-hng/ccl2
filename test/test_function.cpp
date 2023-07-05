#include <ccl2/function.h>
#include <gtest/gtest.h>

namespace {

inline int add(int a, int b) {
    return a + b;
}

inline constexpr auto lambda_add = [](int a, int b) {
    return a + b;
};

class AddFunctor {
public:
    int operator()(int a, int b) { return a + b; }
};

class Algorithm {
public:
    int add(int a, int b) { return a + b; }
};

}  // namespace

TEST(Function, function_ptr) {
    auto f = ccl2::bind(add);
    bool b = std::is_same_v<ccl2::function<int>, decltype(f)>;
    EXPECT_TRUE(b);
    EXPECT_TRUE(ccl2::invoke(f, 4, 3) == 7);
}

TEST(Function, member_function) {
    auto algo = std::make_shared<Algorithm>();
    auto f    = ccl2::bind(&Algorithm::add, algo);
    bool b    = std::is_same_v<ccl2::function<int>, decltype(f)>;
    EXPECT_TRUE(b);
    EXPECT_TRUE(ccl2::invoke(f, 4, 3) == 7);
}

TEST(Function, functor) {
    auto f = ccl2::bind(AddFunctor());
    bool b = std::is_same_v<ccl2::function<int>, decltype(f)>;
    EXPECT_TRUE(b);
    EXPECT_TRUE(ccl2::invoke(f, 4, 3) == 7);
}

TEST(Function, lambda) {
    auto f = ccl2::bind(lambda_add);
    bool b = std::is_same_v<ccl2::function<int>, decltype(f)>;
    EXPECT_TRUE(b);
    EXPECT_TRUE(ccl2::invoke(f, 4, 3) == 7);
}
