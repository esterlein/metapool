#include <memory>
#include <cassert>

#include "fail.hpp"
#include "monotonic_arena.hpp"


namespace hpr {


MonotonicArena::MonotonicArena(std::size_t size, std::size_t alignment)
	: m_size {size}
{
	m_arena = allocate(size, alignment, 0);

	assert(m_arena != nullptr);
}


MonotonicArena::~MonotonicArena()
{
	deallocate(m_arena);
}


std::byte* MonotonicArena::allocate(std::size_t alloc_size, std::size_t alignment, std::size_t shift)
{
	if ([[unlikely]] alloc_size > std::numeric_limits<std::size_t>::max() - shift - alignment - sizeof(void*)) {
		fatal("arena allocation size overflow");
	}

	std::size_t total_size = alloc_size + shift + alignment - 1 + sizeof(void*);

	std::byte* raw = reinterpret_cast<std::byte*>(::malloc(total_size));
	if ([[unlikely]] !raw){
		fatal("arena malloc failed");
	}
	
	uintptr_t raw_addr = reinterpret_cast<uintptr_t>(raw) + sizeof(void*);
	uintptr_t aligned_addr = (raw_addr + shift + alignment - 1) & ~(alignment - 1);

	if ([[unlikely]] aligned_addr - shift < reinterpret_cast<uintptr_t>(raw) + sizeof(void*)) {
		::free(raw);
		fatal("arena alignment shift results in invalid allocation address");
	}

	std::byte* base_addr = reinterpret_cast<std::byte*>(aligned_addr - shift);
	
	reinterpret_cast<void**>(base_addr)[-1] = raw;
	return base_addr;
}


std::byte* MonotonicArena::fetch(std::size_t alloc_size, std::size_t alignment, std::size_t shift)
{
	if ([[unlikely]] alloc_size == 0) {
		return nullptr;
	}

	std::byte* current = m_arena + m_offset;
	std::size_t available = m_size - m_offset;

	void* aligned_ptr = current + shift;
	if ([[unlikely]] std::align(alignment, alloc_size, aligned_ptr, available) == nullptr) {
		fatal("arena fetch failed: not enough space to fit aligned allocation");
	}

	std::byte* data_ptr = static_cast<std::byte*>(aligned_ptr);
	std::byte* base_ptr = data_ptr - shift;

	std::size_t adjustment = static_cast<std::size_t>(data_ptr - current);
	std::size_t allocation_total = adjustment + alloc_size;

	if ([[unlikely]] m_offset + allocation_total > m_size) {
		fatal("arena fetch allocation exceeds available capacity");
	}

	m_offset += allocation_total;

	return base_ptr;
}


void MonotonicArena::deallocate(std::byte* base_addr)
{
	if (!base_addr) return;
	void* raw = reinterpret_cast<void**>(base_addr)[-1];
	::free(raw);
}
} // hpr
