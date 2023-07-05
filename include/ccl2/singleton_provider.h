#pragma once

#include <type_traits>

namespace ccl2 {

namespace detail {

template <typename T>
inline constexpr bool is_noncopyable_v =
    (!std::is_copy_constructible_v<T> && !std::is_copy_assignable_v<T>);

}

template <class T, int = (std::is_constructible_v<T> && detail::is_noncopyable_v<T>)>
struct SingletonProvider {
    using value_type = T;

    static T& get() {
        static T ins{};
        return ins;
    }
};

}  // namespace ccl2
