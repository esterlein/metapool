#pragma once

#include <memory_resource>

#include "metapool.hpp"
#include "allocator.hpp"
#include "monotonic_arena.hpp"


namespace hpr {
namespace mem {

	static inline constexpr std::size_t arena_size = 268435456; // 256 MB


	template <typename... Metapools>
	struct MetapoolList
	{
		using tuple_type = std::tuple<Metapools...>;
		using ptr_variant = std::variant<Metapools*...>;
		static inline constexpr std::size_t count = sizeof...(Metapools);
	};


	using DefaultMetapoolList =
		MetapoolList<
			Metapool<mem::MetapoolConfig<1024, 8, 16, 32, 64, 128, 256, 264>>
		>;


	constexpr AllocatorConfig make_allocator_config(auto count)
	{
		return AllocatorConfig {
			.metapool_count = static_cast<uint32_t>(count),
			.alignment_quantum = alignment_quantum,
			.alignment_shift = MetapoolBase::alloc_header_size
		};
	}

} // hpr::mem


class MemoryModel final
{
public:

	constexpr MemoryModel()
		: m_arena    {mem::arena_size, mem::cacheline}
		, m_metapools{create_metapools<typename DefaultMetapoolList::tuple_type>(&m_arena)}
		, m_allocator{create_descriptors()}
	{}

	static inline Allocator create_allocator()
	{

	}

private:

	template <typename MetapoolsTuple>
	static auto create_metapools(std::pmr::memory_resource* upstream)
	{
		return [upstream]<std::size_t... I>(std::index_sequence<I...>) {
			return std::tuple<std::tuple_element_t<I, MetapoolsTuple>...>{
				std::tuple_element_t<I, MetapoolsTuple>(upstream)...
			};
		}(std::make_index_sequence<std::tuple_size_v<MetapoolsTuple>>{});
	};

	constexpr auto create_descriptors()
	{
		return [this]<std::size_t... I>(std::index_sequence<I...>) {
			return std::array<MetapoolDescriptor, sizeof...(I)>{
				([this]() -> MetapoolDescriptor {
					auto [low, high] = std::get<I>(m_metapools).get_bounds();
					return MetapoolDescriptor{low, high, &std::get<I>(m_metapools)};
				}())...
			};
		}(std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(m_metapools)>>>{});
	};

private:

	MonotonicArena m_arena;

	typename mem::DefaultMetapoolList::tuple_type m_metapools;

	Allocator<DefaultMetapoolList::count> m_allocator;
};
} // hpr
