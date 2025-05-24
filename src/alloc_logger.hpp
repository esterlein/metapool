#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <string_view>

#include "metapool_config.hpp"


namespace hpr::mem {

#if ENABLE_ALLOC_LOGGING

class AllocLogger
{
public:

	static void log(uint32_t raw_size)
	{
		uint32_t index = raw_size / MetapoolConstraints::min_stride_step;

		if (index < k_bucket_count) {
			auto& stat = s_stats[index];
			++stat.count;
			stat.raw_bytes += raw_size;
		}
	}

	static void export_profile(std::string_view filename, bool clear = false)
	{
		std::ofstream out(filename.data());
		out << "bucket,count,raw_bytes\n";

		for (uint32_t i = 0; i < k_bucket_count; ++i) {
			const auto& stat = s_stats[i];
			if (stat.count > 0) {
				uint32_t bucket = i * MetapoolConstraints::min_stride_step;
				out << bucket << ',' << stat.count << ',' << stat.raw_bytes << '\n';
			}
			if (clear) {
				s_stats[i] = {};
			}
		}
	}

private:

	struct AllocStats
	{
		uint32_t count;
		uint32_t raw_bytes;
	};

	static constexpr uint32_t k_bucket_count =
		MetapoolConstraints::max_stride / MetapoolConstraints::min_stride_step;

	inline static std::array<AllocStats, k_bucket_count> s_stats {};
};

#else

class AllocLogger
{
public:

	static inline void log(uint32_t) noexcept {}
	static inline void export_profile(std::string_view = {}, bool = false) noexcept {}
};

#endif

} // hpr::mem
