#pragma once

#include <chrono>
#include <iostream>

#include "benchmark.hpp"
#include "memory_model.hpp"


class BenchmarkSimple : public Benchmark
{
public:

	inline void setup() override
	{
		std::cout << "\n--- metapool memory model simple benchmark ---\n";;
		std::cout << "\nrunning basic metapool tests...\n";

		Benchmark::basic_tests<hpr::mem::AllocatorType::Simple>();
	}

	inline void run() override
	{
		std::cout << "\n";
		run_metapool();
		std::cout << "\n";
		run_pmr();
		std::cout << "\n";
		run_std();
		std::cout << "\n";
		print_summary();
		std::cout << "\n";
	}

	inline void teardown() override
	{
		std::cout << "\n";
		hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Simple>().reset();
	}

private:

	double m_mpool_time {0.0};
	double m_pmr_time   {0.0};
	double m_std_time   {0.0};

	inline void run_metapool()
	{
		auto& allocator = hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Simple>();

		std::cout << "run metapool benchmark..." << std::endl;

		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1000000000; ++i) {
			std::byte* ptr = allocator.alloc(256, 64);
			allocator.free(ptr);
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_mpool_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool benchmark time: " << m_mpool_time << " ms" << std::endl;
	}

	inline void run_pmr()
	{
		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<char> allocator(&upstream);

		std::cout << "run default pmr benchmark..." << std::endl;

		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1000000000; ++i) {
			void* ptr = allocator.resource()->allocate(256, 64);
			allocator.resource()->deallocate(ptr, 256, 64);
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_pmr_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "default pmr benchmark time: " << m_pmr_time << " ms" << std::endl;
	}

	inline void run_std()
	{
		std::allocator<char> allocator;

		std::cout << "run standard allocator benchmark..." << std::endl;

		auto start = std::chrono::high_resolution_clock::now();
		for (int i = 0; i < 1000000000; ++i) {
			char* ptr = allocator.allocate(256);
			allocator.deallocate(ptr, 256);
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_std_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "standard allocator benchmark time: " << m_std_time << " ms" << std::endl;
	}
	
	void print_summary()
	{
		std::cout << "\n--- benchmark summary ---\n";

		std::cout << "metapool: " << m_mpool_time << " ms\n";
		std::cout << "default pmr: " << m_pmr_time << " ms\n";
		std::cout << "standard allocator: " << m_std_time << " ms\n";

		double metapool_vs_pmr = m_pmr_time / m_mpool_time;
		double metapool_vs_std = m_std_time / m_mpool_time;

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
