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

		std::cout << "starting simple benchmark...\n"
		auto start = std::chrono::high_resolution_clock::now();

		// alloc / dealloc

		auto end = std::chrono::high_resolution_clock::now();

		(std::cout)
			<< "simple benchmark time: "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms\n"
	}

	inline void teardown() override
	{

	}
};
