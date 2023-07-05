#pragma once

#include <any>
#include <memory>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/core/noncopyable.hpp>
#include <boost/hana.hpp>
#include <ccl2/function.h>
#include <ccl2/singleton_provider.h>
#include <ccl2/utils.h>

namespace hana = boost::hana;

namespace ccl2 {

template <class MutexPolicy = NonMutex, template <class> class ReaderLock = MutexLock,
          template <class> class WriterLock = MutexLock>
class Service : boost::noncopyable {
public:
    using any_type = void*;
    Service()      = default;

    template <class F, class = hana::when<!std::is_member_function_pointer_v<F>>>
    void register_handler(std::string_view svc, F&& f) {
        WriterLock<MutexPolicy> _lck{mtx_};
        if (registry_.count(svc.data())) {
            throw std::runtime_error("dup svc: " + std::string(svc));
        }
        auto bf    = ccl2::bind(std::forward<F>(f));
        auto pbf   = std::make_shared<decltype(bf)>(std::move(bf));
        any_type p = pbf.get();
        registry_.emplace(std::string(svc), p);

        auto v = std::make_any<decltype(pbf)>(pbf);
        dummy_.emplace_back(v);
    }

    template <class MF, class Obj,
              class = hana::when<std::is_member_function_pointer_v<MF>>>
    void register_handler(std::string_view svc, const MF& f, Obj obj) {
        WriterLock<MutexPolicy> _lck{mtx_};
        if (registry_.count(svc.data())) {
            throw std::runtime_error("dup svc: " + std::string(svc));
        }
        auto bf    = ccl2::bind(f, obj);
        auto pbf   = std::make_shared<decltype(bf)>(std::move(bf));
        any_type p = pbf.get();
        registry_.emplace(std::string(svc), p);

        auto v = std::make_any<decltype(pbf)>(std::move(pbf));
        dummy_.emplace_back(v);
    }

    template <class R, class... Args>
    R call(std::string_view svc, Args&&... args) {
        using T = ccl2::function<R>;
        ReaderLock<MutexPolicy> _lck{mtx_};
        if (!registry_.count(svc.data())) {
            throw std::runtime_error("expect svc: " + std::string(svc));
        }
        auto f = static_cast<T*>(registry_.at(std::string(svc)));
        return ccl2::invoke(*f, std::forward<Args>(args)...);
    }

    template <class R, class Executor, class... Args>
    R async_call(std::string_view svc, Args&&... args,
                 const std::function<void(const R&)>& callback);

private:
    MutexPolicy mtx_;
    std::vector<std::any> dummy_;
    std::unordered_map<std::string, any_type> registry_;
};

using ServiceProvider = SingletonProvider<Service<>>;
using ConcurrentServiceProvider =
    SingletonProvider<Service<std::shared_mutex, std::shared_lock>>;

}  // namespace ccl2
