#pragma once

#include <cstdint>


namespace hpr::mem {


	struct AllocHeader
	{
		uint16_t encoded_index;

		static constexpr AllocHeader make(uint8_t mpool, uint8_t flist)
		{
			return AllocHeader {
				static_cast<uint16_t>((mpool << 8) | flist)
			};
		}

		uint8_t mpool_index() const
		{
			return static_cast<uint8_t>(encoded_index >> 8);
		}

		uint8_t flist_index() const
		{
			return static_cast<uint8_t>(encoded_index & 0xFF);
		}
	};

	static constexpr auto alloc_header_size = sizeof(AllocHeader);

} // hpr::mem
