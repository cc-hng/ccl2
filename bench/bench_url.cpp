
#include <benchmark/benchmark.h>
#include <ccl2/url.h>

static void BM_url_parse(benchmark::State& state) {
    for (auto _ : state) {
        ccl2::url_t u;
        auto ok = ccl2::url_parse("http://www.baidu.com/a/b/c?key=value#chapter1", u);
        (void)ok;
    }
}

BENCHMARK(BM_url_parse);
