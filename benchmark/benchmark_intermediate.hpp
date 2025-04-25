#pragma once

#include <chrono>
#include <iostream>
#include <vector>
#include <array>
#include <memory_resource>

#include "benchmark.hpp"
#include "memory_model.hpp"



class BenchmarkIntermediate : public Benchmark
{
public:

	inline void setup() override
	{
		std::cout << "\n--- metapool intermediate benchmark ---\n";
		Benchmark::basic_tests();
	}

	inline void run() override
	{
		run_pattern("ecs", 32, 4, 5000, 100);
		run_pattern("renderer", 64, 4, 500, 100);
		run_pattern("ui", 48, 1, 256, 1000);
		print_summary();
	}

	inline void teardown() override
	{
		hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Standard>().reset();
	}

private:

	double m_metapool_time = 0.0;
	double m_pmr_time = 0.0;
	double m_std_time = 0.0;

	inline void run_pattern(const char* label, int base_size, int var_factor, int count, int frames)
	{
		double metapool_time = 0, pmr_time = 0, std_time = 0;

		{
			auto& allocator = hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Standard>();
			std::vector<std::byte*> objects;
			objects.reserve(count);

			auto start = std::chrono::high_resolution_clock::now();
			for (int frame = 0; frame < frames; ++frame) {
				for (int i = 0; i < count; ++i) {
					std::byte* ptr = allocator.alloc(base_size + (i % var_factor) * 16, 16);
					objects.push_back(ptr);
				}
				for (auto* ptr : objects) allocator.free(ptr);
				objects.clear();
			}
			auto end = std::chrono::high_resolution_clock::now();
			metapool_time = std::chrono::duration<double, std::milli>(end - start).count();
		}

		{
			std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
			std::pmr::polymorphic_allocator<std::byte> allocator(&upstream);
			std::vector<std::byte*> objects;
			objects.reserve(count);

			auto start = std::chrono::high_resolution_clock::now();
			for (int frame = 0; frame < frames; ++frame) {
				for (int i = 0; i < count; ++i) {
					std::byte* ptr = static_cast<std::byte*>(allocator.allocate(base_size + (i % var_factor) * 16, 16));
					objects.push_back(ptr);
				}
				for (auto* ptr : objects) allocator.deallocate(ptr, 0, 16);
				objects.clear();
			}
			auto end = std::chrono::high_resolution_clock::now();
			pmr_time = std::chrono::duration<double, std::milli>(end - start).count();
		}

		{
			std::allocator<std::byte> allocator;
			std::vector<std::byte*> objects;
			objects.reserve(count);

			auto start = std::chrono::high_resolution_clock::now();
			for (int frame = 0; frame < frames; ++frame) {
				for (int i = 0; i < count; ++i) {
					std::byte* ptr = allocator.allocate(base_size + (i % var_factor) * 16);
					objects.push_back(ptr);
				}
				for (auto* ptr : objects) allocator.deallocate(ptr, base_size);
				objects.clear();
			}
			auto end = std::chrono::high_resolution_clock::now();
			std_time = std::chrono::duration<double, std::milli>(end - start).count();
		}

		std::cout << "\n--- " << label << " pattern summary ---\n";
		std::cout << "metapool: " << metapool_time << " ms\n";
		std::cout << "pmr:      " << pmr_time << " ms\n";
		std::cout << "std:      " << std_time << " ms\n";

		if (std::string(label) == "ecs") {
			m_metapool_time = metapool_time;
			m_pmr_time = pmr_time;
			m_std_time = std_time;
		}
	}

	inline void print_summary()
	{
		std::cout << "\n--- overall summary (ecs pattern only) ---\n";
		std::cout << "metapool: " << m_metapool_time << " ms\n";
		std::cout << "pmr:      " << m_pmr_time << " ms\n";
		std::cout << "std:      " << m_std_time << " ms\n";

		double vs_pmr = m_pmr_time / m_metapool_time;
		double vs_std = m_std_time / m_metapool_time;

		std::cout << "\n--- performance comparison (ecs pattern) ---\n";
		std::cout << "metapool vs pmr: " << vs_pmr << "x " << (vs_pmr > 1.0 ? "faster" : "slower") << "\n";
		std::cout << "metapool vs std: " << vs_std << "x " << (vs_std > 1.0 ? "faster" : "slower") << "\n";
	}
};

inline std::unique_ptr<Benchmark> create_benchmark_intermediate()
{
	return std::make_unique<BenchmarkIntermediate>();
}
