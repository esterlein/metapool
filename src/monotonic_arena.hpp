#pragma once

#include "metapool.tpp"
#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <memory_resource>


namespace hpr {


class MonotonicArena : public std::pmr::memory_resource
{
public:

	explicit MonotonicArena(std::size_t size, std::size_t alignment = hpr::mem::cacheline, std::size_t shift = 0);
	virtual ~MonotonicArena() override;

	MonotonicArena(const MonotonicArena&) = delete;
	MonotonicArena& operator=(const MonotonicArena&) = delete;

	MonotonicArena(MonotonicArena&& other) noexcept;
	MonotonicArena& operator=(MonotonicArena&& other) noexcept;

	std::byte* fetch(std::size_t size);

	inline void reset() noexcept { m_offset = 0; }

protected:

	virtual void* do_allocate(std::size_t size, std::size_t alignment) override;
	inline virtual void do_deallocate(void* block, std::size_t size, std::size_t alignment) override {}
	inline virtual bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
	}

	[[nodiscard]] inline void* align_pointer(void* ptr, std::size_t alignment)
	{
		std::uintptr_t p = reinterpret_cast<std::uintptr_t>(ptr);
		std::uintptr_t aligned = (p + alignment - 1) & ~(alignment - 1);
		return reinterpret_cast<void*>(aligned);
	}

private:

	std::byte* allocate_aligned(std::size_t size, std::size_t alignment = 64, std::size_t shift = 0);
	void free_aligned(std::byte* aligned_ptr);

	std::byte*  m_arena;
	std::size_t m_size;
	std::size_t m_offset;
	std::size_t m_shift;
};
} // hpr
