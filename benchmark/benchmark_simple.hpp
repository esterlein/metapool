#pragma once

#include <chrono>
#include <iostream>

#include "benchmark.hpp"


class BenchmarkSimple : public Benchmark
{
public:

	inline void setup() override
	{
		Benchmark::basic_tests();
	}

	inline void run() override
	{
		hpr::MemoryModel memory_model;
		auto& allocator = memory_model.get_memory_resource();

		std::cout << "starting simple benchmark...\n";
		auto start = std::chrono::high_resolution_clock::now();

		for (int i = 0; i < 1000000; ++i) {
			void* ptr = allocator.allocate(256, 64);
			allocator.deallocate(ptr, 256, 64);
		}

		auto end = std::chrono::high_resolution_clock::now();

		(std::cout)
			<< "simple benchmark time: "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms\n";
	}

	inline void teardown() override
	{

	}
};


std::unique_ptr<Benchmark> create_benchmark_simple()
{
	return std::make_unique<BenchmarkSimple>();
}
