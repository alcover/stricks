#include <benchmark/benchmark.h>
#include <string>
#include <array>

extern "C" {
#include "sds/sds.h"
#include "../src/stx.h"
}

std::string randStr(size_t n) 
{
	std::array<char, 4> ch = {'a', 'b', 'c', 'd'}; 
	std::string ret;
	for (size_t i = 0; i<n; ++i)
		ret.push_back(ch[i % ch.size()]);

  	return ret;
}

void BM_stx_append (benchmark::State& state) 
{
	auto const to_append = randStr(state.range(0));
	stx_t s = stx_from("");
	for (auto _ : state) {
		stx_append(&s, to_append.c_str(), to_append.size());
	}
	benchmark::ClobberMemory();
	stx_free(s);
}

void BM_sds_append (benchmark::State& state) 
{
	auto const to_append = randStr(state.range(0));
	sds s = sdsnew("");
	for (auto _ : state) {
		s = sdscatlen(s, to_append.c_str(), to_append.size());
	}
	benchmark::ClobberMemory();
	sdsfree(s);
}

void BM_std_append(benchmark::State& state) 
{
	auto const to_append = randStr(state.range(0));
	std::string s;
	for (auto _ : state) {
		s += to_append;
	}
	benchmark::ClobberMemory();
}


#define MULT 8
#define RNG 1<<15

// BENCHMARK(BM_std_append)->RangeMultiplier(MULT)->Range(8, RNG);
BENCHMARK(BM_sds_append)->RangeMultiplier(MULT)->Range(8, RNG);
BENCHMARK(BM_stx_append)->RangeMultiplier(MULT)->Range(8, RNG);

BENCHMARK_MAIN();
