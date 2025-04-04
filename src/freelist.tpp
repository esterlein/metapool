#pragma once

#include <cassert>
#include <cstdint>


namespace hpr {


template <uint32_t Stride, uint32_t BlockCount>
void Freelist<Stride, BlockCount>::initialize(std::byte* memory) noexcept
{
	assert(memory != nullptr && BlockCount > 0);

	m_head = reinterpret_cast<Block*>(memory);
	Block* current = m_head;

	for (std::size_t i = 1; i < BlockCount; ++i) {
		Block* next = reinterpret_cast<Block*>(memory + i * Stride);
		current->next = next;
		current = next;
	}
	current->next = nullptr;
}


template <uint32_t Stride, uint32_t BlockCount>
std::byte* Freelist<Stride, BlockCount>::fetch() noexcept
{
	if (!m_head) return nullptr;
	return pop();
}


template <std::size_t Stride, std::size_t BlockCount>
void Freelist<Stride, BlockCount>::release(std::byte* block) noexcept
{
	assert(block != nullptr);
	push(block);
}
} // hpr
