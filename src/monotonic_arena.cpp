#include <memory>

#include "monotonic_arena.hpp"


namespace hpr {


MonotonicArena::MonotonicArena(std::size_t size, std::size_t alignment)
	: m_aligned_arena{allocate_aligned(size, alignment)}
	, m_size         {size}
	, m_offset       {0}
{
	if (!m_aligned_arena) {
		throw std::bad_alloc();
	}
}


MonotonicArena::~MonotonicArena()
{
	free_aligned(m_aligned_arena);
}


MonotonicArena::MonotonicArena(MonotonicArena&& other) noexcept
	: m_aligned_arena{other.m_aligned_arena}
	, m_size         {other.m_size}
	, m_offset       {other.m_offset}
{
	other.m_aligned_arena = nullptr;
	other.m_size   = 0;
	other.m_offset = 0;
}


MonotonicArena& MonotonicArena::operator=(MonotonicArena&& other) noexcept
{
	if (this != &other) {
		if (m_aligned_arena) {
			free_aligned(m_aligned_arena);
		}
		m_aligned_arena = other.m_aligned_arena;
		m_size          = other.m_size;
		m_offset        = other.m_offset;
		other.m_aligned_arena = nullptr;
		other.m_size          = 0;
		other.m_offset        = 0;
	}
	return *this;
}


std::byte* MonotonicArena::fetch(std::size_t alloc_size)
{
	std::byte* current = m_aligned_arena + m_offset;

	if (m_offset + alloc_size > m_size) {
		throw std::bad_alloc();
	}

	m_offset += alloc_size;
	return current;
}


void* MonotonicArena::do_allocate(std::size_t alloc_size, std::size_t alignment)
{
	std::byte* current = m_aligned_arena + m_offset;
	std::size_t available = m_size - m_offset;

	void* aligned = current;
	if (std::align(alignment, alloc_size, aligned, available) == nullptr) {
	    throw std::bad_alloc();
	}

	std::size_t adjustment = static_cast<std::byte*>(aligned) - current;
	std::size_t allocation = adjustment + alloc_size;

	if (m_offset + allocation > m_size) {
	    throw std::bad_alloc();
	}

	m_offset += allocation;
	return aligned;
}


std::byte* MonotonicArena::allocate_aligned(std::size_t alloc_size, std::size_t alignment)
{
	std::size_t total_size = alloc_size + alignment - 1 + sizeof(void*);
	std::byte* raw = reinterpret_cast<std::byte*>(::malloc(total_size));
	if (!raw) return nullptr;
	
	uintptr_t raw_address = reinterpret_cast<uintptr_t>(raw) + sizeof(void*);
	uintptr_t aligned_address = (raw_address + alignment - 1) & ~(alignment - 1);
	std::byte* aligned_ptr = reinterpret_cast<std::byte*>(aligned_address);
	
	reinterpret_cast<void**>(aligned_ptr)[-1] = raw;
	return aligned_ptr;
}


void MonotonicArena::free_aligned(std::byte* aligned_ptr)
{
	if (!aligned_ptr) return;
	void* raw = reinterpret_cast<void**>(aligned_ptr)[-1];
	::free(raw);
}
} // hpr
