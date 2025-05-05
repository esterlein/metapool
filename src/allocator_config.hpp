#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

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
		uint32_t stride_shift {0};
	};


	struct allocator_config_tag {};

	template <auto MetapoolRangeArray>
	struct AllocatorConfig
	{
		using tag = allocator_config_tag;

		static constexpr auto range_metadata = MetapoolRangeArray;
		static constexpr auto range_count    = MetapoolRangeArray.size();

		using ProxyArrayType = std::array<MetapoolProxy, range_count>;

		static constexpr uint32_t alignment_quantum {8U};

		static constexpr uint32_t min_stride = range_metadata[0].stride_min;

		static constexpr uint32_t max_stride = range_metadata[range_count - 1].stride_max;

		static_assert(range_count > 0,
			"allocator config: empty range list");

		static_assert(range_metadata[0].stride_step > 0,
			"allocator config: first range has zero stride step");
	};


	template <typename T>
	concept IsAllocatorConfig = requires {

		typename std::remove_cvref_t<T>::tag;
		typename std::remove_cvref_t<T>::ProxyArrayType;

		{ std::remove_cvref_t<T>::range_metadata };
		{ std::remove_cvref_t<T>::range_count }       -> std::convertible_to<std::size_t>;
		{ std::remove_cvref_t<T>::alignment_quantum } -> std::convertible_to<uint32_t>;
		{ std::remove_cvref_t<T>::min_stride }        -> std::convertible_to<uint32_t>;
		{ std::remove_cvref_t<T>::max_stride }        -> std::convertible_to<uint32_t>;
	} && std::is_same_v<typename std::remove_cvref_t<T>::tag, allocator_config_tag> && []() constexpr {

		const auto& ranges = std::remove_cvref_t<T>::range_metadata;
		const auto range_count = ranges.size();

		if (range_count == 0)
			return false;

		for (std::size_t i = 0; i < range_count; ++i) {
			const auto& range = ranges[i];

			if (range.stride_step == 0 || range.stride_min > range.stride_max)
				return false;

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
		return true;
	}();

} // hpr::mem
} // hpr
