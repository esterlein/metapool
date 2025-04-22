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

	for (std::size_t pool_index = 0; pool_index < m_pools.size(); ++pool_index) {

		auto& pool = m_pools[pool_index];

		std::size_t pool_size = pool.stride * pool.block_count;

		std::byte* pool_memory = m_upstream->fetch(pool_size, mem::cacheline);

		fprintf(stderr,
			"init pool %zu: ptr = %p  stride = %u  blocks = %u  end = %p\n",
			pool_index,
			(void*)pool_memory,
			pool.stride,
			pool.block_count,
			(void*)(pool_memory + pool_size)
		);
		fflush(stderr);

		std::visit(
			[pool_memory](auto& freelist) {
				freelist.initialize(pool_memory);
			},
			pool.freelist
		);
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

	if constexpr (std::is_constant_evaluated()) {
		static_assert(block != nullptr,
			"block is nullptr in metapool fetch");
	}
	else {
		assert(block != nullptr &&
			"block is nullptr"  && __func__);
	}

	return std::launder(block);
}


template <mem::IsMetapoolConfig Config>
void Metapool<Config>::release(uint8_t pool_index, std::byte* block)
{
	if constexpr (std::is_constant_evaluated()) {
		static_assert(block != nullptr,
			"block is nullptr in metapool fetch");
		static_assert(pool_index < m_pools.size(),
			"pool index out of bounds in metapool fetch");
	}
	else {
		assert(block != nullptr &&
			"block is nullptr"  && __func__);
		assert(pool_index < m_pools.size() &&
			"pool index out of bounds"     && __func__);
	}

	m_pools[pool_index].fl_release(&m_pools[pool_index].freelist, block);
}


template <mem::IsMetapoolConfig Config>
MetapoolProxy Metapool<Config>::create_proxy()
{
	return MetapoolProxy {

		static_cast<void*>(this),

		[](void* mpool_this, uint8_t pool_index) -> std::byte* {
			return static_cast<Metapool*>(mpool_this)->fetch(pool_index);
		},

		[](void* mpool_this, uint8_t pool_index, std::byte* block) {
			static_cast<Metapool*>(mpool_this)->release(pool_index, block);
		}
	};
}
} // hpr
