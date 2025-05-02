#pragma once

#include <chrono>
#include <iostream>
#include <vector>
#include <memory_resource>

#include "benchmark.hpp"
#include "memory_model.hpp"


class BenchmarkSimple : public Benchmark
{
public:

	inline void setup() override
	{
		std::cout << "\n--- metapool memory model simple benchmark ---\n" << std::endl;
		std::cout << "running basic metapool tests...\n" << std::endl;

		Benchmark::basic_tests();
	}

	inline void run() override
	{
		std::cout << std::endl;
		run_mpool();
		std::cout << std::endl;
		run_std();
		std::cout << std::endl;
		run_pmr();
		std::cout << std::endl;

		print_summary();
		std::cout << std::endl;
	}

	inline void teardown() override
	{
		std::cout << std::endl;

		auto& adapter_std = adapter_simple_std();
		adapter_std.reset();
	}

private:

	double m_native_time {0.0};
	double m_std_time    {0.0};
	double m_pmr_time    {0.0};

	static constexpr std::size_t k_allocation_count = 8192;


	void run_mpool()
	{
		auto& adapter_std = adapter_simple_std();

		std::cout << "--- metapool through std benchmark ---\n" << std::endl;

		std::size_t sizes[] = {32, 64, 128, 256, 512, 1024};
		std::size_t align = 64;

		std::vector<std::byte*, AdapterStd<std::byte*>> blocks(adapter_std);

		blocks.reserve(k_allocation_count);

		auto start = std::chrono::high_resolution_clock::now();
		for (std::size_t i = 0; i < k_allocation_count; ++i) {
			std::size_t size = sizes[i % (sizeof(sizes) / sizeof(sizes[0]))];
			std::byte* ptr = adapter_std.alloc(static_cast<uint32_t>(size), static_cast<uint32_t>(align));
			blocks.push_back(ptr);
		}
		for (std::byte* ptr : blocks) {
			adapter_std.free(ptr);
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_native_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "native time: " << m_native_time << " ms\n" << std::endl;
	}

	void run_std()
	{
		std::cout << "--- std benchmark ---\n" << std::endl;

		std::allocator<std::byte*> block_allocator;
		std::allocator<std::byte> data_allocator;

		std::vector<std::byte*, std::allocator<std::byte*> blocks {block_allocator};
		blocks.reserve(k_allocation_count);

		std::size_t sizes[] = {32, 64, 128, 256, 512, 1024};

		auto start = std::chrono::high_resolution_clock::now();
		for (std::size_t i = 0; i < k_allocation_count; ++i) {
			std::size_t size = sizes[i % (sizeof(sizes) / sizeof(sizes[0]))];
			std::byte* ptr = data_allocator.allocate(size);
			blocks.push_back(ptr);
		}
		for (std::byte* ptr : blocks) {
			data_allocator.deallocate(ptr, 0);
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_std_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std allocator time: " << m_std_time << " ms\n" << std::endl;
	}

	void run_pmr()
	{
		std::cout << "--- pmr benchmark ---\n" << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<std::byte> data_allocator(&upstream);
		std::pmr::polymorphic_allocator<std::byte*> block_allocator(&upstream);

		std::pmr::vector<std::byte*> blocks {block_allocator};
		blocks.reserve(k_allocation_count);

		std::size_t sizes[] = {32, 64, 128, 256, 512, 1024};
		std::size_t align = 64;

		auto start = std::chrono::high_resolution_clock::now();
		for (std::size_t i = 0; i < k_allocation_count; ++i) {
			std::size_t size = sizes[i % (sizeof(sizes) / sizeof(sizes[0]))];
			void* ptr = data_allocator.allocate(size);
			blocks.push_back(reinterpret_cast<std::byte*>(ptr));
		}
		for (std::byte* ptr : blocks) {
			data_allocator.deallocate(ptr, 0);
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_pmr_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr time: " << m_pmr_time << " ms\n" << std::endl;
	}

	void print_summary()
	{
		std::cout << "\n--- benchmark summary ---\n" << std::endl;

		std::cout << "native: " << m_native_time << " ms\n";
		std::cout << "std:    " << m_std_time    << " ms\n";
		std::cout << "pmr:    " << m_pmr_time    << " ms\n";

		auto ratio = [](double base, double val) {
			double r = val / base;
			std::cout << r << "x " << (r > 1.0 ? "slower" : "faster");
		};

		std::cout << "\n--- comparison vs native ---\n" << std::endl;

		std::cout << "std: "; ratio(m_native_time, m_std_time); std::cout << "\n";
		std::cout << "pmr: "; ratio(m_native_time, m_pmr_time); std::cout << "\n";
	}
};

std::unique_ptr<Benchmark> create_benchmark_simple()
{
	return std::make_unique<BenchmarkSimple>();
}
