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
	, m_pools   { compute_pools() }
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

	std::byte* base_memory = static_cast<std::byte*>(m_upstream->allocate(total_size, mem::cacheline));
	std::byte* current_memory = base_memory;

	std::visit(
		[current_memory](auto& freelist) {
			freelist.initialize(current_memory);
		}, m_pools[0].freelist);

	current_memory += m_pools[0].stride * m_pools[0].block_count;

	for (std::size_t i = 1; i < m_pools.size(); ++i) {
		uintptr_t current_addr = reinterpret_cast<uintptr_t>(current_memory);

		if (current_addr % mem::cacheline != 0) {
			uintptr_t aligned_addr = (current_addr + mem::cacheline - 1) & ~(mem::cacheline - 1);
			current_memory = reinterpret_cast<std::byte*>(aligned_addr);
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
	: m_upstream{std::exchange(other.m_upstream, nullptr)}
	, m_pools   {std::move(other.m_pools)}
{}


template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
Metapool<BasePoolBlockCount, StridePivots...>&
Metapool<BasePoolBlockCount, StridePivots...>::operator=(Metapool&& other) noexcept
{
	if (this != &other) {
			m_upstream = std::exchange(other.m_upstream, nullptr);
			m_pools = std::move(other.m_pools);
	}
	return *this;
}


template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
std::optional<std::size_t> Metapool<BasePoolBlockCount, StridePivots...>::get_pool_index(std::size_t stride)
{
	if (stride < m_pools.front().stride || stride > m_pools.back().stride)
		return std::nullopt;

	return (stride - m_pools.front().stride) / mem::stride_multiple;
}

template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
std::byte* Metapool<BasePoolBlockCount, StridePivots...>::fetch(std::size_t stride)
{
	if (stride < m_pools.front().stride || stride > m_pools.back().stride)
		throw std::bad_alloc{};

	auto& freelist_variant = m_pools[(stride - m_pools.front().stride) / 8].freelist;

	return std::visit([](auto&& freelist) -> std::byte* {
		auto* location = freelist.fetch();
		if (!location) {
			throw std::bad_alloc{};
		}
		return reinterpret_cast<std::byte*>(location);

	}, freelist_variant);
}

template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence<BasePoolBlockCount, StridePivots...>
void Metapool<BasePoolBlockCount, StridePivots...>::release(std::size_t stride, std::byte* location)
{
	if (stride < m_pools.front().stride || stride > m_pools.back().stride)
		throw std::runtime_error("no suitable pool found for release");

	auto& freelist_variant = m_pools[(stride - m_pools.front().stride) / 8].freelist;

	std::visit([location](auto&& freelist) {
		freelist.release(location);
	}, freelist_variant);
}
} // hpr
