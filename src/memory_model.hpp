#pragma once

#include <memory_resource>

#include "metapool.hpp"
#include "allocator.hpp"
#include "metapool.tpp"
#include "monotonic_arena.hpp"


namespace hpr {
namespace mem {

	static inline constexpr std::size_t arena_size = 268435456; // 256 MB


	template <typename... Metapools>
	struct MetapoolList
	{
		using tuple_type = std::tuple<Metapools...>;
		using variant_ptr = std::variant<Metapools*...>;
		static inline constexpr std::size_t count = sizeof...(Metapools);
	};

	using StandardMetapoolList =
		MetapoolList <
			Metapool<mem::MetapoolConfig<1024, 8, 16, 32, 64, 128, 256, 264>>
		>;


	enum class AllocatorType
	{
		Standard
	};

} // hpr::mem


class MemoryModel final
{
public:

	MemoryModel() = delete;

	template <mem::AllocatorType Type>
	static auto& get_allocator()
	{
		if constexpr (Type == mem::AllocatorType::Standard) {
			return create_thread_local_allocator<mem::StandardMetapoolList>();
		}
		else if constexpr (Type == mem::AllocatorType::Standard) {
			return create_thread_local_allocator<mem::StandardMetapoolList>();
		}
		else if constexpr (Type == mem::AllocatorType::Standard) {
			return create_thread_local_allocator<mem::StandardMetapoolList>();
		}
	}

private:

	template <typename MetapoolList>
	auto create_metapools()
	{
		using MetapoolsTuple = typename MetapoolList::tuple_type;

		thread_local static MonotonicArena arena{mem::arena_size, mem::cacheline};

		return []<std::size_t... I>(std::index_sequence<I...>) {
			return std::tuple<std::tuple_element_t<I, MetapoolsTuple>...> {
				std::tuple_element_t<I, MetapoolsTuple>(&arena)...
			};
		}(std::make_index_sequence<std::tuple_size_v<MetapoolsTuple>>{});
	}

	template <typename MetapoolsTuple>
	auto create_descriptors(MetapoolsTuple& metapools)
	{
		return [&metapools]<std::size_t... I>(std::index_sequence<I...>) {
			return std::array<MetapoolDescriptor, sizeof...(I)> {
				([&metapools]() -> MetapoolDescriptor {
					auto& pool = std::get<I>(metapools);
					return pool.get_descriptor();
				}())...
			};
		}(std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(metapools)>>>{});
	}

	template <typename MetapoolListType>
	auto& create_thread_local_allocator()
	{
		thread_local static auto metapools = create_metapools<MetapoolListType>();

		thread_local static auto descriptors = create_descriptors(metapools);

		using MetapoolDescriptorArray = std::decay_t<decltype(descriptors)>;
		using AllocatorConfigType = mem::AllocatorConfig<MetapoolDescriptorArray>;

		thread_local static constexpr AllocatorConfigType config {
			.alignment_quantum = mem::alignment_quantum,
			.alignment_shift = MetapoolBase::alloc_header_size
		};

		thread_local static Allocator<AllocatorConfigType> allocator(descriptors);

		return allocator;
	}
};
} // hpr
