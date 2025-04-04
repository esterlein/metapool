#pragma once

#include <chrono>
#include <iostream>

#include "benchmark.hpp"


class BenchmarkSimple : public Benchmark
{
public:

	inline void setup() override
	{
//		std::cout << "\nrunning basic metapool tests...\n";
//		Benchmark::basic_tests();
	}

	inline void run() override
	{
		std::cout << '\n';
		run_metapool();
		run_pmr();
		run_std();
		print_summary();
	}

	inline void teardown() override
	{
		std::cout << "\n";
	}

private:

	double m_metapool_time = 0.0;
	double m_pmr_time = 0.0;
	double m_std_time = 0.0;

	inline void run_metapool()
	{
		hpr::MemoryModel memory_model;
		auto& allocator = memory_model.get_memory_resource();

		std::cout << "starting metapool benchmark...\n";

		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1000000000; ++i) {
			void* ptr = allocator.allocate(256, 64);
			allocator.deallocate(ptr, 256, 64);
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_metapool_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool benchmark time: " << m_metapool_time << " ms\n";
	}

	inline void run_pmr()
	{
		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<char> allocator(&upstream);

		std::cout << "starting default pmr benchmark...\n";

		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1000000000; ++i) {
			void* ptr = allocator.resource()->allocate(256, 64);
			allocator.resource()->deallocate(ptr, 256, 64);
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_pmr_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "default pmr benchmark time: " << m_pmr_time << " ms\n";
	}

	inline void run_std()
	{
		std::allocator<char> allocator;

		std::cout << "starting standard allocator benchmark...\n";

		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1000000000; ++i) {
			char* ptr = allocator.allocate(256);
			allocator.deallocate(ptr, 256);
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_std_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "standard allocator benchmark time: " << m_std_time << " ms\n";
	}
	
	void print_summary()
	{
		std::cout << "\n--- benchmark summary ---\n";

		std::cout << "metapool: " << m_metapool_time << " ms\n";
		std::cout << "default pmr: " << m_pmr_time << " ms\n";
		std::cout << "standard allocator: " << m_std_time << " ms\n";

		double metapool_vs_pmr = m_pmr_time / m_metapool_time;
		double metapool_vs_std = m_std_time / m_metapool_time;

		std::cout << "\n--- performance comparison ---\n";

		(std::cout)
			<< "metapool vs pmr: " << metapool_vs_pmr << "x "
			<< (metapool_vs_pmr > 1.0 ? "faster" : "slower") << "\n";
		(std::cout)
			<< "metapool vs std: " << metapool_vs_std << "x "
			<< (metapool_vs_std > 1.0 ? "faster" : "slower") << "\n";
	}
};


std::unique_ptr<Benchmark> create_benchmark_simple()
{
	return std::make_unique<BenchmarkSimple>();
}
