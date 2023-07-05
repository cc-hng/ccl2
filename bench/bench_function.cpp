#include <functional>
#include <benchmark/benchmark.h>
#include <ccl2/function.h>

static void BM_std_function(benchmark::State& state) {
    for (auto _ : state) {
        std::function<int(int, int)> f = [](int a, int b) {
            return a + b;
        };
        [[maybe_unused]] auto r = f(3, 4);
    }
}

BENCHMARK(BM_std_function);

static void BM_ccl2_function(benchmark::State& state) {
    for (auto _ : state) {
        ccl2::function<int> f   = ccl2::bind([](int a, int b) { return a + b; });
        [[maybe_unused]] auto r = ccl2::invoke(f, 3, 4);
    }
}

BENCHMARK(BM_ccl2_function);
