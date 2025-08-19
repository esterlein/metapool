#pragma once

#include <memory>
#include <cstdlib>
#include <cstddef>

#include "fail.hpp"


namespace mtp::core {


class MonotonicArena final
{
public:

	MonotonicArena(std::size_t size, std::size_t alignment)
		: m_size {size}
	{
		std::size_t padded_size = ((size + alignment - 1) / alignment) * alignment;

		m_arena = static_cast<std::byte*>(std::aligned_alloc(alignment, padded_size));

		if (!m_arena) {
			mtp::err::fatal(mtp::err::arena_alloc_failed);
		}
	}

	~MonotonicArena()
	{
		std::free(m_arena);
	}

	MonotonicArena(const MonotonicArena&) = delete;
	MonotonicArena& operator=(const MonotonicArena&) = delete;
	MonotonicArena(MonotonicArena&& other) = delete;
	MonotonicArena& operator=(MonotonicArena&& other) = delete;

public:

	inline std::byte* fetch(std::size_t alloc_size, std::size_t alignment, std::size_t shift)
	{
		if (alloc_size == 0) [[unlikely]] {
			return nullptr;
		}
	
		std::byte* current = m_arena + m_offset;
		std::size_t available = m_size - m_offset;
	
		void* aligned_ptr = current + shift;
		if (std::align(alignment, alloc_size, aligned_ptr, available) == nullptr) [[unlikely]] {
			MTP_ASSERT_CTX(false,
				mtp::err::arena_fetch_no_fit,
				mtp::err::format_ctx("size = %zu, align = %zu, shift = %zu", alloc_size, alignment, shift));
		}

		std::byte* base_ptr = static_cast<std::byte*>(aligned_ptr);
	
		std::size_t adjustment = static_cast<std::size_t>(base_ptr - current);
		std::size_t allocation_total = adjustment + alloc_size;

		MTP_ASSERT_CTX(m_offset + allocation_total <= m_size,
			mtp::err::arena_fetch_overflow,
			mtp::err::format_ctx("size = %zu, align = %zu, shift = %zu", alloc_size, alignment, shift));

		m_offset += allocation_total;
	
		return base_ptr;
	}


	inline bool is_equal(const MonotonicArena& other) const noexcept
	{
		return this == &other;
	}


	inline void reset() noexcept
	{
		m_offset = 0;
	}

private:

	std::byte*  m_arena  {nullptr};
	std::size_t m_size   {0};
	std::size_t m_offset {0};
};

} // mtp::core
