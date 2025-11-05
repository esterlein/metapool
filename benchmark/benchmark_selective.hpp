#pragma once

#include <array>
#include <chrono>
#include <random>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <string_view>
#include <memory_resource>

#include "mtp_memory.hpp"

#include "benchmark.hpp"



struct SelectiveTiny
{
	std::array<int, 2> data {};
	SelectiveTiny()  { touch_prefix(this, 8); }
	~SelectiveTiny() { touch_prefix(this, 8); }
};

struct SelectiveSmall
{
	std::array<int, 8> data {};
	SelectiveSmall()  { touch_prefix(this, 32); }
	~SelectiveSmall() { touch_prefix(this, 32); }
};

struct SelectiveMedium
{
	std::array<int, 32> data {};
	SelectiveMedium()  { touch_prefix(this, 128); }
	~SelectiveMedium() { touch_prefix(this, 128); }
};

struct SelectiveLarge
{
	std::array<int, 256> data {};
	SelectiveLarge()  { touch_prefix(this, 32); }
	~SelectiveLarge() { touch_prefix(this, 32); }
};

struct SelectiveXL
{
	std::array<int, 1024> data {};
	SelectiveXL()  { touch_prefix(this, 256); }
	~SelectiveXL() { touch_prefix(this, 256); }
};

struct SelectiveXXL
{
	std::array<int, 4096> data {};
	SelectiveXXL()  { touch_step(this, sizeof(*this), 256, 4096); }
	~SelectiveXXL() { touch_step(this, sizeof(*this), 256, 4096); }
};

struct SelectiveGiant
{
	std::array<int, 8192> data {};
	SelectiveGiant()  { touch_scatter(this, sizeof(*this), 64, 0xC0FFEE); }
	~SelectiveGiant() { touch_scatter(this, sizeof(*this), 64, 0xC0FFEE); }
};



class BenchmarkSelective : public Benchmark
{
private:

	using Set = mtp::metaset <

		mtp::def<mtp::capf::flat, 5000, 8,        24,        24>,
		mtp::def<mtp::capf::flat, 5000, 8,        40,        40>,
		mtp::def<mtp::capf::flat, 5000, 8,        72,        72>,
		mtp::def<mtp::capf::flat, 5000, 8,       136,       136>,
		mtp::def<mtp::capf::flat, 5000, 8,       264,       264>,
		mtp::def<mtp::capf::flat, 5000, 8,       520,       520>,
		mtp::def<mtp::capf::flat, 5000, 8,      1032,      1032>,
		mtp::def<mtp::capf::flat, 5000, 8,      4104,      4104>,
		mtp::def<mtp::capf::flat, 5000, 8,     16392,     16392>,
		mtp::def<mtp::capf::flat, 5000, 8,     32776,     32776>,
		mtp::def<mtp::capf::flat, 5000, 8,     65544,     65544>,
		mtp::def<mtp::capf::flat,    2, 8,  81960000,  81960000>,
		mtp::def<mtp::capf::flat,    2, 8, 163840008, 163840008>,
		mtp::def<mtp::capf::flat,    2, 8, 268435464, 268435464>
	>;

public:

	inline void setup() override
	{
		std::cout << "\n\n\n--- METAPOOL SELECTIVE BENCHMARK ---\n" << std::endl;

		m_trash.resize(trash_size, 0x55);
	}

	inline void run() override
	{
		std::array<size_t, raw_test_count> sizes = {
			16, 32, 64, 128, 256, 512, 1024, 4096, 16384, 65536
		};

		std::cout << "\nrunning raw allocation tests...\n" << std::endl;

		for (size_t i = 0; i < sizes.size(); ++i) {
			size_t size = sizes[i];

			auto times = run_raw_alloc(std::to_string(size), size, count, frames);
			m_raw_results[i * 2 + 0] = times[0]; // malloc
			m_raw_results[i * 2 + 1] = times[1]; // metapool
		}

		std::cout << "\nrunning construct path tests...\n" << std::endl;

		m_construct_tiny[0]   = run_construct_std<SelectiveTiny>(count, frames);
		m_construct_tiny[1]   = run_construct_pmr<SelectiveTiny>(count, frames);
		m_construct_tiny[2]   = run_construct_mtp<SelectiveTiny>(count, frames);

		m_construct_small[0]  = run_construct_std<SelectiveSmall>(count, frames);
		m_construct_small[1]  = run_construct_pmr<SelectiveSmall>(count, frames);
		m_construct_small[2]  = run_construct_mtp<SelectiveSmall>(count, frames);

		m_construct_medium[0] = run_construct_std<SelectiveMedium>(count, frames);
		m_construct_medium[1] = run_construct_pmr<SelectiveMedium>(count, frames);
		m_construct_medium[2] = run_construct_mtp<SelectiveMedium>(count, frames);

		m_construct_large[0]  = run_construct_std<SelectiveLarge>(count, frames);
		m_construct_large[1]  = run_construct_pmr<SelectiveLarge>(count, frames);
		m_construct_large[2]  = run_construct_mtp<SelectiveLarge>(count, frames);

		m_construct_xl[0]     = run_construct_std<SelectiveXL>(count, frames);
		m_construct_xl[1]     = run_construct_pmr<SelectiveXL>(count, frames);
		m_construct_xl[2]     = run_construct_mtp<SelectiveXL>(count, frames);

		m_construct_xxl[0]    = run_construct_std<SelectiveXXL>(count, frames);
		m_construct_xxl[1]    = run_construct_pmr<SelectiveXXL>(count, frames);
		m_construct_xxl[2]    = run_construct_mtp<SelectiveXXL>(count, frames);

		m_construct_giant[0]  = run_construct_std<SelectiveGiant>(count, frames);
		m_construct_giant[1]  = run_construct_pmr<SelectiveGiant>(count, frames);
		m_construct_giant[2]  = run_construct_mtp<SelectiveGiant>(count, frames);

		mtp::export_trace("trace/construct.csv");

		std::cout << "\nrunning emplace tests...\n" << std::endl;

		m_emplace_tiny[0]     = run_emplace_std<SelectiveTiny>(count, frames);
		m_emplace_tiny[1]     = run_emplace_pmr<SelectiveTiny>(count, frames);
		m_emplace_tiny[2]     = run_emplace_mtp<SelectiveTiny>(count, frames);

		m_emplace_small[0]    = run_emplace_std<SelectiveSmall>(count, frames);
		m_emplace_small[1]    = run_emplace_pmr<SelectiveSmall>(count, frames);
		m_emplace_small[2]    = run_emplace_mtp<SelectiveSmall>(count, frames);

		m_emplace_medium[0]   = run_emplace_std<SelectiveMedium>(count, frames);
		m_emplace_medium[1]   = run_emplace_pmr<SelectiveMedium>(count, frames);
		m_emplace_medium[2]   = run_emplace_mtp<SelectiveMedium>(count, frames);

		m_emplace_large[0]    = run_emplace_std<SelectiveLarge>(count, frames);
		m_emplace_large[1]    = run_emplace_pmr<SelectiveLarge>(count, frames);
		m_emplace_large[2]    = run_emplace_mtp<SelectiveLarge>(count, frames);

		m_emplace_xl[0]       = run_emplace_std<SelectiveXL>(count, frames);
		m_emplace_xl[1]       = run_emplace_pmr<SelectiveXL>(count, frames);
		m_emplace_xl[2]       = run_emplace_mtp<SelectiveXL>(count, frames);

		m_emplace_xxl[0]      = run_emplace_std<SelectiveXXL>(count, frames);
		m_emplace_xxl[1]      = run_emplace_pmr<SelectiveXXL>(count, frames);
		m_emplace_xxl[2]      = run_emplace_mtp<SelectiveXXL>(count, frames);

		m_emplace_giant[0]    = run_emplace_std<SelectiveGiant>(count, frames);
		m_emplace_giant[1]    = run_emplace_pmr<SelectiveGiant>(count, frames);
		m_emplace_giant[2]    = run_emplace_mtp<SelectiveGiant>(count, frames);

		mtp::export_trace("trace/emplace.csv");

		std::cout << "\nrunning churn tests...\n" << std::endl;

		m_churn_tiny[0]       = run_churn_std<SelectiveTiny, count>(frames);
		m_churn_tiny[1]       = run_churn_pmr<SelectiveTiny, count>(frames);
		m_churn_tiny[2]       = run_churn_mtp<SelectiveTiny, count>(frames);
		
		m_churn_small[0]      = run_churn_std<SelectiveSmall, count>(frames);
		m_churn_small[1]      = run_churn_pmr<SelectiveSmall, count>(frames);
		m_churn_small[2]      = run_churn_mtp<SelectiveSmall, count>(frames);
		
		m_churn_medium[0]     = run_churn_std<SelectiveMedium, count>(frames);
		m_churn_medium[1]     = run_churn_pmr<SelectiveMedium, count>(frames);
		m_churn_medium[2]     = run_churn_mtp<SelectiveMedium, count>(frames);
		
		m_churn_large[0]      = run_churn_std<SelectiveLarge, count>(frames);
		m_churn_large[1]      = run_churn_pmr<SelectiveLarge, count>(frames);
		m_churn_large[2]      = run_churn_mtp<SelectiveLarge, count>(frames);
		
		m_churn_xl[0]         = run_churn_std<SelectiveXL, count>(frames);
		m_churn_xl[1]         = run_churn_pmr<SelectiveXL, count>(frames);
		m_churn_xl[2]         = run_churn_mtp<SelectiveXL, count>(frames);
		
		m_churn_xxl[0]        = run_churn_std<SelectiveXXL, count>(frames);
		m_churn_xxl[1]        = run_churn_pmr<SelectiveXXL, count>(frames);
		m_churn_xxl[2]        = run_churn_mtp<SelectiveXXL, count>(frames);
		
		m_churn_giant[0]      = run_churn_std<SelectiveGiant, count>(frames);
		m_churn_giant[1]      = run_churn_pmr<SelectiveGiant, count>(frames);
		m_churn_giant[2]      = run_churn_mtp<SelectiveGiant, count>(frames);

		mtp::export_trace("trace/churn.csv");

		std::cout << '\n';
		print_summary_raw(sizes);
		print_summary_group("--- construct path", construct_entries);
		print_summary_group("--- emplace / grow", emplace_entries);
		print_summary_group("--- churn simulation", churn_entries);
		std::cout << '\n';
	}

	inline void teardown() override
	{
		auto& metapool = mtp::get_allocator<Set>();
		metapool.reset();
		std::cout << "\n";
	}

private:

	static constexpr size_t count  = 5000;
	static constexpr size_t frames = 100;

	static constexpr size_t alignment      = 8;
	static constexpr size_t max_pointers   = 10000;
	static constexpr size_t raw_test_count = 10;

	std::array<double, raw_test_count * 2> m_raw_results {};

	std::array<double, 3> m_construct_tiny   {};
	std::array<double, 3> m_construct_small  {};
	std::array<double, 3> m_construct_medium {};
	std::array<double, 3> m_construct_large  {};
	std::array<double, 3> m_construct_xl     {};
	std::array<double, 3> m_construct_xxl    {};
	std::array<double, 3> m_construct_giant  {};

	std::array<double, 3> m_emplace_tiny     {};
	std::array<double, 3> m_emplace_small    {};
	std::array<double, 3> m_emplace_medium   {};
	std::array<double, 3> m_emplace_large    {};
	std::array<double, 3> m_emplace_xl       {};
	std::array<double, 3> m_emplace_xxl      {};
	std::array<double, 3> m_emplace_giant    {};

	std::array<double, 3> m_churn_tiny       {};
	std::array<double, 3> m_churn_small      {};
	std::array<double, 3> m_churn_medium     {};
	std::array<double, 3> m_churn_large      {};
	std::array<double, 3> m_churn_xl         {};
	std::array<double, 3> m_churn_xxl        {};
	std::array<double, 3> m_churn_giant      {};

	static constexpr size_t trash_size = 4 * 1024 * 1024;
	std::vector<uint8_t> m_trash;


	struct BenchmarkEntry
	{
		const char* name;
		size_t size;
		const std::array<double, 3>& times;
	};

	std::array<BenchmarkEntry, 7> construct_entries {{
		{ "SelectiveTiny",    sizeof(SelectiveTiny),    m_construct_tiny  },
		{ "SelectiveSmall",   sizeof(SelectiveSmall),   m_construct_small },
		{ "SelectiveMedium",  sizeof(SelectiveMedium),  m_construct_medium },
		{ "SelectiveLarge",   sizeof(SelectiveLarge),   m_construct_large },
		{ "SelectiveXL",      sizeof(SelectiveXL),      m_construct_xl },
		{ "SelectiveXXL",     sizeof(SelectiveXXL),     m_construct_xxl },
		{ "SelectiveGiant",   sizeof(SelectiveGiant),   m_construct_giant },
	}};

	std::array<BenchmarkEntry, 7> emplace_entries {{
		{ "SelectiveTiny",    sizeof(SelectiveTiny),    m_emplace_tiny  },
		{ "SelectiveSmall",   sizeof(SelectiveSmall),   m_emplace_small },
		{ "SelectiveMedium",  sizeof(SelectiveMedium),  m_emplace_medium },
		{ "SelectiveLarge",   sizeof(SelectiveLarge),   m_emplace_large },
		{ "SelectiveXL",      sizeof(SelectiveXL),      m_emplace_xl },
		{ "SelectiveXXL",     sizeof(SelectiveXXL),     m_emplace_xxl },
		{ "SelectiveGiant",   sizeof(SelectiveGiant),   m_emplace_giant },
	}};

	std::array<BenchmarkEntry, 7> churn_entries {{
		{ "SelectiveTiny",    sizeof(SelectiveTiny),    m_churn_tiny  },
		{ "SelectiveSmall",   sizeof(SelectiveSmall),   m_churn_small },
		{ "SelectiveMedium",  sizeof(SelectiveMedium),  m_churn_medium },
		{ "SelectiveLarge",   sizeof(SelectiveLarge),   m_churn_large },
		{ "SelectiveXL",      sizeof(SelectiveXL),      m_churn_xl },
		{ "SelectiveXXL",     sizeof(SelectiveXXL),     m_churn_xxl },
		{ "SelectiveGiant",   sizeof(SelectiveGiant),   m_churn_giant },
	}};


	inline void flush_cache()
	{
		uint64_t sum = 0;
		for (size_t i = 0; i < trash_size; i += 64) {
			sum += m_trash[i];
		}
		if (sum == 0xdeadbeef) std::abort();
	}


	template <typename T, size_t N>
	void print_deltas(const std::array<T*, N>& ptrs, size_t count, const char* label)
	{
		std::vector<uintptr_t> raw;
		raw.reserve(count);
		for (size_t i = 0; i < count; ++i) {
			if (ptrs[i])
				raw.push_back(reinterpret_cast<uintptr_t>(ptrs[i]));
		}
	
		std::cout << "\n--- deltas (" << label << ") ---\n";
		for (size_t i = 1; i < raw.size(); ++i) {
			std::ptrdiff_t delta = raw[i] - raw[i - 1];
			std::cout << delta << " ";
		}
		std::cout << "\n";
	}


	void print_summary_group(std::string_view label, std::span<const BenchmarkEntry> entries)
	{
		auto ratio_str = [](double base, double val) -> std::string {
			if (base == 0.0) return "-";
			double r = val >= base ? val / base : base / val;
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(2) << r << "x "
			    << (val > base ? "slower" : "faster");
			return oss.str();
		};

		struct Row {
			std::string label;
			std::string std_time;
			std::string pmr_time;
			std::string mtp_time;
			std::string vs_std;
			std::string vs_pmr;
		};

		std::vector<Row> rows;
		size_t col0 = label.length();
		size_t col1 = std::strlen("std (ms)");
		size_t col2 = std::strlen("pmr (ms)");
		size_t col3 = std::strlen("mtp (ms)");
		size_t col4 = std::strlen("mtp vs std");
		size_t col5 = std::strlen("mtp vs pmr");

		for (const auto& e : entries) {
			std::ostringstream label_os, s0, s1, s2;
			label_os << e.name << " (" << e.size << ")";
			s0 << std::fixed << std::setprecision(2) << e.times[0];
			s1 << std::fixed << std::setprecision(2) << e.times[1];
			s2 << std::fixed << std::setprecision(2) << e.times[2];

			Row r;
			r.label    = label_os.str();
			r.std_time = s0.str();
			r.pmr_time = s1.str();
			r.mtp_time = s2.str();
			r.vs_std   = ratio_str(e.times[0], e.times[2]);
			r.vs_pmr   = ratio_str(e.times[1], e.times[2]);

			col0 = std::max(col0, r.label.length());
			col1 = std::max(col1, r.std_time.length());
			col2 = std::max(col2, r.pmr_time.length());
			col3 = std::max(col3, r.mtp_time.length());
			col4 = std::max(col4, r.vs_std.length());
			col5 = std::max(col5, r.vs_pmr.length());

			rows.push_back(std::move(r));
		}

		constexpr int gap = 4;
		auto pad = [](size_t w) { return static_cast<int>(w + gap); };

		std::cout << "\n" << std::left
			<< std::setw(pad(col0)) << label
			<< std::setw(pad(col1)) << "std (ms)"
			<< std::setw(pad(col2)) << "pmr (ms)"
			<< std::setw(pad(col3)) << "mtp (ms)"
			<< std::setw(pad(col4)) << "mtp vs std"
			<< "mtp vs pmr" << "\n";

		std::cout << std::string(col0 + col1 + col2 + col3 + col4 + col5 + gap * 5, '-') << "\n";

		for (const auto& r : rows) {
			std::cout << std::left
				<< std::setw(pad(col0)) << r.label
				<< std::setw(pad(col1)) << r.std_time
				<< std::setw(pad(col2)) << r.pmr_time
				<< std::setw(pad(col3)) << r.mtp_time
				<< std::setw(pad(col4)) << r.vs_std
				<< r.vs_pmr << "\n";
		}
	}


	template <typename AllocFunc, typename FreeFunc>
	double run_raw_loop(
		std::string_view label,
		size_t size,
		size_t count,
		uint32_t frames,
		AllocFunc alloc,
		FreeFunc free
	)
	{
		std::cout << "run " << label << " raw allocation (size: " << size << ")..." << std::endl;

		std::array<void*, max_pointers> pointers;
		if (count > max_pointers) {
			std::cerr << "count exceeds max pointer storage" << std::endl;
			return 0.0;
		}

		flush_cache();

		auto start = std::chrono::high_resolution_clock::now();

		for (uint32_t frame = 0; frame < frames; ++frame) {
			for (size_t i = 0; i < count; ++i)
				pointers[i] = alloc(size);
			for (size_t i = 0; i < count; ++i)
				free(pointers[i], size);
		}

		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << label << " raw alloc time: " << time << " ms" << std::endl;
		return time;
	}


	std::array<double, 2> run_raw_alloc(std::string_view label, size_t size, size_t count, uint32_t frames)
	{
		std::array<double, 2> time;

		time[0] = run_raw_loop("malloc", size, count, frames,
			[](size_t s) { return std::malloc(s); },
			[](void* p, size_t) { std::free(p); }
		);

		auto& metapool = mtp::get_allocator<Set>();
		time[1] = run_raw_loop("metapool", size, count, frames,
			[&metapool](size_t s) { return metapool.alloc(s, alignment); },
			[&metapool](void* p, size_t) { metapool.free(static_cast<std::byte*>(p)); }
		);

		return time;
	}


	void print_summary_raw(const std::array<size_t, raw_test_count>& sizes)
	{
		auto ratio_str = [](double base, double val) -> std::string {
			if (base == 0.0) return "-";
			double r = val >= base ? val / base : base / val;
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(2) << r << "x "
				<< (val > base ? "slower" : "faster");
			return oss.str();
		};

		struct Row {
			std::string size;
			std::string malloc_time;
			std::string mtp_time;
			std::string ratio;
		};

		std::vector<Row> rows;
		size_t col0 = std::strlen("--- raw");
		size_t col1 = std::strlen("malloc (ms)");
		size_t col2 = std::strlen("mtp (ms)");
		size_t col3 = std::strlen("mtp vs malloc");

		for (size_t i = 0; i < raw_test_count; ++i) {
			double malloc_time = m_raw_results[i * 2 + 0];
			double mtp_time    = m_raw_results[i * 2 + 1];

			std::ostringstream oss_malloc, oss_mtp;
			oss_malloc << std::fixed << std::setprecision(4) << malloc_time;
			oss_mtp << std::fixed << std::setprecision(4) << mtp_time;

			Row r;
			r.size         = std::to_string(sizes[i]);
			r.malloc_time  = oss_malloc.str();
			r.mtp_time     = oss_mtp.str();
			r.ratio        = ratio_str(malloc_time, mtp_time);

			col0 = std::max(col0, r.size.length());
			col1 = std::max(col1, r.malloc_time.length());
			col2 = std::max(col2, r.mtp_time.length());
			col3 = std::max(col3, r.ratio.length());

			rows.push_back(std::move(r));
		}

		std::cout << std::left
			<< std::setw(col0 + 4) << "--- raw"
			<< std::setw(col1 + 4) << "malloc (ms)"
			<< std::setw(col2 + 4) << "mtp (ms)"
			<< "mtp vs malloc" << "\n";

		std::cout << std::string(col0 + col1 + col2 + col3 + 12, '-') << "\n";

		for (const auto& r : rows) {
			std::cout << std::left
				<< std::setw(col0 + 4) << r.size
				<< std::setw(col1 + 4) << r.malloc_time
				<< std::setw(col2 + 4) << r.mtp_time
				<< r.ratio << "\n";
		}
	}


	template <typename T>
	double run_construct_std(size_t count, uint32_t frames)
	{
		std::cout << "run std construct/destruct (size: " << sizeof(T) << ")..." << std::endl;

		std::allocator<T> alloc;
		std::array<T*, max_pointers> ptrs;

		flush_cache();

		auto start = std::chrono::high_resolution_clock::now();
		for (uint32_t frame = 0; frame < frames; ++frame) {
			for (size_t i = 0; i < count; ++i) {
				T* p = alloc.allocate(1);
				ptrs[i] = new (p) T();
			}
			for (size_t i = 0; i < count; ++i) {
				ptrs[i]->~T();
				alloc.deallocate(ptrs[i], 1);
			}
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std construct time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	double run_construct_pmr(size_t count, uint32_t frames)
	{
		std::cout << "run pmr construct/destruct (size: " << sizeof(T) << ")..." << std::endl;

		std::pmr::unsynchronized_pool_resource pool;
		std::pmr::polymorphic_allocator<T> alloc(&pool);
		std::array<T*, max_pointers> ptrs;

		flush_cache();

		auto start = std::chrono::high_resolution_clock::now();
		for (uint32_t frame = 0; frame < frames; ++frame) {
			for (size_t i = 0; i < count; ++i) {
				T* p = alloc.allocate(1);
				ptrs[i] = new (p) T();
			}
			for (size_t i = 0; i < count; ++i) {
				ptrs[i]->~T();
				alloc.deallocate(ptrs[i], 1);
			}
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr construct time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	double run_construct_mtp(size_t count, uint32_t frames)
	{
		std::cout << "run metapool construct/destruct (size: " << sizeof(T) << ")..." << std::endl;

		auto& metapool = mtp::get_allocator<Set>();
		std::array<T*, max_pointers> ptrs;

		flush_cache();

		auto start = std::chrono::high_resolution_clock::now();
		for (uint32_t frame = 0; frame < frames; ++frame) {
			for (size_t i = 0; i < count; ++i)
				ptrs[i] = metapool.construct<T>();

			for (size_t i = 0; i < count; ++i)
				metapool.destruct(ptrs[i]);
		}
		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool construct time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T>
	double run_emplace_std(size_t count, uint32_t frames)
	{
		std::cout << "run std::vector emplace (size: " << sizeof(T) << ")..." << std::endl;

		std::vector<T> vec;
		double total_ms = 0.0;

		flush_cache();

		for (uint32_t frame = 0; frame < frames; ++frame) {
			vec.clear();

			auto start = std::chrono::high_resolution_clock::now();
			for (size_t i = 0; i < count; ++i)
				vec.emplace_back();
			auto end = std::chrono::high_resolution_clock::now();

			total_ms += std::chrono::duration<double, std::milli>(end - start).count();
		}

		std::cout << "std emplace time: " << total_ms << " ms" << std::endl;
		return total_ms;
	}


	template <typename T>
	double run_emplace_pmr(size_t count, uint32_t frames)
	{
		std::cout << "run pmr::vector emplace (size: " << sizeof(T) << ")..." << std::endl;

		std::pmr::unsynchronized_pool_resource pool;
		std::pmr::vector<T> vec(&pool);
		double total_ms = 0.0;

		flush_cache();

		for (uint32_t frame = 0; frame < frames; ++frame) {
			vec.clear();

			auto start = std::chrono::high_resolution_clock::now();
			for (size_t i = 0; i < count; ++i)
				vec.emplace_back();
			auto end = std::chrono::high_resolution_clock::now();

			total_ms += std::chrono::duration<double, std::milli>(end - start).count();
		}

		std::cout << "pmr emplace time: " << total_ms << " ms" << std::endl;
		return total_ms;
	}


	template <typename T>
	double run_emplace_mtp(size_t count, uint32_t frames)
	{
		std::cout << "run mtp::vault emplace (size: " << sizeof(T) << ")..." << std::endl;

		mtp::vault<T, Set> vlt;

		double total_ms = 0.0;

		flush_cache();

		for (uint32_t frame = 0; frame < frames; ++frame) {
			vlt.clear();

			auto start = std::chrono::high_resolution_clock::now();
			for (size_t i = 0; i < count; ++i)
				vlt.emplace_back();
			auto end = std::chrono::high_resolution_clock::now();

			total_ms += std::chrono::duration<double, std::milli>(end - start).count();
		}

		std::cout << "mtp emplace time: " << total_ms << " ms" << std::endl;
		return total_ms;
	}


	template <typename T, size_t Count>
	double run_churn_std(uint32_t frames)
	{
		std::cout << "run std churn simulation (size: " << sizeof(T) << ")..." << std::endl;

		std::allocator<T> alloc;
		std::array<T*, Count> ptrs {};
		size_t alive = 0;

		std::mt19937 rng(std::random_device {}());
		std::uniform_real_distribution<> dist(0.0, 1.0);

		auto start = std::chrono::high_resolution_clock::now();

		for (size_t i = 0; i < Count; ++i) {
			T* p = alloc.allocate(1);
			std::construct_at(p);
			ptrs[alive++] = p;
		}

		for (uint32_t f = 0; f < frames; ++f) {
			size_t next = 0;

			for (size_t i = 0; i < alive; ++i) {
				if (dist(rng) > 0.2) {
					std::destroy_at(ptrs[i]);
					alloc.deallocate(ptrs[i], 1);
				}
				else {
					ptrs[next++] = ptrs[i];
				}
			}
			while (next < Count) {
				T* p = alloc.allocate(1);
				std::construct_at(p);
				ptrs[next++] = p;
			}

			alive = next;
		}
		for (size_t i = 0; i < alive; ++i) {
			std::destroy_at(ptrs[i]);
			alloc.deallocate(ptrs[i], 1);
		}

		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "std churn time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T, size_t Count>
	double run_churn_pmr(uint32_t frames)
	{
		std::cout << "run pmr churn simulation (size: " << sizeof(T) << ")..." << std::endl;

		std::pmr::unsynchronized_pool_resource pool;
		std::pmr::polymorphic_allocator<T> alloc(&pool);
		std::array<T*, Count> ptrs {};
		size_t alive = 0;

		std::mt19937 rng(std::random_device {}());
		std::uniform_real_distribution<> dist(0.0, 1.0);

		auto start = std::chrono::high_resolution_clock::now();

		for (size_t i = 0; i < Count; ++i) {
			T* p = alloc.allocate(1);
			std::construct_at(p);
			ptrs[alive++] = p;
		}

		for (uint32_t f = 0; f < frames; ++f) {
			size_t next = 0;

			for (size_t i = 0; i < alive; ++i) {
				if (dist(rng) > 0.2) {
					std::destroy_at(ptrs[i]);
					alloc.deallocate(ptrs[i], 1);
				}
				else {
					ptrs[next++] = ptrs[i];
				}
			}
			while (next < Count) {
				T* p = alloc.allocate(1);
				std::construct_at(p);
				ptrs[next++] = p;
			}

			alive = next;
		}
		for (size_t i = 0; i < alive; ++i) {
			std::destroy_at(ptrs[i]);
			alloc.deallocate(ptrs[i], 1);
		}

		auto end = std::chrono::high_resolution_clock::now();

		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "pmr churn time: " << time << " ms" << std::endl;
		return time;
	}


	template <typename T, size_t Count>
	double run_churn_mtp(uint32_t frames)
	{
		std::cout << "run metapool churn simulation (size: " << sizeof(T) << ")..." << std::endl;

		auto& metapool = mtp::get_allocator<Set>();
		std::array<T*, Count> ptrs {};
		size_t alive = 0;

		std::mt19937 rng(std::random_device {}());
		std::uniform_real_distribution<> dist(0.0, 1.0);

		auto start = std::chrono::high_resolution_clock::now();

		for (size_t i = 0; i < Count; ++i) {
			ptrs[alive++] = metapool.construct<T>();
		}

		for (uint32_t f = 0; f < frames; ++f) {
			size_t next = 0;

			for (size_t i = 0; i < alive; ++i) {
				if (dist(rng) > 0.2) {
					metapool.destruct(ptrs[i]);
				} else {
					ptrs[next++] = ptrs[i];
				}
			}
			while (next < Count) {
				ptrs[next++] = metapool.construct<T>();
			}

			alive = next;
		}
	
		for (size_t i = 0; i < alive; ++i) {
			metapool.destruct(ptrs[i]);
		}
	
		auto end = std::chrono::high_resolution_clock::now();
		double time = std::chrono::duration<double, std::milli>(end - start).count();
		std::cout << "metapool churn time: " << time << " ms" << std::endl;
		return time;
	}

};


inline std::unique_ptr<Benchmark> create_benchmark_selective()
{
	return std::make_unique<BenchmarkSelective>();
}

