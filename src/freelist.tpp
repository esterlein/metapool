#pragma once

#include <cassert>
#include <cstdint>
#include <cstddef>

#include "fail.hpp"


namespace hpr {


template <uint32_t Stride, uint32_t BlockCount>
void Freelist<Stride, BlockCount>::initialize(std::byte* memory)
{
	assert(memory != nullptr &&
		THIS_FUNC && ": passed memory is nullptr");

	m_memory_base = memory;
	m_memory_end  = memory + (size_t(BlockCount) * Stride);

	m_head = reinterpret_cast<Block*>(memory);
	Block* current = m_head;

	for (uint32_t i = 1; i < BlockCount; ++i) {

		size_t offset = size_t(i) * Stride;
		Block* next = reinterpret_cast<Block*>(memory + offset);

		if (i > std::numeric_limits<uint32_t>::max() / Stride) [[unlikely]] {
			fatal("freelist::initialize(): integer overflow");
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
		THIS_FUNC && ": head is nullptr");
	return pop();
}


template <uint32_t Stride, uint32_t BlockCount>
void Freelist<Stride, BlockCount>::release(std::byte* block) noexcept
{
	assert(block != nullptr &&
		THIS_FUNC && ": block is nullptr");
	assert(block >= m_memory_base && block < m_memory_end &&
		THIS_FUNC && ": block does not belong to this freelist");
	push(block);
}
} // hpr
