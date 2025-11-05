#pragma once

#include "mtpint.hpp"

#include <array>
#include <type_traits>

#include "fail.hpp"
#include "freelist_proxy.hpp"


namespace mtp::cfg {


struct allocator_config_tag {};


template <auto MetapoolRangeArray>
struct AllocatorConfig
{
	using tag = allocator_config_tag;

	static constexpr auto range_metadata = MetapoolRangeArray;
	static constexpr uint32_t range_count = static_cast<uint32_t>(range_metadata.size());

	static constexpr size_t total_stride_count = [] {
		uint32_t total = 0;
		for (uint32_t i = 0; i < range_count; ++i)
			total += range_metadata[i].stride_count;
		return static_cast<size_t>(total);
	}();

	static constexpr uint32_t max_stride_count = 4096U;

	static_assert(total_stride_count <= max_stride_count,
		CONFIG_STRIDE_LIMIT_MSG);

	using ProxyArrayType = std::array<mtp::core::FreelistProxy, total_stride_count>;

	static constexpr uint32_t alignment_quantum {8U};

	static constexpr uint32_t min_stride = range_metadata[0].stride_min;
	static constexpr uint32_t max_stride = range_metadata[range_count - 1].stride_max;

	static_assert(range_count > 0,
		CONFIG_EMPTY_RANGE_MSG);

	static_assert(range_metadata[0].stride_step > 0,
		CONFIG_FIRST_ZERO_STEP_MSG);
};


template <typename Config>
concept IsAllocatorConfig = requires {

	typename std::remove_cvref_t<Config>::tag;
	typename std::remove_cvref_t<Config>::ProxyArrayType;

	{ std::remove_cvref_t<Config>::range_metadata };
	{ std::remove_cvref_t<Config>::range_count }        -> std::convertible_to<uint32_t>;
	{ std::remove_cvref_t<Config>::total_stride_count } -> std::convertible_to<size_t>;
	{ std::remove_cvref_t<Config>::alignment_quantum }  -> std::convertible_to<uint32_t>;
	{ std::remove_cvref_t<Config>::min_stride }         -> std::convertible_to<uint32_t>;
	{ std::remove_cvref_t<Config>::max_stride }         -> std::convertible_to<uint32_t>;
} && std::is_same_v<typename std::remove_cvref_t<Config>::tag, allocator_config_tag> && []() constexpr {

	const auto& ranges = std::remove_cvref_t<Config>::range_metadata;
	const uint32_t count = static_cast<uint32_t>(ranges.size());

	if (count == 0)
		return false;

	uint32_t total_stride_count = 0;

	for (uint32_t i = 0; i < count; ++i) {
		const auto& range = ranges[i];

		if (range.stride_step == 0 || range.stride_min > range.stride_max)
			return false;

		total_stride_count += range.stride_count;

		const bool is_single = range.stride_min == range.stride_max;

		if (!is_single) {
			if ((range.stride_step & (range.stride_step - 1)) == 0) {
				uint32_t mask = range.stride_step - 1;
				if ((range.stride_min & mask) != 0 || (range.stride_max & mask) != 0)
					return false;
			} else {
				if ((range.stride_min % range.stride_step) != 0 ||
					(range.stride_max % range.stride_step) != 0)
					return false;
			}
		}
	}

	if (total_stride_count > Config::max_stride_count)
		return false;

	return true;
}();

} // mtp::cfg

