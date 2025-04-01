#pragma once

#include <cstdint>
#include <numeric>
#include <optional>


namespace hpr {
namespace mem {

	static inline constexpr std::size_t cacheline = 64;
}


template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
Metapool<BasePoolBlockCount, StridePivots...>::Metapool(std::pmr::memory_resource* upstream)
	: m_upstream{ upstream }
{
	assert(upstream != nullptr);

	std::size_t total_size = m_pools[0].stride * m_pools[0].block_count;

	if (m_pools.size() > 1) {
		std::size_t current_pos = total_size;

		total_size += std::accumulate(
			m_pools.begin() + 1, m_pools.end(), std::size_t(0),
			[&current_pos](std::size_t sum, const auto& pool) {
				if (current_pos % mem::cacheline != 0) {

					std::size_t aligned_pos =
						(current_pos + mem::cacheline - 1UL) & ~(mem::cacheline - 1UL);

					sum += (aligned_pos - current_pos);
					current_pos = aligned_pos;
				}
				sum += pool.stride * pool.block_count;
				current_pos += pool.stride * pool.block_count;
				return sum;
			}
		);
	}

	std::byte* base_memory = static_cast<std::byte*>(m_upstream->allocate_aligned(total_size, mem::cacheline, alloc_header_size));
	std::byte* current_memory = base_memory;

	std::visit(
		[current_memory](auto& freelist) {
			freelist.initialize(current_memory);
		}, m_pools[0].freelist);

	current_memory += m_pools[0].stride * m_pools[0].block_count;

	for (std::size_t i = 1; i < m_pools.size(); ++i) {
		uintptr_t current_addr = reinterpret_cast<uintptr_t>(current_memory);

		if ((current_addr + alloc_header_size) % mem::cacheline != 0) {
			uintptr_t shift_aligned_addr = (current_addr + alloc_header_size + mem::cacheline - 1) & ~(mem::cacheline - 1);
			current_memory = reinterpret_cast<std::byte*>(shift_aligned_addr - alloc_header_size);
		}

		std::visit(
			[current_memory](auto& freelist) {
				freelist.initialize(current_memory);
			}, m_pools[i].freelist);

		current_memory += m_pools[i].stride * m_pools[i].block_count;
	}
}


template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
Metapool<BasePoolBlockCount, StridePivots...>::~Metapool()
{}


template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
Metapool<BasePoolBlockCount, StridePivots...>::Metapool(Metapool&& other) noexcept
	: m_upstream    {std::exchange(other.m_upstream, nullptr)}
	, m_pools       {std::move(other.m_pools)}
	, m_lookup_table{std::move(other.m_lookup_table)}
{}


template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
Metapool<BasePoolBlockCount, StridePivots...>&
Metapool<BasePoolBlockCount, StridePivots...>::operator=(Metapool&& other) noexcept
{
	if (this != &other) {
		m_upstream = std::exchange(other.m_upstream, nullptr);
		m_pools = std::move(other.m_pools);
		m_lookup_table = std::move(other.m_lookup_table);
	}
	return *this;
}


template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
std::optional<std::size_t> Metapool<BasePoolBlockCount, StridePivots...>::get_pool_index(std::size_t stride)
{
	return std::nullopt;
}

template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
std::byte* Metapool<BasePoolBlockCount, StridePivots...>::fetch(std::size_t stride)
{
	if (stride < m_pools.front().stride || stride > m_pools.back().stride)
		throw std::bad_alloc{};

	const auto pool_index = (stride - m_pools.front().stride) / 8;
	const auto& [_, functions] = lookup_table[pool_index];
	const auto& [fetch_func, _] = functions;

	std::byte* block = fetch_func(&m_pools[pool_index].freelist);
	if (!block)
		throw std::bad_alloc{};

	auto* header = reinterpret_cast<AllocationHeader*>(block);
	header->pool_index = pool_index;
	header->magic = 0xABCD;

	std::byte* object_location = block + sizeof(AllocationHeader);

	T* object = std::launder(new (object_location) T(std::forward<Types>(args)...));
	return object;
}

template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
void Metapool<BasePoolBlockCount, StridePivots...>::release(std::byte* location)
{
	if (!location) return;

	std::byte* block = location - sizeof(AllocationHeader);
	auto* header = reinterpret_cast<AllocationHeader*>(block);

	if (header->magic != 0xABCD)
		throw std::runtime_error("memory corruption detected");

	const auto pool_index = header->pool_index;

	const auto& [_, functions] = lookup_table[pool_index];
	const auto& [_, release_func] = functions;
	release_func(&m_pools[pool_index].freelist, block);
}
} // hpr
