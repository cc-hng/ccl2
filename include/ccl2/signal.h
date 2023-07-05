#pragma once

#include <shared_mutex>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <boost/core/noncopyable.hpp>
#include <ccl2/function.h>
#include <ccl2/singleton_provider.h>
#include <ccl2/utils.h>

namespace ccl2 {

template <class MutexPolicy = NonMutex, template <class> class ReaderLock = MutexLock,
          template <class> class WriterLock = MutexLock>
class Signal final : public boost::noncopyable {
public:
    Signal() = default;

    template <class Signature, class Callable>
    void register_handler2(std::string_view topic, Callable&& call) {
        WriterLock<MutexPolicy> _lck{mtx_};
        registry_.emplace(
            std::string(topic),
            ccl2::bind.template operator()<Signature>(std::forward<Callable>(call)));
    }

    template <class F, class = std::enable_if_t<!std::is_member_function_pointer_v<F>>>
    void register_handler(std::string_view topic, F&& f) {
        WriterLock<MutexPolicy> _lck{mtx_};
        registry_.emplace(std::string(topic), ccl2::bind(std::forward<F>(f)));
    }

    // NOTICE: lifetime of T
    // Obj * or std::shared_ptr<Obj>
    template <class MemFn, class Obj,
              class = std::enable_if_t<std::is_member_function_pointer_v<MemFn>>>
    void register_handler(std::string_view topic, const MemFn& mem, Obj obj) {
        WriterLock<MutexPolicy> _lck{mtx_};
        registry_.emplace(std::string(topic), ccl2::bind(mem, obj));
    }

    template <class... Args>
    void emit(std::string_view topic, Args&&... args) {
        ReaderLock<MutexPolicy> _lck{mtx_};
        auto range = registry_.equal_range(topic.data());
        for (auto it = range.first; it != range.second; ++it) {
            ccl2::invoke(it->second, args...);
        }
    }

    template <class Executor, class... Args>
    void emit_on(Executor& executor, std::string_view topic, Args&&... args) {
        // executor.e
        std::function<void(std::string, Args...)> f = [this](std::string _topic,
                                                             Args&&... _args) {
            emit(_topic, std::forward<Args>(_args)...);
        };
        executor.enqueue(std::bind(f, std::string(topic), std::forward<Args>(args)...));
    }

    void remove(std::string_view topic) {
        WriterLock<MutexPolicy> _lck{mtx_};
        auto range = registry_.equal_range(topic.data());
        registry_.erase(range.first, range.second);
    }

private:
    MutexPolicy mtx_;
    std::unordered_multimap<std::string, ccl2::function<void>> registry_;
};

using SignalProvider = SingletonProvider<Signal<>>;
using ConcurrentSignalProvider =
    SingletonProvider<Signal<std::shared_mutex, std::shared_lock>>;

}  // namespace ccl2
