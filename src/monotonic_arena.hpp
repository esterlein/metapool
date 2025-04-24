#pragma once

#include <cstdlib>
#include <cstddef>
#include <cstdint>


namespace hpr {


class MonotonicArena final
{
public:

	MonotonicArena(std::size_t size, std::size_t alignment);
	~MonotonicArena();

	MonotonicArena(const MonotonicArena&) = delete;
	MonotonicArena& operator=(const MonotonicArena&) = delete;
	MonotonicArena(MonotonicArena&& other) = delete;
	MonotonicArena& operator=(MonotonicArena&& other) = delete;

public:

	std::byte* allocate(std::size_t size, std::size_t alignment, std::size_t shift);
	std::byte* fetch(std::size_t size, std::size_t alignment, std::size_t shift);
	void deallocate(std::byte* aligned_ptr);

	inline bool is_equal(const MonotonicArena& other) const noexcept
	{
		return this == &other;
	}

	inline void reset() noexcept
	{ m_offset = 0; }

	[[nodiscard]] inline void* align_pointer(void* ptr, std::size_t alignment)
	{
		std::uintptr_t uintptr = reinterpret_cast<std::uintptr_t>(ptr);
		std::uintptr_t aligned = (uintptr + alignment - 1) & ~(alignment - 1);
		return reinterpret_cast<void*>(aligned);
	}

private:

	std::byte*  m_arena  {nullptr};
	std::size_t m_size   {0};
	std::size_t m_offset {0};
};
} // hpr
