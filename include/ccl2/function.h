#pragma once

// refer: https://cloud.tencent.com/developer/ask/sof/104494

#include <functional>
#include <memory>
#include <type_traits>
#include <boost/callable_traits.hpp>
#include <boost/hana.hpp>

namespace ct   = boost::callable_traits;
namespace hana = boost::hana;

namespace ccl2 {

namespace detail {

template <class T>
struct tuple_pop_front;

template <class T, class... Ts>
struct tuple_pop_front<hana::tuple<T, Ts...>> {
    using type = hana::tuple<Ts...>;
};

template <class T, class... Ts>
struct tuple_pop_front<std::tuple<T, Ts...>> {
    using type = std::tuple<Ts...>;
};

}  // namespace detail

template <int i>
using Int2Type = std::integral_constant<int, i>;
//!
//! remap function from
//!     R (*)(Args...)
//! to
//!     void (*)(void *args, void *result)
//!
//! @param args:    hana::tuple<Args...>
//! @param result:  R/void
//!
template <class R>
class function {
    using Func = std::function<void(void*, void*)>;
    Func callable_;

    template <class Fn>
    auto make_copyable_function(Fn&& f) {
        using DF = std::decay_t<Fn>;
        auto spf = std::make_shared<DF>(std::forward<Fn>(f));
        return [spf](auto&&... args) -> decltype(auto) {
            return (*spf)(std::forward<decltype(args)>(args)...);
        };
    }

public:
    using Ret = R;

    template <class Fn>
    function(Fn&& f) {
        if constexpr (std::is_copy_assignable_v<std::remove_reference_t<Fn>>) {
            callable_ = std::move(f);
        } else {
            callable_ = make_copyable_function(std::forward<Fn>(f));
        }
    }

    function(function<R>&& other) : callable_(std::move(other.callable_)) {}

    void operator()(void* args, void* result) const {
        if (callable_) {
            callable_(args, result);
        }
    }
};

struct bind_t {
    template <typename Signature, typename Fn,
              typename = hana::when<!std::is_member_function_pointer<Fn>::value>>
    constexpr decltype(auto) operator()(Fn&& fn, std::true_type = {}) const {
        using Args = ct::args_t<Signature, hana::tuple>;
        using Ret  = ct::return_type_t<Signature>;

        // mutable for functor
        auto newf = [fn = std::move(fn)](void* args, void* result) mutable {
            auto args_tuple = static_cast<Args*>(args);
            if constexpr (std::is_void_v<Ret>) {
                hana::unpack(*args_tuple, std::move(fn));
            } else {
                *(Ret*)result = hana::unpack(*args_tuple, std::move(fn));
            }
        };
        return function<Ret>(std::move(newf));
    }

    template <typename Fn,
              typename = hana::when<!std::is_member_function_pointer<Fn>::value>>
    constexpr decltype(auto) operator()(Fn&& fn, std::false_type = {}) const {
        using Args = ct::args_t<Fn, hana::tuple>;
        using Ret  = ct::return_type_t<Fn>;

        // mutable for functor
        auto newf = [fn = std::move(fn)](void* args, void* result) mutable {
            auto args_tuple = static_cast<Args*>(args);
            if constexpr (std::is_void_v<Ret>) {
                hana::unpack(*args_tuple, std::move(fn));
            } else {
                *(Ret*)result = hana::unpack(*args_tuple, std::move(fn));
            }
        };
        return function<Ret>(std::move(newf));
    }

    template <typename Fn, typename Self,
              typename = hana::when<std::is_member_function_pointer<Fn>::value>>
    constexpr decltype(auto) operator()(const Fn& fn, Self self) const {
        using Ret        = ct::return_type_t<Fn>;
        using ArgsOrigin = ct::args_t<Fn, hana::tuple>;
        using Args       = typename detail::tuple_pop_front<ArgsOrigin>::type;

        if (self == nullptr) {
            throw std::runtime_error("self is a nullptr");
        }

        auto f = [fn, self](auto&&... args) {
            if constexpr (std::is_pointer_v<Self>) {
                return (self->*fn)(std::forward<decltype(args)>(args)...);
            } else {
                static constexpr auto has_get =
                    hana::is_valid([](auto&& obj) -> decltype(obj.get()) {});
                BOOST_HANA_CONSTANT_CHECK(has_get(self));
                auto origin = self.get();
                return (origin->*fn)(std::forward<decltype(args)>(args)...);
            }
        };
        auto newf = [f](void* args, void* result) {
            auto args_tuple = static_cast<Args*>(args);
            if constexpr (std::is_void_v<Ret>) {
                hana::unpack(*args_tuple, std::move(f));
            } else {
                *(Ret*)result = hana::unpack(*args_tuple, std::move(f));
            }
        };
        return function<Ret>(std::move(newf));
    }
};

[[maybe_unused]] constexpr bind_t bind{};

//! invoke function<Ret, ArgsTuple> with Args...
struct invoker_t {
    template <typename R, typename... Args>
    R operator()(const function<R>& f, Args&&... args) const {
        auto args_tuple = hana::make_tuple(std::forward<Args>(args)...);
        // TODO: check Xs is_assignable by decltype(args_tupe)
        if constexpr (std::is_void_v<R>) {
            f(&args_tuple, nullptr);
        } else {
            R result{};
            f(&args_tuple, &result);
            return result;
        }
    }
};

[[maybe_unused]] constexpr invoker_t invoke{};

}  // namespace ccl2
