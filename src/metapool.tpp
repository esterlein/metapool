#pragma once

#include <cstdint>
#include <numeric>

#include "alloc_header.hpp"
#include "monotonic_arena.hpp"



namespace hpr {
namespace mem {

	static inline constexpr uint32_t cacheline = 64U;

}


template <mem::IsMetapoolConfig Config>
Metapool<Config>::Metapool(MonotonicArena* upstream)
	: m_upstream {upstream}
{
	assert(upstream != nullptr);

	uint32_t total_size = m_pools[0].stride * m_pools[0].block_count;

	if (m_pools.size() > 1) {
		uint32_t current_pos = total_size;

		total_size += std::accumulate(
			m_pools.begin() + 1, m_pools.end(), uint32_t(0),
			[&current_pos](uint32_t sum, const auto& pool) {
				if (current_pos % mem::cacheline != 0) {

					uint32_t aligned_pos =
						(current_pos + mem::cacheline - 1U) & ~(mem::cacheline - 1U);

					sum += (aligned_pos - current_pos);
					current_pos = aligned_pos;
				}
				sum += pool.stride * pool.block_count;
				current_pos += pool.stride * pool.block_count;
				return sum;
			}
		);
	}

	std::byte* base_memory = static_cast<std::byte*>(m_upstream->fetch(total_size, mem::cacheline));
	std::byte* current_memory = base_memory;

	std::visit(
		[current_memory](auto& freelist) {
			freelist.initialize(current_memory);
		}, m_pools[0].freelist);

	current_memory += m_pools[0].stride * m_pools[0].block_count;

	for (std::size_t i = 1; i < m_pools.size(); ++i) {
		uintptr_t current_addr = reinterpret_cast<uintptr_t>(current_memory);

		if ((current_addr + mem::alloc_header_size) % mem::cacheline != 0) {
			uintptr_t shift_aligned_addr =
				(current_addr + mem::alloc_header_size + mem::cacheline - 1U) & ~(mem::cacheline - 1U);
			current_memory = reinterpret_cast<std::byte*>(shift_aligned_addr - mem::alloc_header_size);
		}

		std::visit(
			[current_memory](auto& freelist) {
				freelist.initialize(current_memory);
			}, m_pools[i].freelist);

		current_memory += m_pools[i].stride * m_pools[i].block_count;
	}
}


template <mem::IsMetapoolConfig Config>
Metapool<Config>::~Metapool()
{
	if (!m_upstream)
		return;
	for (auto& pool : m_pools) {
		std::visit([](auto& freelist) {
			freelist.reset();
		}, pool.freelist);
	}
	m_upstream = nullptr;
}


template <mem::IsMetapoolConfig Config>
std::byte* Metapool<Config>::fetch(uint8_t pool_index)
{
	std::byte* block = m_pools[pool_index].fl_fetch(&m_pools[pool_index].freelist);

	if (!block) [[unlikely]]
		return nullptr;

	return std::launder(block);
}


template <mem::IsMetapoolConfig Config>
void Metapool<Config>::release(uint8_t pool_index, std::byte* block)
{
	if (!block) [[unlikely]]
		return;

	if (pool_index >= m_pools.size()) [[unlikely]]
		return;

	m_pools[pool_index].fl_release(&m_pools[pool_index].freelist, block);
}


template <mem::IsMetapoolConfig Config>
MetapoolProxy Metapool<Config>::create_proxy()
{
	return MetapoolProxy {
		{
			m_pools.front().stride,
			m_pools.back().stride
		},
		Config::stride_step,
		static_cast<void*>(this),
		[](void* mpool_this, std::size_t stride) -> std::byte* {
			return static_cast<Metapool*>(mpool_this)->fetch(stride);
		},
		[](void* mpool_this, std::byte* location) {
			static_cast<Metapool*>(mpool_this)->release(location);
		}
	};
}
} // hpr
