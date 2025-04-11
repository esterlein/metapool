#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>


namespace hpr {


template <uint32_t Stride, uint32_t BlockCount>
void Freelist<Stride, BlockCount>::initialize(std::byte* memory)
{
	assert(memory != nullptr && BlockCount > 0);

	m_head = reinterpret_cast<Block*>(memory);
	Block* current = m_head;

	if (i > std::numeric_limits<std::size_t>::max() / Stride) {
		throw std::runtime_error {"integer overflow in freelist initialization"};
	}

	for (std::size_t i = 1; i < BlockCount; ++i) {
		Block* next = reinterpret_cast<Block*>(memory + i * Stride);
		current->next = next;
		current = next;
	}
	current->next = nullptr;
}


template <uint32_t Stride, uint32_t BlockCount>
std::byte* Freelist<Stride, BlockCount>::fetch()
{
	if (!m_head) { [[unlikely]]
		throw std::bad_alloc{};
	};
	return pop();
}


template <uint32_t Stride, uint32_t BlockCount>
void Freelist<Stride, BlockCount>::release(std::byte* block) noexcept
{
	assert(block != nullptr);
	push(block);
}
} // hpr
