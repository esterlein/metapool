#pragma once

#include <array>
#include <vector>
#include <chrono>
#include <cstring>
#include <iostream>
#include <memory_resource>

#include "benchmark.hpp"
#include "memory_model.hpp"



struct DummySmall
{
	int data[8] {};
	DummySmall() {std::memset(data, 0x11, sizeof(data));}
	~DummySmall() {std::memset(data, 0, sizeof(data));}
};

struct DummyMedium
{
	int data[256] {};
	DummyMedium() {std::memset(data, 0x22, sizeof(data));}
	~DummyMedium() {std::memset(data, 0, sizeof(data));}
};

struct DummyBig
{
	int data[4096] {};
	DummyBig() {std::memset(data, 0x33, sizeof(data));}
	~DummyBig() {std::memset(data, 0, sizeof(data));}
};


class BenchmarkIntermediate : public Benchmark
{
public:

	inline void setup() override
	{
		std::cout << "\n--- metapool memory model intermediate benchmark ---\n";
		std::cout << "\nrunning basic metapool tests...\n";

		Benchmark::basic_tests<hpr::mem::AllocatorType::Intermediate>();
	}

	inline void run() override
	{
		std::cout << "\n";
		run_pattern_raw_alloc("ecs", 32, 4, 5000, 100);
		std::cout << "\n";
		run_pattern_raw_alloc("renderer", 64, 4, 500, 100);
		std::cout << "\n";
		run_pattern_raw_alloc("ui", 48, 1, 256, 1000);

		hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Intermediate>().reset();

		std::cout << "\n\n";
		run_pattern_dummy_alloc<DummySmall>("ecs", 5000, 100);
		std::cout << "\n";
		run_pattern_dummy_construct<DummySmall>("ecs", 5000, 100);

		hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Intermediate>().reset();

		std::cout << "\n\n";
		run_pattern_dummy_alloc<DummyMedium>("renderer", 500, 100);
		std::cout << "\n";
		run_pattern_dummy_construct<DummyMedium>("renderer", 500, 100);

		hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Intermediate>().reset();

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
		hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Intermediate>().reset();
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


	void run_pattern_raw_alloc(const char* label, int base_size, int var_factor, int count, int frames)
	{
		run_raw_alloc_mpool(label, base_size, var_factor, count, frames);
		std::cout << "\n";
		run_raw_alloc_pmr(label, base_size, var_factor, count, frames);
		std::cout << "\n";
		run_raw_alloc_std(label, base_size, var_factor, count, frames);
		std::cout << "\n";
	}


	void run_raw_alloc_mpool(const char* label, int base_size, int var_factor, int count, int frames)
	{
		std::cout << "run metapool raw allocation..." << std::endl;

		auto& allocator = hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Intermediate>();
		std::vector<std::byte*> objects;
		objects.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				objects.push_back(allocator.alloc(base_size + (i % var_factor) * k_var_base, k_alignment));
			}
			for (auto* ptr : objects) allocator.free(ptr);
			objects.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::cout
			<< "metapool raw alloc time (" << label << "): "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms" << std::endl;
	}


	void run_raw_alloc_pmr(const char* label, int base_size, int var_factor, int count, int frames)
	{
		std::cout << "run pmr raw allocation..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<std::byte> allocator(&upstream);
		std::vector<std::byte*> objects;
		objects.reserve(count);
		
		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				objects.push_back(static_cast<std::byte*>(allocator.allocate(base_size + (i % var_factor) * k_var_base)));
			}
			for (auto* ptr : objects) allocator.deallocate(ptr, 0);
			objects.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::cout
			<< "pmr raw alloc time (" << label << "): "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms" << std::endl;
	}


	void run_raw_alloc_std(const char* label, int base_size, int var_factor, int count, int frames)
	{
		std::cout << "run std raw allocation..." << std::endl;

		std::allocator<std::byte> allocator;
		std::vector<std::byte*> objects;
		objects.reserve(count);
		
		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				objects.push_back(allocator.allocate(base_size + (i % var_factor) * k_var_base));
			}
			for (auto* ptr : objects) allocator.deallocate(ptr, base_size);
			objects.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::cout
			<< "std raw alloc time (" << label << "): "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms" << std::endl;
	}


	template<typename T>
	void run_pattern_dummy_alloc(const char* label, int count, int frames)
	{
		std::cout << "--- " << label << " dummy alloc/free pattern ---" << std::endl;

		std::cout << "\n";
		run_dummy_alloc_mpool<T>(count, frames);
		std::cout << "\n";
		run_dummy_alloc_pmr<T>(count, frames);
		std::cout << "\n";
		run_dummy_alloc_std<T>(count, frames);
	}


	template<typename T>
	void run_dummy_alloc_mpool(int count, int frames)
	{
		std::cout << "run metapool dummy allocation..." << std::endl;

		auto& allocator = hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Intermediate>();
		std::vector<std::byte*> objects;
		objects.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				objects.push_back(allocator.alloc(sizeof(T), k_alignment));
			}
			for (auto* ptr : objects) allocator.free(ptr);
			objects.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::cout
			<< "metapool alloc time: "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms" << std::endl;
	}


	template<typename T>
	void run_dummy_alloc_pmr(int count, int frames)
	{
		std::cout << "run pmr dummy allocation..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<std::byte> allocator(&upstream);
		std::vector<std::byte*> objects;
		objects.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				objects.push_back(static_cast<std::byte*>(allocator.allocate(sizeof(T))));
			}
			for (auto* ptr : objects) allocator.deallocate(ptr, 0);
			objects.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::cout
			<< "pmr alloc time: "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms" << std::endl;
	}


	template<typename T>
	void run_dummy_alloc_std(int count, int frames)
	{
		std::cout << "run std dummy allocation..." << std::endl;

		std::allocator<std::byte> allocator;
		std::vector<std::byte*> objects;
		objects.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				objects.push_back(allocator.allocate(sizeof(T)));
			}
			for (auto* ptr : objects) allocator.deallocate(ptr, sizeof(T));
			objects.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::cout
			<< "std alloc time: "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms" << std::endl;
	}


	template<typename T>
	void run_pattern_dummy_construct(const char* label, int count, int frames)
	{
		std::cout << "--- " << label << " dummy construct/destruct pattern ---" << std::endl;

		std::cout << "\n";
		run_construct_mpool<T>(count, frames);
		std::cout << "\n";
		run_construct_pmr<T>(count, frames);
		std::cout << "\n";
		run_construct_std<T>(count, frames);
	}


	template<typename T>
	void run_construct_mpool(int count, int frames)
	{
		std::cout << "run mpool dummy construct/destruct..." << std::endl;

		auto& allocator = hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Intermediate>();
		std::vector<T*> objects;
		objects.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				objects.push_back(allocator.construct<T>());
			}
			for (auto* ptr : objects) allocator.destruct(ptr);
			objects.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::cout
			<< "metapool construct/destruct time: "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms" << std::endl;
	}


	template<typename T>
	void run_construct_pmr(int count, int frames)
	{
		std::cout << "run pmr dummy construct/destruct..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<std::byte> allocator(&upstream);
		std::vector<T*> objects;
		objects.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				std::byte* raw = static_cast<std::byte*>(allocator.allocate(sizeof(T)));
				T* ptr = ::new (raw) T();
				objects.push_back(ptr);
			}
			for (auto* ptr : objects) {
				ptr->~T();
				allocator.deallocate(reinterpret_cast<std::byte*>(ptr), 0);
			}
			objects.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		std::cout
			<< "pmr construct/destruct time: "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms" << std::endl;
	}


	template<typename T>
	void run_construct_std(int count, int frames)
	{
		std::cout << "run std dummy construct/destruct..." << std::endl;

		std::allocator<T> allocator;
		std::vector<T*> objects;
		objects.reserve(count);
	
		auto start = std::chrono::high_resolution_clock::now();
		for (int frame = 0; frame < frames; ++frame) {
			for (int i = 0; i < count; ++i) {
				T* ptr = allocator.allocate(1);
				::new (static_cast<void*>(ptr)) T();
				objects.push_back(ptr);
			}
			for (auto* ptr : objects) {
				ptr->~T();
				allocator.deallocate(ptr, 1);
			}
			objects.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();
	
		std::cout
			<< "std construct/destruct time: "
			<< std::chrono::duration<double, std::milli>(end - start).count()
			<< " ms\n";
	}


	inline void print_summary() const
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
	
		double vs_pmr_raw = m_pmr_raw_time / m_mpool_raw_time;
		double vs_std_raw = m_std_raw_time / m_mpool_raw_time;

		double vs_pmr_alloc = m_pmr_dummy_alloc_time / m_mpool_dummy_alloc_time;
		double vs_std_alloc = m_std_dummy_alloc_time / m_mpool_dummy_alloc_time;

		double vs_pmr_construct = m_pmr_dummy_construct_time / m_mpool_dummy_construct_time;
		double vs_std_construct = m_std_dummy_construct_time / m_mpool_dummy_construct_time;

		std::cout << "\nraw alloc:\n";
		std::cout << "metapool vs pmr: " << vs_pmr_raw << "x " << (vs_pmr_raw > 1.0 ? "faster" : "slower") << "\n";
		std::cout << "metapool vs std: " << vs_std_raw << "x " << (vs_std_raw > 1.0 ? "faster" : "slower") << "\n";

		std::cout << "\ndummy alloc/free:\n";
		std::cout << "metapool vs pmr: " << vs_pmr_alloc << "x " << (vs_pmr_alloc > 1.0 ? "faster" : "slower") << "\n";
		std::cout << "metapool vs std: " << vs_std_alloc << "x " << (vs_std_alloc > 1.0 ? "faster" : "slower") << "\n";

		std::cout << "\ndummy construct/destruct:\n";
		std::cout << "metapool vs pmr: " << vs_pmr_construct << "x " << (vs_pmr_construct > 1.0 ? "faster" : "slower") << "\n";
		std::cout << "metapool vs std: " << vs_std_construct << "x " << (vs_std_construct > 1.0 ? "faster" : "slower") << "\n";
	}
};


inline std::unique_ptr<Benchmark> create_benchmark_intermediate()
{
	return std::make_unique<BenchmarkIntermediate>();
}
