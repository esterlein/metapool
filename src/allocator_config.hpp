#pragma once

#include <cstddef>
#include <cstdint>

#include "metapool_descriptor.hpp"


namespace hpr {
namespace mem {


	struct allocator_config_tag {};

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

		static constexpr auto metapool_count = std::tuple_size_v<DescriptorArray>;
		static constexpr auto alloc_header_size = sizeof(AllocHeader);

		static constexpr uint32_t alignment_quantum {0};
		uint32_t min_stride_step {0};
		uint32_t min_stride {0};
		uint32_t max_stride {0};
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
