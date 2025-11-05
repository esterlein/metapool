#pragma once

#include "mtpint.hpp"

#include <array>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <string_view>


#ifndef MTP_ENABLE_TRACE
	#if defined(NDEBUG)
		#define MTP_ENABLE_TRACE 0
	#else
		#define MTP_ENABLE_TRACE 0
	#endif
#endif


namespace mtp::cfg {


#if MTP_ENABLE_TRACE


class AllocTracer
{
public:

	AllocTracer() = delete;

	static void trace(uint32_t raw_size, uint32_t alignment, uint32_t stride, uint16_t proxy_index)
	{
		const int index = find_or_insert(raw_size, alignment, proxy_index);
		if (index >= 0) {
			auto& stat = stats[index];
			++stat.count;
			stat.raw_total_bytes += raw_size;
			stat.stride_total_bytes += stride;
		} else {
			++overflow.count;
			overflow.raw_total_bytes += raw_size;
			overflow.stride_total_bytes += stride;
		}
	}

	static void trace_fallback(uint32_t raw_size, uint32_t alignment, uint16_t proxy_index)
	{
		const int index = find_or_insert(raw_size, alignment, proxy_index);
		if (index >= 0) {
			++stats[index].fallback_count;
		} else {
			++overflow.fallback_count;
		}
	}

	static void export_trace(std::string_view filename, bool clear = false)
	{
		std::filesystem::create_directories(std::filesystem::path(filename).parent_path());

		std::ofstream out{std::string(filename)};
		if (!out.is_open())
			return;

		out << "raw_size,alignment,proxy_index,count,fallbacks,raw_total_bytes,stride_total_bytes\n";

		for (uint32_t i = 0; i < used; ++i) {
			out << keys[i][0] << ','
				<< keys[i][1] << ','
				<< keys[i][2] << ','
				<< stats[i].count << ','
				<< stats[i].fallback_count << ','
				<< stats[i].raw_total_bytes << ','
				<< stats[i].stride_total_bytes << '\n';
		}

		if (overflow.count > 0 || overflow.fallback_count > 0) {
			out << "?,?,?,"
				<< overflow.count << ','
				<< overflow.fallback_count << ','
				<< overflow.raw_total_bytes << ','
				<< overflow.stride_total_bytes << '\n';
		}

		if (clear) {
			used = 0;
			overflow = {};
		}

		std::cout << "trace written: " << filename << '\n';
	}

private:

	struct Stat
	{
		uint32_t count;
		uint32_t fallback_count;
		uint64_t raw_total_bytes;
		uint64_t stride_total_bytes;

		Stat() noexcept
			: count{0}
			, fallback_count{0}
			, raw_total_bytes{0}
			, stride_total_bytes{0}
		{}
	};

	static constexpr uint32_t max_trace_entries = 32768;

	static int find_or_insert(uint32_t raw_size, uint32_t alignment, uint16_t proxy_index)
	{
		for (uint32_t i = 0; i < used; ++i) {
			if (keys[i][0] == raw_size &&
				keys[i][1] == alignment &&
				keys[i][2] == proxy_index)
				return static_cast<int>(i);
		}

		if (used < max_trace_entries) {
			keys[used] = { raw_size, alignment, proxy_index };
			stats[used] = Stat{};
			return static_cast<int>(used++);
		}

		return -1;
	}

	static inline std::array<std::array<uint32_t, 3>, max_trace_entries> keys {};
	static inline std::array<Stat, max_trace_entries> stats {};
	static inline uint32_t used = 0;
	static inline Stat overflow {};
};

#else

class AllocTracer
{
public:
	static inline void trace(uint32_t, uint32_t, uint32_t, uint16_t) noexcept {}
	static inline void trace_fallback(uint32_t, uint32_t, uint16_t) noexcept {}
	static inline void export_trace(std::string_view = {}, bool = false) noexcept {}
};

#endif

} // mtp::cfg

