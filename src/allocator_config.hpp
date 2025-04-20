#pragma once

#include <array>
#include <cstdint>

#include "metapool_proxy.hpp"



namespace hpr {


class MetapoolProxy;


namespace mem {


	struct RangeMetadata
	{
		uint32_t stride_min   {0};
		uint32_t stride_max   {0};
		uint32_t stride_step  {0};
		uint32_t stride_count {0};
	};


	struct allocator_config_tag {};

	template <auto MetapoolRangeArray>
	struct AllocatorConfig
	{
		using tag = allocator_config_tag;

		static constexpr auto range_metadata = MetapoolRangeArray;
		static constexpr auto range_count    = MetapoolRangeArray.size();

		using ProxyArrayType = std::array<MetapoolProxy, range_count>;
	};


template <typename T>
concept IsAllocatorConfig = requires {

	typename T::tag;
	typename T::ProxyArrayType;

	{ T::range_metadata } -> std::same_as<decltype(T::range_metadata)>;
	{ T::range_count }    -> std::convertible_to<std::size_t>;

} && []() constexpr {

	const auto& ranges      = T::range_metadata;
	const std::size_t range_count = ranges.size();

	if (range_count == 0) {
		return false;
	}

	for (std::size_t i = 0; i < range_count; ++i) {
		const auto& this_range = ranges[i];

		if (this_range.stride_step == 0) {
			return false;
		}

		if ((this_range.stride_step & (this_range.stride_step - 1)) == 0) {
			uint32_t mask = this_range.stride_step - 1;
			if ((this_range.stride_min & mask) ||
				(this_range.stride_max & mask))
			{
				return false;
			}
		}
		else {
			if ((this_range.stride_min % this_range.stride_step) != 0 ||
				(this_range.stride_max % this_range.stride_step) != 0)
			{
				return false;
			}
		}

		if (i + 1 < range_count) {
			const auto& next_range = ranges[i + 1];
			if (this_range.stride_max + this_range.stride_step
				!= next_range.stride_min)
			{
				return false;
			}
		}
	}

	return true;
}();

} // hpr::mem
} // hpr
