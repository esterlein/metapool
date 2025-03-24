#include <chrono>
#include <iostream>

#include "benchmark.hpp"


class SimpleBenchmark : public Benchmark
{
public:

	void setup() override
	{

	}

	void run() override
	{
		auto start = std::chrono::high_resolution_clock::now();

		// alloc / dealloc

		auto end = std::chrono::high_resolution_clock::now();

		(std::cout)
			<< "simple benchmark time: "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms"
			<< std::endl;
	}

	void teardown() override
	{

	}
};
