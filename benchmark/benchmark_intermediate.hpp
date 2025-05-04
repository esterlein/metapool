#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string_view>
#include <memory_resource>

#include "benchmark.hpp"
#include "memory_api.hpp"
#include "container_factory.hpp"



struct DummySmall
{
	std::array<int, 8> data {};

	DummySmall()
	{
		std::memset(static_cast<void*>(data.data()), 0x11, sizeof(data));
	}

	~DummySmall()
	{
		std::memset(static_cast<void*>(data.data()), 0, sizeof(data));
	}
};

struct DummyMedium
{
	std::array<int, 256> data {};

	DummyMedium()
	{
		std::memset(static_cast<void*>(data.data()), 0x22, sizeof(data));
	}

	~DummyMedium()
	{
		std::memset(static_cast<void*>(data.data()), 0, sizeof(data));
	}
};

struct DummyBig
{
	std::array<int, 4096> data {};

	DummyBig()
	{
		std::memset(static_cast<void*>(data.data()), 0x33, sizeof(data));
	}

	~DummyBig()
	{
		std::memset(static_cast<void*>(data.data()), 0, sizeof(data));
	}
};


class BenchmarkIntermediate : public Benchmark
{
public:

	using System = hpr::mem::BenchmarkIntermediateSystem;

	inline void setup() override
	{
		std::cout << "\n--- metapool memory model intermediate benchmark ---\n" << std::endl;
		std::cout << "running basic metapool tests..." << std::endl;

		Benchmark::basic_tests();
	}

	inline void run() override
	{
		std::cout << std::endl;
		run_pattern_raw_alloc("ecs", 32, 4, 5000, 100);
		std::cout << std::endl;
		run_pattern_raw_alloc("renderer", 64, 4, 500, 100);
		std::cout << std::endl;
		run_pattern_raw_alloc("ui", 48, 1, 256, 1000);

		std::cout << "\n\n";
		run_pattern_dummy_alloc<DummySmall>("ecs", 5000, 100);
		std::cout << "\n";
		run_pattern_dummy_construct<DummySmall>("ecs", 5000, 100);

		std::cout << "\n\n";
		run_pattern_dummy_alloc<DummyMedium>("renderer", 500, 100);
		std::cout << "\n";
		run_pattern_dummy_construct<DummyMedium>("renderer", 500, 100);

		std::cout << "\n\n";
		run_pattern_dummy_alloc<DummyBig>("ui", 256, 1000);
		std::cout << "\n";
		run_pattern_dummy_construct<DummyBig>("ui", 256, 1000);

		std::cout << "\n\n";
		print_summary();
		std::cout << "\n";
	}

	inline void teardown() override
	{
		std::cout << "\n";
		auto& allocator = hpr::mem::get_system_allocator<System>();
		allocator.reset();
	}

private:

	const int k_alignment {16};
	const int k_var_base  {16};

	double m_mpool_raw_time {0.0};
	double m_pmr_raw_time   {0.0};
	double m_std_raw_time   {0.0};

	double m_mpool_dummy_alloc_time {0.0};
	double m_pmr_dummy_alloc_time   {0.0};
	double m_std_dummy_alloc_time   {0.0};

	double m_mpool_dummy_construct_time {0.0};
	double m_pmr_dummy_construct_time   {0.0};
	double m_std_dummy_construct_time   {0.0};


	void run_pattern_raw_alloc(std::string_view label, uint32_t base_size, uint32_t var_factor, std::size_t count, uint32_t frames)
	{
		run_raw_alloc_mpool(label, base_size, var_factor, count, frames);
		std::cout << std::endl;
		run_raw_alloc_std(label, base_size, var_factor, count, frames);
		std::cout << std::endl;
		run_raw_alloc_pmr(label, base_size, var_factor, count, frames);
		std::cout << std::endl;
	}


	void run_raw_alloc_mpool(std::string_view label, uint32_t base_size, uint32_t var_factor, std::size_t count, uint32_t frames)
	{
		std::cout << "run metapool raw allocation..." << std::endl;

		auto& allocator = hpr::mem::get_system_allocator<System>();
		auto vec = hpr::cntr::make_vector<std::byte*, System>(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i)
				vec.push_back(allocator.alloc(base_size + (i % var_factor) * k_var_base, k_alignment));
			for (auto* ptr : vec)
				allocator.free(ptr);
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_mpool_raw_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool raw alloc time (" << label << "): " << m_mpool_raw_time << " ms" << std::endl;
	}


	void run_raw_alloc_std(std::string_view label, std::size_t base_size, std::size_t var_factor, std::size_t count, uint32_t frames)
	{
		std::cout << "run std raw allocation..." << std::endl;

		std::allocator<std::byte*> block_allocator;
		std::allocator<std::byte> data_allocator;

		std::vector<std::byte*, std::allocator<std::byte*>> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				vec.push_back(data_allocator.allocate(base_size + (i % var_factor) * k_var_base));
			}
			for (auto* ptr : vec) {
				data_allocator.deallocate(ptr, base_size);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();
	
		m_std_raw_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std raw alloc time (" << label << "): " << m_std_raw_time << " ms" << std::endl;
	}


	void run_raw_alloc_pmr(std::string_view label, std::size_t base_size, std::size_t var_factor, std::size_t count, uint32_t frames)
	{
		std::cout << "run pmr raw allocation..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<std::byte*> block_allocator(&upstream);
		std::pmr::polymorphic_allocator<std::byte> data_allocator(&upstream);

		std::pmr::vector<std::byte*> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				vec.push_back(static_cast<std::byte*>(
					data_allocator.allocate(base_size + (i % var_factor) * k_var_base)));
			}
			for (auto* ptr : vec) {
				data_allocator.deallocate(ptr, 0);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();
	
		m_pmr_raw_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr raw alloc time (" << label << "): " << m_pmr_raw_time << " ms" << std::endl;
	}


	template <typename T>
	void run_pattern_dummy_alloc(std::string_view label, std::size_t count, uint32_t frames)
	{
		std::cout << "--- " << label << " dummy alloc/free pattern ---" << std::endl;
		std::cout << "\n";
		run_dummy_alloc_mpool<T>(count, frames);
		std::cout << "\n";
		run_dummy_alloc_std<T>(count, frames);
		std::cout << "\n";
		run_dummy_alloc_pmr<T>(count, frames);
	}

	template <typename T>
	void run_dummy_alloc_mpool(std::size_t count, uint32_t frames)
	{
		std::cout << "run metapool dummy allocation..." << std::endl;

		auto& allocator = hpr::mem::get_system_allocator<System>();
		auto vec = hpr::cntr::make_vector<std::byte*, System>(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				vec.push_back(allocator.alloc(sizeof(T), k_alignment));
			}
			for (auto* ptr : vec) {
				allocator.free(ptr);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_mpool_dummy_alloc_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool alloc time: " << m_mpool_dummy_alloc_time << " ms" << std::endl;
	}


	template <typename T>
	void run_dummy_alloc_std(std::size_t count, uint32_t frames)
	{
		std::cout << "run std dummy allocation..." << std::endl;

		std::allocator<std::byte*> block_allocator;
		std::allocator<std::byte> data_allocator;

		std::vector<std::byte*, std::allocator<std::byte*>> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				vec.push_back(data_allocator.allocate(sizeof(T)));
			}
			for (auto* ptr : vec) {
				data_allocator.deallocate(ptr, sizeof(T));
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_std_dummy_alloc_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std alloc time: " << m_std_dummy_alloc_time << " ms" << std::endl;
	}


	template <typename T>
	void run_dummy_alloc_pmr(std::size_t count, uint32_t frames)
	{
		std::cout << "run pmr dummy allocation..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<std::byte*> block_allocator(&upstream);
		std::pmr::polymorphic_allocator<std::byte> data_allocator(&upstream);

		std::pmr::vector<std::byte*> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				vec.push_back(static_cast<std::byte*>(data_allocator.allocate(sizeof(T))));
			}
			for (auto* ptr : vec) {
				data_allocator.deallocate(ptr, 0);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_pmr_dummy_alloc_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr alloc time: " << m_pmr_dummy_alloc_time << " ms" << std::endl;
	}


	template <typename T>
	void run_pattern_dummy_construct(std::string_view label, std::size_t count, uint32_t frames)
	{
		std::cout << "--- " << label << " dummy construct/destruct pattern ---" << std::endl;
		std::cout << "\n";
		run_construct_mpool<T>(count, frames);
		std::cout << "\n";
		run_construct_std<T>(count, frames);
		std::cout << "\n";
		run_construct_pmr<T>(count, frames);
	}


	template <typename T>
	void run_construct_mpool(std::size_t count, uint32_t frames)
	{
		std::cout << "run mpool dummy construct/destruct..." << std::endl;

		auto& allocator = hpr::mem::get_system_allocator<System>();
		auto vec = hpr::cntr::make_vector<T*, System>(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				vec.push_back(allocator.construct<T>());
			}
			for (auto* ptr : vec) {
				allocator.destruct(ptr);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_mpool_dummy_construct_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool construct/destruct time: " << m_mpool_dummy_construct_time << " ms" << std::endl;
	}


	template <typename T>
	void run_construct_std(std::size_t count, uint32_t frames)
	{
		std::cout << "run std dummy construct/destruct..." << std::endl;

		std::allocator<T*> block_allocator;
		std::allocator<T> data_allocator;

		std::vector<T*, std::allocator<T*>> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				T* ptr = data_allocator.allocate(1);
				::new (static_cast<void*>(ptr)) T();
				vec.push_back(ptr);
			}
			for (auto* ptr : vec) {
				ptr->~T();
				data_allocator.deallocate(ptr, 1);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_std_dummy_construct_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std construct/destruct time: " << m_std_dummy_construct_time << " ms" << std::endl;
	}


	template <typename T>
	void run_construct_pmr(std::size_t count, uint32_t frames)
	{
		std::cout << "run pmr dummy construct/destruct..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<T*> block_allocator(&upstream);
		std::pmr::polymorphic_allocator<T> data_allocator(&upstream);

		std::pmr::vector<T*> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				T* raw = static_cast<T*>(data_allocator.allocate(sizeof(T)));
				T* ptr = ::new (raw) T();
				vec.push_back(ptr);
			}
			for (auto* ptr : vec) {
				ptr->~T();
				data_allocator.deallocate(reinterpret_cast<T*>(ptr), 0);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		m_pmr_dummy_construct_time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr construct/destruct time: " << m_pmr_dummy_construct_time << " ms" << std::endl;
	}


	void print_summary() const
	{
		std::cout << "\n--- benchmark summary (ecs pattern only) ---\n";

		std::cout << "\nraw alloc:\n";
		std::cout << "metapool: " << m_mpool_raw_time << " ms\n";
		std::cout << "pmr:      " << m_pmr_raw_time   << " ms\n";
		std::cout << "std:      " << m_std_raw_time   << " ms\n";

		std::cout << "\ndummy alloc/free:\n";
		std::cout << "metapool: " << m_mpool_dummy_alloc_time << " ms\n";
		std::cout << "pmr:      " << m_pmr_dummy_alloc_time   << " ms\n";
		std::cout << "std:      " << m_std_dummy_alloc_time   << " ms\n";

		std::cout << "\ndummy construct/destruct:\n";
		std::cout << "metapool: " << m_mpool_dummy_construct_time << " ms\n";
		std::cout << "pmr:      " << m_pmr_dummy_construct_time   << " ms\n";
		std::cout << "std:      " << m_std_dummy_construct_time   << " ms\n";

		std::cout << "\n--- performance comparison (ecs pattern only) ---\n";

		auto cmp = [](double base, double val) {
			double r = val / base;
			std::cout << r << "x " << (r > 1.0 ? "slower" : "faster") << "\n";
		};

		std::cout << "\nraw alloc:\n";
		std::cout << "metapool vs pmr: "; cmp(m_mpool_raw_time, m_pmr_raw_time);
		std::cout << "metapool vs std: "; cmp(m_mpool_raw_time, m_std_raw_time);

		std::cout << "\ndummy alloc/free:\n";
		std::cout << "metapool vs pmr: "; cmp(m_mpool_dummy_alloc_time, m_pmr_dummy_alloc_time);
		std::cout << "metapool vs std: "; cmp(m_mpool_dummy_alloc_time, m_std_dummy_alloc_time);

		std::cout << "\ndummy construct/destruct:\n";
		std::cout << "metapool vs pmr: "; cmp(m_mpool_dummy_construct_time, m_pmr_dummy_construct_time);
		std::cout << "metapool vs std: "; cmp(m_mpool_dummy_construct_time, m_std_dummy_construct_time);
	}
};

inline std::unique_ptr<Benchmark> create_benchmark_intermediate()
{
	return std::make_unique<BenchmarkIntermediate>();
}
