#pragma once

#include <cstdint>


namespace hpr {
namespace mem {

		struct AllocHeader
		{
			uint8_t pool_index;
			uint8_t magic = 0xAB;
		};

		static constexpr auto alloc_header_size = sizeof(AllocHeader);

} // hpr::mem
} // hpr
