#pragma once

#include <algorithm>
#include <string>
#include <string_view>
#include <ctype.h>

#define CCL2_CALL_FUNC_OUTSIDE(fn) \
    [[maybe_unused]] static const bool __b##__LINE__##__ = ((fn), true)

namespace ccl2 {

struct NonMutex {
    inline void lock() {}
    inline bool try_lock() { return true; }
    inline void unlock() {}
};

template <class Mutex>
class MutexLock {
public:
    explicit MutexLock(Mutex& mtx) : mtx_(mtx) { mtx.lock(); }
    ~MutexLock() { mtx_.unlock(); }

private:
    Mutex& mtx_;
};

// case-independent (ci) compare_less binary function
struct ci_less {
    struct nocase_compare {
        bool operator()(const unsigned char& c1, const unsigned char& c2) const {
            return tolower(c1) < tolower(c2);
        }
    };

    bool operator()(const std::string& s1, const std::string& s2) const {
        return std::lexicographical_compare(s1.begin(),
                                            s1.end(),  // source range
                                            s2.begin(),
                                            s2.end(),           // dest range
                                            nocase_compare());  // comparison
    }

    bool operator()(std::string_view s1, std::string_view s2) const {
        return std::lexicographical_compare(s1.begin(),
                                            s1.end(),  // source range
                                            s2.begin(),
                                            s2.end(),           // dest range
                                            nocase_compare());  // comparison
    }
};

}  // namespace ccl2
