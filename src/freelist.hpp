#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>


namespace hpr {


template <uint32_t Stride, uint32_t BlockCount>
class Freelist final
{

	static_assert(Stride >= sizeof(void*), "stride is smaller than a pointer");

public:

	Freelist() = default;

	void initialize(std::byte* memory) noexcept;
	[[nodiscard]] std::byte* fetch() noexcept;
	void release(std::byte* block) noexcept;

	inline void reset() noexcept { m_head = nullptr; }
	[[nodiscard]] inline bool empty() const noexcept
	{ return m_head == nullptr; }
	[[nodiscard]] inline constexpr uint32_t stride() const noexcept
	{ return Stride; }
	[[nodiscard]] inline constexpr uint32_t block_count() const noexcept
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

	static_assert(sizeof(Block) <= Stride, "block is larger than stride");

	Block* m_head = nullptr;

private:

	// debugging
	uint32_t m_block_count = BlockCount;
	const uint32_t m_total_block_count = BlockCount;

	inline std::byte* pop() noexcept
	{
		Block* block = m_head;
		m_head = m_head->next;
		--m_block_count;
		return block->get_memory();
	}

	inline void push(std::byte* location) noexcept
	{
		auto* block = reinterpret_cast<Block*>(location);
		block->next = m_head;
		m_head = block;
		++m_block_count;
	}
};

} // hpr

#include "freelist.tpp"
