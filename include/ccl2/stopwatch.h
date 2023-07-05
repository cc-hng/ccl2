#pragma once

#include <chrono>

// Displays elapsed seconds since construction as double.
namespace ccl2 {

class StopWatch {
    using clock = std::chrono::steady_clock;
    clock::time_point start_tp_;

public:
    StopWatch() : start_tp_{clock::now()} {}

    double elapsed() const {
        return std::chrono::duration<double>(clock::now() - start_tp_).count();
    }

    void reset() { start_tp_ = clock ::now(); }
};

}  // namespace ccl2
