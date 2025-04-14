#pragma once

#include <cstddef>
#include <cstdint>

#include "metapool_proxy.hpp"


namespace hpr {
namespace mem {


	struct RangeMetadata
	{
		uint32_t stride_step {0};
		uint32_t stride_min  {0};
		uint32_t stride_max  {0};

		constexpr uint32_t count() const {
			return (stride_max - stride_min) / stride_step;
		}
	};


	struct allocator_config_tag {};

	template <auto RangeCount>
	struct AllocatorConfig
	{
		using tag = allocator_config_tag;

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
	};

} // hpr::mem
} // hpr
