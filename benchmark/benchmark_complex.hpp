#pragma once

#include <chrono>
#include <iostream>

#include "benchmark.hpp"


class BenchmarkComplex : public Benchmark
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


std::unique_ptr<Benchmark> create_benchmark_complex()
{
	return std::make_unique<BenchmarkComplex>();
}
