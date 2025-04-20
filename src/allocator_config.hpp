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

	template <auto RangeCount>
	struct AllocatorConfig
	{
		using tag = allocator_config_tag;
		using proxy_array_type = std::array<MetapoolProxy, RangeCount>;

		static constexpr uint32_t alignment_quantum {8U};

		std::array<RangeMetadata, RangeCount> range_metadata;

		constexpr AllocatorConfig(const std::array<RangeMetadata, RangeCount>& mpool_traits_arr)
			: range_metadata(mpool_traits_arr) {}
	};


	template <typename T>
	concept IsAllocatorConfig = requires {
		typename T::tag;
		typename T::proxy_array_type;

		std::same_as<typename T::tag, allocator_config_tag>;

		T::range_metadata;

	} && []() constexpr {
		const auto& ranges = T::range_metadata;

		if (ranges.size() == 0) {
			return false;
		}

		for (std::size_t i = 0; i < ranges.size(); ++i) {
			const auto& range = ranges[i];

			if (range.stride_step == 0) {
				return false;
			}

			if ((range.stride_step & (range.stride_step - 1)) == 0) {
				const uint32_t mask = range.stride_step - 1;
				if ((range.stride_min & mask) != 0 || (range.stride_max & mask) != 0) {
					return false;
				}
			}
			else {
				if (range.stride_min % range.stride_step != 0 || 
					range.stride_max % range.stride_step != 0) {
					return false;
				}
			}

			if (i < ranges.size() - 1) {
				const auto& next = ranges[i + 1];
				if (range.stride_max + range.stride_step != next.stride_min) {
					return false;
				}
			}
		}

		return true;
	}();

} // hpr::mem
} // hpr
