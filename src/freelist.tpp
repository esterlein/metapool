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
	m_memory_end  = memory + (static_cast<std::size_t>(BlockCount) * Stride);

	m_head = reinterpret_cast<Block*>(memory);
	Block* current = m_head;

	for (auto i = 1; i < BlockCount; ++i) {

		if (i > std::numeric_limits<uint32_t>::max() / Stride) { [[unlikely]]
			throw std::runtime_error {"integer overflow in freelist initialization"};
		}

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
	}
	return pop();
}


template <uint32_t Stride, uint32_t BlockCount>
void Freelist<Stride, BlockCount>::release(std::byte* block) noexcept
{
	assert(block != nullptr);
	if (block < m_memory_base || block >= m_memory_end) { [[unlikely]]
		assert(false && "block does not belong to this freelist");
		return;
	}
	push(block);
}
} // hpr
