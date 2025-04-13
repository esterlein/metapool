#pragma once

#include <cstddef>
#include <cstdint>

#include "metapool_descriptor.hpp"


namespace hpr {
namespace mem {


	struct allocator_config_tag {};

	// this structure should be parameterized not by a descriptor array,
	// but by a metapool registry type which is a tuple of metapool configs

	// this way, the allocator config will help produce a constexpr stride index computation
	// inside an allocator template instance

	template <typename DescriptorArray>
	struct AllocatorConfig
	{
		using tag = allocator_config_tag;
		using descriptor_array_type = DescriptorArray;

		struct AllocHeader
		{
			uint16_t pool_index;
			uint16_t magic = 0xABCD;
		};

		static constexpr auto alloc_header_size = sizeof(AllocHeader);
		static constexpr auto metapool_count = std::tuple_size_v<DescriptorArray>;

		static constexpr uint32_t alignment_quantum {0};

		uint32_t stride_step {0}; // should be multiple stride steps
		uint32_t stride_min  {0}; // and bounds for each metapool
		uint32_t stride_max  {0};
	};


	template <typename T>
	concept IsAllocatorConfig = requires {

		typename T::tag;
		typename T::descriptor_array_type;

		std::same_as<typename T::tag, allocator_config_tag>;

		requires MetapoolDescriptorArray <
			typename T::descriptor_array_type,
			std::tuple_size_v<typename T::descriptor_array_type>
		>;
	};

} // hpr::mem
} // hpr
