#include <memory>

#include "monotonic_arena.hpp"


namespace hpr {


MonotonicArena::MonotonicArena(std::size_t size, std::size_t alignment, std::size_t shift)
	: m_arena {allocate(size, alignment, shift)}
	, m_size  {size}
	, m_offset{0}
	, m_shift {shift}
{
	if (!m_arena) {
		throw std::bad_alloc();
	}
}


MonotonicArena::~MonotonicArena()
{
	deallocate(m_arena);
}


MonotonicArena::MonotonicArena(MonotonicArena&& other) noexcept
	: m_arena {other.m_arena}
	, m_size  {other.m_size}
	, m_offset{other.m_offset}
	, m_shift {other.m_shift}
{
	other.m_arena  = nullptr;
	other.m_size   = 0;
	other.m_offset = 0;
	other.m_shift  = 0;
}


MonotonicArena& MonotonicArena::operator=(MonotonicArena&& other) noexcept
{
	if (this != &other) {
		if (m_arena) {
			deallocate(m_arena);
		}
		m_arena  = other.m_arena;
		m_size   = other.m_size;
		m_offset = other.m_offset;
		m_shift  = other.m_shift;
		other.m_arena  = nullptr;
		other.m_size   = 0;
		other.m_offset = 0;
		other.m_shift  = 0;
	}
	return *this;
}


std::byte* MonotonicArena::allocate(std::size_t alloc_size, std::size_t alignment, std::size_t shift)
{
	std::size_t total_size = alloc_size + shift + alignment - 1 + sizeof(void*);
	std::byte* raw = reinterpret_cast<std::byte*>(::malloc(total_size));
	if (!raw) return nullptr;
	
	uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw) + sizeof(void*);
	uintptr_t aligned_addr = (raw_addr + shift + alignment - 1) & ~(alignment - 1);
	std::byte* base_addr = reinterpret_cast<std::byte*>(aligned_addr - shift);
	
	reinterpret_cast<void**>(base_addr)[-1] = raw;
	return base_addr;
}


std::byte* MonotonicArena::fetch(std::size_t alloc_size, std::size_t alignment)
{
	if (alloc_size == 0) {
		return nullptr;
	}
	std::byte* current = m_arena + m_offset;
	std::size_t available = m_size - m_offset;
	void* aligned_ptr = current;
	if (std::align(alignment, alloc_size, aligned_ptr, available) == nullptr) {
		throw std::bad_alloc();
	}
	std::size_t adjustment = static_cast<std::size_t>(static_cast<std::byte*>(aligned_ptr) - current);
	std::size_t allocation_total = adjustment + alloc_size;
	if (m_offset + allocation_total > m_size) {
		throw std::bad_alloc();
	}
	m_offset += allocation_total;
	return static_cast<std::byte*>(aligned_ptr);
}


void MonotonicArena::deallocate(std::byte* base_addr)
{
	if (!base_addr) return;
	void* raw = reinterpret_cast<void**>(base_addr)[-1];
	::free(raw);
}
} // hpr
