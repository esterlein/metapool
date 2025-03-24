#pragma once

#include <chrono>
#include <iostream>

#include "benchmark.hpp"


class BenchmarkIntermediate : public Benchmark
{
public:

	inline void setup() override
	{

	}

	inline void run() override
	{

	}

	inline void teardown() override
	{

	}
};


std::unique_ptr<Benchmark> create_benchmark_intermediate()
{
	return std::make_unique<BenchmarkIntermediate>();
}
