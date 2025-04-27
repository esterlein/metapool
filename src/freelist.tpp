#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>


namespace hpr {


template <uint32_t Stride, uint32_t BlockCount>
void Freelist<Stride, BlockCount>::initialize(std::byte* memory)
{
	assert(memory != nullptr && BlockCount > 0);

	m_memory_base = memory;
	m_memory_end  = memory + (size_t(BlockCount) * Stride);

	m_head = reinterpret_cast<Block*>(memory);
	Block* current = m_head;

	for (uint32_t i = 1; i < BlockCount; ++i) {

		size_t offset = size_t(i) * Stride;
		Block* next = reinterpret_cast<Block*>(memory + offset);

		if (i > std::numeric_limits<uint32_t>::max() / Stride) { [[unlikely]]
			throw std::runtime_error{"integer overflow in freelist initialization"};
		}

		current->next = next;
		current = next;
	}

	current->next = nullptr;
}


template <uint32_t Stride, uint32_t BlockCount>
void Freelist<Stride, BlockCount>::reset() noexcept
{
	m_head = reinterpret_cast<Block*>(m_memory_base);
	Block* current = m_head;

	std::byte* ptr = m_memory_base + Stride;
	for (uint32_t i = 1; i < BlockCount; ++i, ptr += Stride) {
		current->next = reinterpret_cast<Block*>(ptr);
		current = current->next;
	}

	current->next = nullptr;
}


template <uint32_t Stride, uint32_t BlockCount>
std::byte* Freelist<Stride, BlockCount>::fetch() noexcept
{
	assert(m_head != nullptr &&
		"head is nullptr"    && __func__);

	return pop();
}


template <uint32_t Stride, uint32_t BlockCount>
void Freelist<Stride, BlockCount>::release(std::byte* block) noexcept
{
	assert(block != nullptr &&
		"block is nullptr"  && __func__);
	assert(block >= m_memory_base || block < m_memory_end &&
		"block does not belong to this freelist" && __func__);

	push(block);
}
} // hpr
