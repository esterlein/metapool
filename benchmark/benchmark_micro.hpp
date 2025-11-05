#pragma once

#include <array>
#include <chrono>
#include <vector>
#include <iostream>
#include <string_view>
#include <memory_resource>

#include "mtp_memory.hpp"

#include "benchmark.hpp"



struct MicroTiny
{
	std::array<int8_t, 3> data {};
	MicroTiny() { touch_prefix(this, 3); }
};

struct MicroSmall
{
	std::array<int, 8> data {};
	MicroSmall() { touch_prefix(this, 32); }
};

struct MicroMed1
{
	std::array<int, 96> data {};
	MicroMed1() { touch_prefix(this, 384); }
};

struct MicroMed2
{
	std::array<int, 160> data {};
	MicroMed2() { touch_prefix(this, 640); }
};

struct MicroLarge1
{
	std::array<int, 1200> data {};
	MicroLarge1() { touch_prefix(this, 4800); }
};

struct MicroLarge2
{
	std::array<int, 2100> data {};
	MicroLarge2() { touch_prefix(this, 8400); }
};

struct MicroXLarge
{
	std::array<int, 3900> data {};
	MicroXLarge() { touch_prefix(this, 15600); }
};


template <typename T>
struct DummyOps
{
	static constexpr std::string_view label = "unknown";
};

template <> struct DummyOps<MicroTiny>    { static constexpr std::string_view label = "tiny"; };
template <> struct DummyOps<MicroSmall>   { static constexpr std::string_view label = "small"; };
template <> struct DummyOps<MicroMed1>    { static constexpr std::string_view label = "med1"; };
template <> struct DummyOps<MicroMed2>    { static constexpr std::string_view label = "med2"; };
template <> struct DummyOps<MicroLarge1>  { static constexpr std::string_view label = "large1"; };
template <> struct DummyOps<MicroLarge2>  { static constexpr std::string_view label = "large2"; };
template <> struct DummyOps<MicroXLarge>  { static constexpr std::string_view label = "xlarge"; };


class BenchmarkMicro : public Benchmark
{
private:

	using Set = mtp::metaset <

		mtp::def<mtp::capf::flat, 2, 8, 640'000'000, 640'000'000>
	>;

public:

	inline void setup() override
	{
		std::cout << "\n\n\n--- METAPOOL MICRO BENCHMARK ---\n" << std::endl;

		m_trash.resize(trash_size, 0x55);
	}

	inline void teardown() override
	{
		auto& allocator = mtp::get_allocator<Set>();
		allocator.reset();
	}

	inline void run() override
	{
		m_emplace_entries.clear();
		m_reserve_entries.clear();

		run_all<MicroTiny>();
		run_all<MicroSmall>();
		run_all<MicroMed1>();
		run_all<MicroMed2>();
		run_all<MicroLarge1>();
		run_all<MicroLarge2>();
		run_all<MicroXLarge>();

		std::cout << std::endl;

		print_summary(m_reserve_entries, m_emplace_entries);
	}

private:

	struct Result
	{
		double reserve = 0;
		double emplace = 0;
	};

	struct BenchmarkEntry
	{
		std::string_view name;
		size_t size;
		std::array<Result, 3> results {};
	};

	std::vector<BenchmarkEntry> m_reserve_entries;
	std::vector<BenchmarkEntry> m_emplace_entries;


	static constexpr size_t trash_size = 4 * 1024 * 1024;
	std::vector<uint8_t> m_trash;

	inline void flush_cache()
	{
		uint64_t sum = 0;
		for (size_t i = 0; i < trash_size; i += 64) {
			sum += m_trash[i];
		}
		if (sum == 0xdeadbeef) std::abort();
	}


	template <typename T>
	static constexpr size_t get_count()
	{
		if constexpr (std::is_same_v<T, MicroTiny>)        return 15'000'000;
		else if constexpr (std::is_same_v<T, MicroSmall>)  return 7'000'000;
		else if constexpr (std::is_same_v<T, MicroMed1>)   return 800'000;
		else if constexpr (std::is_same_v<T, MicroMed2>)   return 500'000;
		else if constexpr (std::is_same_v<T, MicroLarge1>) return 80'000;
		else if constexpr (std::is_same_v<T, MicroLarge2>) return 36'000;
		else if constexpr (std::is_same_v<T, MicroXLarge>) return 16'000;
		else return 0;
	}

	template <typename T>
	static constexpr size_t get_frames()
	{
		if constexpr (std::is_same_v<T, MicroTiny>)        return 1000;
		else if constexpr (std::is_same_v<T, MicroSmall>)  return 1000;
		else if constexpr (std::is_same_v<T, MicroMed1>)   return 800;
		else if constexpr (std::is_same_v<T, MicroMed2>)   return 600;
		else if constexpr (std::is_same_v<T, MicroLarge1>) return 400;
		else if constexpr (std::is_same_v<T, MicroLarge2>) return 300;
		else if constexpr (std::is_same_v<T, MicroXLarge>) return 200;
		else return 0;
	}

	template <typename T>
	void run_all()
	{
		const size_t count  = get_count<T>();
		const size_t frames = get_frames<T>();

		std::cout << "\nrunning emplace tests (" << DummyOps<T>::label << ")...\n" << std::endl;

		Result emplace_std = run_emplace_std<T>(count, frames);
		Result emplace_pmr = run_emplace_pmr<T>(count, frames);
		Result emplace_mtp = run_emplace_mtp<T>(count, frames);

		m_emplace_entries.push_back({
			DummyOps<T>::label,
			sizeof(T),
			{ emplace_std, emplace_pmr, emplace_mtp }
		});

		std::cout << "\nrunning reserve tests (" << DummyOps<T>::label << ")...\n" << std::endl;

		Result reserve_std = run_reserve_std<T>(count, frames);
		Result reserve_pmr = run_reserve_pmr<T, 1300000000>(count, frames);
		Result reserve_mtp = run_reserve_mtp<T>(count, frames);

		m_reserve_entries.push_back({
			DummyOps<T>::label,
			sizeof(T),
			{ reserve_std, reserve_pmr, reserve_mtp }
		});
	}

	template <typename T>
	Result run_emplace_std(size_t count, size_t)
	{
		std::cout << "run std emplace (size: " << sizeof(T) << ")..." << std::endl;

		Result result;
		std::vector<T> vec;
		vec.reserve(count);

		flush_cache();

		auto t1 = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < count; ++i)
			vec.emplace_back();
		auto t2 = std::chrono::high_resolution_clock::now();

		result.emplace = std::chrono::duration<double, std::milli>(t2 - t1).count();
		std::cout << "emplace std: " << result.emplace << " ms\n";
		return result;
	}

	template <typename T>
	Result run_emplace_pmr(size_t count, size_t)
	{
		std::cout << "run pmr emplace (size: " << sizeof(T) << ")..." << std::endl;

		Result result;
		std::pmr::unsynchronized_pool_resource upstream;
		std::pmr::vector<T> vec(&upstream);
		vec.reserve(count);

		flush_cache();

		auto t1 = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < count; ++i)
			vec.emplace_back();
		auto t2 = std::chrono::high_resolution_clock::now();

		result.emplace = std::chrono::duration<double, std::milli>(t2 - t1).count();
		std::cout << "emplace pmr: " << result.emplace << " ms\n";
		return result;
	}

	template <typename T>
	Result run_emplace_mtp(size_t count, size_t)
	{
		std::cout << "run mtp emplace (size: " << sizeof(T) << ")..." << std::endl;

		Result result;
		auto vlt = mtp::make_vault<T, Set>(count);

		flush_cache();

		auto t1 = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < count; ++i)
			vlt.emplace_back();
		auto t2 = std::chrono::high_resolution_clock::now();

		result.emplace = std::chrono::duration<double, std::milli>(t2 - t1).count();
		std::cout << "emplace mtp: " << result.emplace << " ms\n";
		return result;
	}

	template <typename T>
	Result run_reserve_std(size_t count, size_t frames)
	{
		std::cout << "run std reserve (size: " << sizeof(T) << ")..." << std::endl;

		Result result;

		volatile size_t sink = 0;

		auto t1 = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < frames; ++i) {
			std::vector<T> vec;
			vec.reserve(count);
			sink ^= reinterpret_cast<std::uintptr_t>(vec.data());
		}
		auto t2 = std::chrono::high_resolution_clock::now();

		result.reserve = std::chrono::duration<double, std::milli>(t2 - t1).count();

		std::cout << "reserve std: " << result.reserve << " ms\n";
		return result;
	}

	template <typename T, size_t ArenaBytes>
	Result run_reserve_pmr(size_t count, size_t frames)
	{
		std::cout << "run pmr reserve (size: " << sizeof(T) << ")...\n";

		struct fail_resource final : std::pmr::memory_resource
		{
			void* do_allocate(size_t, size_t) override
			{
				return nullptr;
			}

			void  do_deallocate(void*, size_t, size_t) override
			{}

			bool  do_is_equal(const std::pmr::memory_resource& other) const noexcept override
			{
				return this == &other;
			}
		};

		static thread_local std::byte* thread_local_buffer = []{
			return static_cast<std::byte*>(
				::operator new(ArenaBytes, std::align_val_t(alignof(std::max_align_t))));
		}();

		static thread_local fail_resource fail_upstream;

		std::pmr::monotonic_buffer_resource mono {thread_local_buffer, ArenaBytes, &fail_upstream};
		std::pmr::pool_options opts {};

		opts.max_blocks_per_chunk = 1024;
		opts.largest_required_pool_block = std::max<size_t>(64, sizeof(T));

		std::pmr::unsynchronized_pool_resource pool {opts, &mono};
		std::pmr::polymorphic_allocator<T> allocator {&pool};

		Result result {};
		volatile std::uintptr_t sink = 0;

		auto t1 = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < frames; ++i) {

			{
				std::pmr::vector<T> vec {allocator};
				vec.reserve(count);
				sink ^= reinterpret_cast<std::uintptr_t>(vec.data());
			}

			pool.release();
			mono.release();
		}
		auto t2 = std::chrono::high_resolution_clock::now();

		result.reserve = std::chrono::duration<double, std::milli>(t2 - t1).count();
		std::cout << "reserve pmr: " << result.reserve << " ms\n";

		return result;
	}

	template <typename T>
	Result run_reserve_mtp(size_t count, size_t frames)
	{
		std::cout << "run mtp reserve (size: " << sizeof(T) << ")..." << std::endl;

		Result result;

		volatile size_t sink = 0;

		auto t1 = std::chrono::high_resolution_clock::now();
		for (size_t i = 0; i < frames; ++i) {
			mtp::vault<T, Set> vlt;
			vlt.reserve(count);
			sink ^= reinterpret_cast<std::uintptr_t>(vlt.data());
		}
		auto t2 = std::chrono::high_resolution_clock::now();

		result.reserve = std::chrono::duration<double, std::milli>(t2 - t1).count();

		std::cout << "reserve mtp: " << result.reserve << " ms\n";
		return result;
	}


	void print_summary(std::span<const BenchmarkEntry> reserve, std::span<const BenchmarkEntry> emplace)
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

		auto build_rows = [&](std::span<const BenchmarkEntry> entries, auto getter) -> std::vector<Row> {
			std::vector<Row> rows;
			for (const auto& e : entries) {
				std::ostringstream s0, s1, s2, label_os;
				label_os << e.name << " (" << e.size << ")";

				double t_std = getter(e.results[0]);
				double t_pmr = getter(e.results[1]);
				double t_mtp = getter(e.results[2]);

				s0 << std::fixed << t_std;
				s1 << std::fixed << t_pmr;
				s2 << std::fixed << t_mtp;

				rows.push_back({
					label_os.str(),
					s0.str(),
					s1.str(),
					s2.str(),
					ratio_str(t_std, t_mtp),
					ratio_str(t_pmr, t_mtp)
				});
			}
			return rows;
		};

		auto reserve_rows = build_rows(reserve, [](const Result& r) { return r.reserve; });
		auto emplace_rows = build_rows(emplace, [](const Result& r) { return r.emplace; });

		size_t col0 = std::strlen("label");
		size_t col1 = std::strlen("std (ms)");
		size_t col2 = std::strlen("pmr (ms)");
		size_t col3 = std::strlen("mtp (ms)");
		size_t col4 = std::strlen("mtp vs std");
		size_t col5 = std::strlen("mtp vs pmr");

		auto update_widths = [&](const std::vector<Row>& rows) {
			for (const auto& r : rows) {
				col0 = std::max(col0, r.label.length());
				col1 = std::max(col1, r.std_time.length());
				col2 = std::max(col2, r.pmr_time.length());
				col3 = std::max(col3, r.mtp_time.length());
				col4 = std::max(col4, r.vs_std.length());
				col5 = std::max(col5, r.vs_pmr.length());
			}
		};

		update_widths(reserve_rows);
		update_widths(emplace_rows);

		constexpr int gap = 4;
		auto pad = [](size_t w) { return static_cast<int>(w + gap); };

		auto print_rows = [&](std::string_view label, const std::vector<Row>& rows) {
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
		};

		print_rows("--- reserve", reserve_rows);
		print_rows("--- emplace", emplace_rows);
		std::cout << "\n\n";
	}
};

inline std::unique_ptr<Benchmark> create_benchmark_micro()
{
	return std::make_unique<BenchmarkMicro>();
}

