#pragma once

#include <cstddef>
#include <cstdint>

#include "metapool_proxy.hpp"


namespace hpr {
namespace mem {


	struct allocator_config_tag {};

	template <auto RangeCount>
	struct AllocatorConfig
	{
		using tag = allocator_config_tag;

		struct AllocHeader
		{
			uint8_t pool_index;
			uint8_t magic = 0xAB;
		};

		static constexpr auto alloc_header_size = sizeof(AllocHeader);

		static constexpr uint32_t alignment_quantum {8U};

		struct RangeMetadata
		{
			uint32_t stride_step {0};
			uint32_t stride_min  {0};
			uint32_t stride_max  {0};

			constexpr uint32_t count() const {
				return (stride_max - stride_min) / stride_step;
			}
		};

		std::array<RangeMetadata, RangeCount> range_metadata;
	};


	template <typename T>
	concept IsAllocatorConfig = requires {

		typename T::tag;
		typename T::proxy_array_type;

		std::same_as<typename T::tag, allocator_config_tag>;

/*		requires MetapoolProxyArray <
			typename T::proxy_array_type,
			std::tuple_size_v<typename T::proxy_array_type>
		>;*/
	};

} // hpr::mem
} // hpr
