#pragma once

#include <array>
#include <cstdint>
#include <vector>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string_view>
#include <memory_resource>

#include "benchmark.hpp"

#include "mtp_setup.hpp"
#include "mtp_memory.hpp"

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

	using System = mtp::BenchmarkIntermediateSystem;

	inline void setup() override
	{
		std::cout << "\n--- metapool memory model intermediate benchmark ---\n" << std::endl;
		std::cout << "running basic metapool tests..." << std::endl;

		Benchmark::basic_tests();
	}


	inline void teardown() override
	{
		std::cout << "\n";
		auto& allocator = mtp::get_system_allocator<System>();
		allocator.reset();
	}

private:

	const std::size_t k_alignment {16};
	const std::size_t k_var_base  {16};

	std::array<double, 3> m_raw_time_ecs    {};
	std::array<double, 3> m_raw_time_render {};
	std::array<double, 3> m_raw_time_ui     {};

	std::array<double, 3> m_dummy_alloc_time_ecs     {};
	std::array<double, 3> m_dummy_construct_time_ecs {};
	std::array<double, 3> m_dummy_emplace_time_ecs   {};

	std::array<double, 3> m_dummy_alloc_time_render     {};
	std::array<double, 3> m_dummy_construct_time_render {};
	std::array<double, 3> m_dummy_emplace_time_render   {};

	std::array<double, 3> m_dummy_alloc_time_ui     {};
	std::array<double, 3> m_dummy_construct_time_ui {};
	std::array<double, 3> m_dummy_emplace_time_ui   {};


	auto run_pattern_raw_alloc(std::string_view label, uint32_t base_size, uint32_t var_factor, std::size_t count, uint32_t frames)
	{
		std::array<double, 3> time;
		time[0] = run_raw_alloc_mtp(label, base_size, var_factor, count, frames);
		std::cout << std::endl;
		time[1] = run_raw_alloc_std(label, base_size, var_factor, count, frames);
		std::cout << std::endl;
		time[2] = run_raw_alloc_pmr(label, base_size, var_factor, count, frames);
		std::cout << std::endl;
		return time;
	}


	double run_raw_alloc_mtp(std::string_view label, uint32_t base_size, uint32_t var_factor, std::size_t count, uint32_t frames)
	{
		std::cout << "run metapool raw allocation..." << std::endl;

		auto& allocator = mtp::get_system_allocator<System>();
		auto vec = hpr::cntr::make_vector<std::byte*, System>();
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			for (auto i = 0; i < count; ++i) {
				vec.push_back(allocator.alloc(base_size + (i % var_factor) * k_var_base, k_alignment));
			}
			for (auto* ptr : vec) {
				allocator.free(ptr);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool raw alloc time (" << label << "): " << time << " ms" << std::endl;
		return time;
	}


	double run_raw_alloc_std(std::string_view label, std::size_t base_size, std::size_t var_factor, std::size_t count, uint32_t frames)
	{
		std::cout << "run std raw allocation..." << std::endl;

		std::allocator<std::byte*> block_allocator;
		std::allocator<std::byte> data_allocator;

		std::vector<std::byte*, std::allocator<std::byte*>> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			for (auto i = 0; i < count; ++i) {
				vec.push_back(data_allocator.allocate(base_size + (i % var_factor) * k_var_base));
			}
			for (auto* ptr : vec) {
				data_allocator.deallocate(ptr, base_size);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();
	
		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std raw alloc time (" << label << "): " << time << " ms" << std::endl;
		return time;
	}


	double run_raw_alloc_pmr(std::string_view label, std::size_t base_size, std::size_t var_factor, std::size_t count, uint32_t frames)
	{
		std::cout << "run pmr raw allocation..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<std::byte*> block_allocator(&upstream);
		std::pmr::polymorphic_allocator<std::byte> data_allocator(&upstream);

		std::pmr::vector<std::byte*> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			for (auto i = 0; i < count; ++i) {
				vec.push_back(static_cast<std::byte*>(
					data_allocator.allocate(base_size + (i % var_factor) * k_var_base)));
			}
			for (auto* ptr : vec) {
				data_allocator.deallocate(ptr, 0);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();
	
		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr raw alloc time (" << label << "): " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	std::array<double, 3> run_pattern_dummy_alloc(std::string_view label, std::size_t count, uint32_t frames)
	{
		std::cout << "--- " << label << " dummy alloc/free pattern ---" << std::endl;
		std::array<double, 3> time;
		std::cout << "\n";
		time[0] = run_dummy_alloc_mtp<T>(count, frames);
		std::cout << "\n";
		time[1] = run_dummy_alloc_std<T>(count, frames);
		std::cout << "\n";
		time[2] = run_dummy_alloc_pmr<T>(count, frames);
		return time;
	}

	template <typename T>
	double run_dummy_alloc_mtp(std::size_t count, uint32_t frames)
	{
		std::cout << "run metapool dummy allocation..." << std::endl;

		auto& allocator = mtp::get_system_allocator<System>();
		auto vec = hpr::cntr::make_vector<std::byte*, System>();
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			for (auto i = 0; i < count; ++i) {
				vec.push_back(allocator.alloc(sizeof(T), k_alignment));
			}
			for (auto* ptr : vec) {
				allocator.free(ptr);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool alloc time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	double run_dummy_alloc_std(std::size_t count, uint32_t frames)
	{
		std::cout << "run std dummy allocation..." << std::endl;

		std::allocator<std::byte*> block_allocator;
		std::allocator<std::byte> data_allocator;

		std::vector<std::byte*, std::allocator<std::byte*>> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			for (auto i = 0; i < count; ++i) {
				vec.push_back(data_allocator.allocate(sizeof(T)));
			}
			for (auto* ptr : vec) {
				data_allocator.deallocate(ptr, sizeof(T));
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std alloc time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	double run_dummy_alloc_pmr(std::size_t count, uint32_t frames)
	{
		std::cout << "run pmr dummy allocation..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<std::byte*> block_allocator(&upstream);
		std::pmr::polymorphic_allocator<std::byte> data_allocator(&upstream);

		std::pmr::vector<std::byte*> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			for (auto i = 0; i < count; ++i) {
				vec.push_back(static_cast<std::byte*>(data_allocator.allocate(sizeof(T))));
			}
			for (auto* ptr : vec) {
				data_allocator.deallocate(ptr, 0);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr alloc time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	std::array<double, 3> run_pattern_dummy_construct(std::string_view label, std::size_t count, uint32_t frames)
	{
		std::cout << "--- " << label << " dummy construct/destruct pattern ---" << std::endl;
		std::array<double, 3> time;
		std::cout << "\n";
		time[0] = run_dummy_construct_mtp<T>(count, frames);
		std::cout << "\n";
		time[1] = run_dummy_construct_std<T>(count, frames);
		std::cout << "\n";
		time[2] = run_dummy_construct_pmr<T>(count, frames);
		return time;
	}


	template <typename T>
	double run_dummy_construct_mtp(std::size_t count, uint32_t frames)
	{
		std::cout << "run mpool dummy construct/destruct..." << std::endl;

		auto& allocator = mtp::get_system_allocator<System>();
		auto vec = hpr::cntr::make_vector<T*, System>();
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			for (auto i = 0; i < count; ++i) {
				vec.push_back(allocator.construct<T>());
			}
			for (auto* ptr : vec) {
				allocator.destruct(ptr);
			}
			vec.clear();
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool construct/destruct time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	double run_dummy_construct_std(std::size_t count, uint32_t frames)
	{
		std::cout << "run std dummy construct/destruct..." << std::endl;

		std::allocator<T*> block_allocator;
		std::allocator<T> data_allocator;

		std::vector<T*, std::allocator<T*>> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			for (auto i = 0; i < count; ++i) {
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

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std construct/destruct time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	double run_dummy_construct_pmr(std::size_t count, uint32_t frames)
	{
		std::cout << "run pmr dummy construct/destruct..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::polymorphic_allocator<T*> block_allocator(&upstream);
		std::pmr::polymorphic_allocator<T> data_allocator(&upstream);

		std::pmr::vector<T*> vec {block_allocator};
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			for (auto i = 0; i < count; ++i) {
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

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr construct/destruct time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	std::array<double, 3> run_pattern_dummy_emplace(std::string_view label, std::size_t count, uint32_t frames)
	{
		std::cout << "--- " << label << " dummy emplace pattern ---" << std::endl;
		std::array<double, 3> time;
		std::cout << "\n";
		time[0] = run_dummy_emplace_mtp<T>(count, frames);
		std::cout << "\n";
		time[1] = run_dummy_emplace_std<T>(count, frames);
		std::cout << "\n";
		time[2] = run_dummy_emplace_pmr<T>(count, frames);
		return time;
	}


	template <typename T>
	double run_dummy_emplace_mtp(std::size_t count, uint32_t frames)
	{
		std::cout << "run metapool std::vector<T> in-place construction..." << std::endl;

		auto vec = hpr::cntr::make_vector<T*, System>();
		vec.reserve(count);

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			vec.clear();
			vec.reserve(count);
			for (auto i = 0; i < count; ++i) {
				vec.emplace_back();
			}
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool std::vector<T>::emplace time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	double run_dummy_emplace_std(std::size_t count, uint32_t frames)
	{
		std::cout << "run std::allocator vector<T> in-place construction..." << std::endl;

		std::vector<T> vec;

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			vec.clear();
			vec.reserve(count);
			for (auto i = 0; i < count; ++i) {
				vec.emplace_back();
			}
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std vector<T>::emplace time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	double run_dummy_emplace_pmr(std::size_t count, uint32_t frames)
	{
		std::cout << "run pmr::vector<T> in-place construction..." << std::endl;

		std::pmr::monotonic_buffer_resource upstream(std::pmr::get_default_resource());
		std::pmr::vector<T> vec {&upstream};

		auto start = std::chrono::high_resolution_clock::now();
		for (auto frame = 0; frame < frames; ++frame) {
			vec.clear();
			vec.reserve(count);
			for (auto i = 0; i < count; ++i) {
				vec.emplace_back();
			}
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr vector<T>::emplace time: " << time << " ms" << std::endl;
		return time;
	}

public:

	inline void run() override
	{
		std::cout << std::endl;
		m_raw_time_ecs = run_pattern_raw_alloc("ecs", 32, 4, 5000, 100);
		hpr::mem::AllocLogger::export_profile("alloc_raw_ecs.csv", true);
		std::cout << std::endl;
		m_raw_time_render = run_pattern_raw_alloc("render", 64, 4, 500, 100);
		hpr::mem::AllocLogger::export_profile("alloc_raw_render.csv", true);
		std::cout << std::endl;
		m_raw_time_ui = run_pattern_raw_alloc("ui", 48, 1, 256, 1000);
		hpr::mem::AllocLogger::export_profile("alloc_raw_ui.csv", true);

		mtp::get_system_allocator<System>().reset();

		std::cout << "\n\n";
		m_dummy_alloc_time_ecs = run_pattern_dummy_alloc<DummySmall>("ecs", 5000, 100);
		hpr::mem::AllocLogger::export_profile("alloc_dummysmall_ecs.csv", true);
		std::cout << "\n";
		m_dummy_construct_time_ecs = run_pattern_dummy_construct<DummySmall>("ecs", 5000, 100);
		hpr::mem::AllocLogger::export_profile("construct_dummysmall_ecs.csv", true);
		std::cout << "\n";
		m_dummy_emplace_time_ecs = run_pattern_dummy_emplace<DummySmall>("ecs", 5000, 100);
		hpr::mem::AllocLogger::export_profile("emplace_dummysmall_ecs.csv", true);

		mtp::get_system_allocator<System>().reset();

		std::cout << "\n\n";
		m_dummy_alloc_time_render = run_pattern_dummy_alloc<DummyMedium>("render", 500, 100);
		hpr::mem::AllocLogger::export_profile("alloc_dummymed_ecs.csv", true);
		std::cout << "\n";
		m_dummy_construct_time_render = run_pattern_dummy_construct<DummyMedium>("render", 500, 100);
		hpr::mem::AllocLogger::export_profile("construct_dummymed_ecs.csv", true);
		std::cout << "\n";
		m_dummy_emplace_time_render = run_pattern_dummy_emplace<DummyMedium>("render", 500, 100);
		hpr::mem::AllocLogger::export_profile("emplace_dummymed_ecs.csv", true);

		mtp::get_system_allocator<System>().reset();

		std::cout << "\n\n";
		m_dummy_alloc_time_ui = run_pattern_dummy_alloc<DummyBig>("ui", 256, 1000);
		hpr::mem::AllocLogger::export_profile("alloc_dummybig_ecs.csv", true);
		std::cout << "\n";
		m_dummy_construct_time_ui = run_pattern_dummy_construct<DummyBig>("ui", 256, 1000);
		hpr::mem::AllocLogger::export_profile("construct_dummybig_ecs.csv", true);
		std::cout << "\n";
		m_dummy_emplace_time_ui = run_pattern_dummy_emplace<DummyBig>("ui", 256, 1000);
		hpr::mem::AllocLogger::export_profile("emplace_dummybig_ecs.csv", true);

		mtp::get_system_allocator<System>().reset();

		std::cout << "\n\n";
		print_summary();
		std::cout << "\n";
	}

private:

	void print_summary() const
	{
		std::cout << "\n--- benchmark summary ---\n";
		auto print_pattern_results = [](
										const std::string& pattern_name,
										const std::array<double, 3>& raw_times,
										const std::array<double, 3>& alloc_times,
										const std::array<double, 3>& construct_times,
										const std::array<double, 3>& emplace_times
		){
			std::cout << "\n=== " << pattern_name << " pattern ===\n\n";

			std::cout << std::left << std::setw(25) << "strategy"
					  << std::setw(15) << "mtp (ms)"
					  << std::setw(15) << "std (ms)"
					  << std::setw(15) << "pmr (ms)"
					  << std::setw(20) << "mtp vs std"
					  << std::setw(20) << "mtp vs pmr"
					  << "\n";

			std::cout << std::string(110, '-') << "\n";

			auto format_cmp = [](double base, double val) -> std::string {
				double r = val / base;
				std::stringstream ss;
				ss << std::fixed << std::setprecision(2) << r << "x " << (r > 1.0 ? "faster" : "slower");
				return ss.str();
			};

			std::cout << std::left << std::setw(25) << "raw alloc"
					  << std::setw(15) << std::fixed << std::setprecision(4) << raw_times[0]
					  << std::setw(15) << std::fixed << std::setprecision(4) << raw_times[1]
					  << std::setw(15) << std::fixed << std::setprecision(4) << raw_times[2]
					  << std::setw(20) << format_cmp(raw_times[0], raw_times[1])
					  << std::setw(20) << format_cmp(raw_times[0], raw_times[2])
					  << "\n";

			std::cout << std::left << std::setw(25) << "dummy alloc/free"
					  << std::setw(15) << std::fixed << std::setprecision(4) << alloc_times[0]
					  << std::setw(15) << std::fixed << std::setprecision(4) << alloc_times[1]
					  << std::setw(15) << std::fixed << std::setprecision(4) << alloc_times[2]
					  << std::setw(20) << format_cmp(alloc_times[0], alloc_times[1])
					  << std::setw(20) << format_cmp(alloc_times[0], alloc_times[2])
					  << "\n";

			std::cout << std::left << std::setw(25) << "dummy construct/destruct"
					  << std::setw(15) << std::fixed << std::setprecision(4) << construct_times[0]
					  << std::setw(15) << std::fixed << std::setprecision(4) << construct_times[1]
					  << std::setw(15) << std::fixed << std::setprecision(4) << construct_times[2]
					  << std::setw(20) << format_cmp(construct_times[0], construct_times[1])
					  << std::setw(20) << format_cmp(construct_times[0], construct_times[2])
					  << "\n";

			std::cout << std::left << std::setw(25) << "dummy emplace/clear"
					  << std::setw(15) << std::fixed << std::setprecision(4) << emplace_times[0]
					  << std::setw(15) << std::fixed << std::setprecision(4) << emplace_times[1]
					  << std::setw(15) << std::fixed << std::setprecision(4) << emplace_times[2]
					  << std::setw(20) << format_cmp(emplace_times[0], emplace_times[1])
					  << std::setw(20) << format_cmp(emplace_times[0], emplace_times[2])
					  << "\n";
		};

		print_pattern_results("ecs", m_raw_time_ecs, m_dummy_alloc_time_ecs, m_dummy_construct_time_ecs, m_dummy_emplace_time_ecs);
		print_pattern_results("render", m_raw_time_render, m_dummy_alloc_time_render, m_dummy_construct_time_render, m_dummy_emplace_time_render);
		print_pattern_results("ui", m_raw_time_ui, m_dummy_alloc_time_ui, m_dummy_construct_time_ui, m_dummy_emplace_time_ui);
	}
};

inline std::unique_ptr<Benchmark> create_benchmark_intermediate()
{
	return std::make_unique<BenchmarkIntermediate>();
}
