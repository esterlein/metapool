#pragma once

#include "mtpint.hpp"

#include <cassert>

#include "fail.hpp"


namespace mtp::core {


template <uint32_t Stride, uint32_t BlockCount>
class Freelist final
{
	static_assert(Stride >= sizeof(void*),
		FREELIST_STRIDE_TOO_SMALL_MSG);

public:

	Freelist() = default;
	~Freelist() = default;

	Freelist(const Freelist&) = default;
	Freelist& operator=(const Freelist&) = default;
	Freelist(Freelist&&) = default;
	Freelist& operator=(Freelist&&) = default;

	using proxy_index_t = uint16_t;


	void initialize(std::byte* memory, proxy_index_t proxy_index)
	{
		MTP_ASSERT(memory != nullptr,
			mtp::err::init_memory_null);

		MTP_ASSERT(reinterpret_cast<std::uintptr_t>(memory) % alignof(Block) == 0,
			mtp::err::init_base_misaligned);

		const size_t total_bytes = static_cast<size_t>(BlockCount) * Stride;

		m_memory_base = memory;
		m_memory_end  = memory + total_bytes;

		m_head = reinterpret_cast<Block*>(memory);
		Block* current = m_head;

		for (size_t i = 0; i < static_cast<size_t>(BlockCount); ++i) {
			std::byte* block_ptr = memory + i * Stride;
			Block* block = reinterpret_cast<Block*>(block_ptr);

			std::byte* header_ptr = block->get_memory() - sizeof(proxy_index_t);

			header_ptr[0] = static_cast<std::byte>(proxy_index & 0xFF);
			header_ptr[1] = static_cast<std::byte>((proxy_index >> 8) & 0xFF);

			if (i + 1 < BlockCount) {

				Block* next = reinterpret_cast<Block*>(memory + (i + 1) * Stride);

				MTP_ASSERT(reinterpret_cast<std::uintptr_t>(next) % alignof(Block) == 0,
					mtp::err::init_next_misaligned);

				block->next = next;
			}
			else {
				block->next = nullptr;
			}
		}
	}


	[[nodiscard]] inline std::byte* fetch() noexcept
	{
		if (m_head == nullptr) [[unlikely]]
			return nullptr;

		Block* block = m_head;
		m_head = m_head->next;
		return block->get_memory();
	}

	inline void release(std::byte* block) noexcept
	{
		MTP_ASSERT(block != nullptr,
			 mtp::err::release_block_null);
		MTP_ASSERT(block >= m_memory_base && block < m_memory_end,
			mtp::err::release_block_outside);

		auto* b = reinterpret_cast<Block*>(block);
		b->next = m_head;
		m_head = b;
	}

	inline void reset() noexcept
	{
		m_head = reinterpret_cast<Block*>(m_memory_base);
		Block* current = m_head;

		std::byte* ptr = m_memory_base + Stride;
		for (size_t i = 1; i < static_cast<size_t>(BlockCount); ++i, ptr += Stride) {
			current->next = reinterpret_cast<Block*>(ptr);
			current = current->next;
		}

		current->next = nullptr;
	}

	[[nodiscard]] bool empty() const noexcept
	{ return m_head == nullptr; }

	[[nodiscard]] constexpr uint32_t stride() const noexcept
	{ return Stride; }

	[[nodiscard]] constexpr uint32_t block_count() const noexcept
	{ return BlockCount; }

private:

	struct Block
	{
		union {
			Block* next;
			std::byte data[Stride];
		};

		inline std::byte* get_memory() noexcept { return data; }
		inline const std::byte* get_memory() const noexcept { return data; }
	};

	static_assert(sizeof(Block) <= Stride,
		FREELIST_BLOCK_TOO_LARGE_MSG);

	Block* m_head {nullptr};

	std::byte* m_memory_base {nullptr};
	std::byte* m_memory_end  {nullptr};
};
} // mtp::core

