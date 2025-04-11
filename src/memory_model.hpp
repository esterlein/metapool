#pragma once

#include "metapool.hpp"
#include "metapool_registry.hpp"
#include "allocator.hpp"
#include "monotonic_arena.hpp"


namespace hpr {
namespace mem {

	static inline constexpr std::size_t arena_size = 268435456; // 256 MB


	using StandardMetapoolRegistry =
		MetapoolRegistry <
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
			return get_thread_local_allocator<mem::StandardMetapoolRegistry>();
		}
		else if constexpr (Type == mem::AllocatorType::Standard) {
			return get_thread_local_allocator<mem::StandardMetapoolRegistry>();
		}
		else if constexpr (Type == mem::AllocatorType::Standard) {
			return get_thread_local_allocator<mem::StandardMetapoolRegistry>();
		}
	}

private:

	template <typename MetapoolRegistry>
	static auto create_metapools()
	{
		using MetapoolsTuple = typename MetapoolRegistry::tuple_type;

		thread_local static MonotonicArena arena{mem::arena_size, mem::cacheline};

		return []<std::size_t... Is>(std::index_sequence<Is...>) {
			return std::tuple<std::tuple_element_t<Is, MetapoolsTuple>...> {
				std::tuple_element_t<Is, MetapoolsTuple>(&arena)...
			};
		}(std::make_index_sequence<std::tuple_size_v<MetapoolsTuple>>{});
	}

	template <typename MetapoolsTuple>
	static auto create_descriptors(MetapoolsTuple& metapools)
	{
		return [&metapools]<std::size_t... Is>(std::index_sequence<Is...>) {
			return std::array<MetapoolDescriptor, sizeof...(Is)> {
				([&metapools]() -> MetapoolDescriptor {
					auto& pool = std::get<Is>(metapools);
					return pool.descriptor();
				}())...
			};
		}(std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(metapools)>>>{});
	}

	template <typename MetapoolRegistryType>
	static auto& create_thread_local_allocator()
	{
		thread_local static auto metapools = create_metapools<MetapoolRegistryType>();

		thread_local static auto descriptors = create_descriptors(metapools);

		using MetapoolDescriptorArray = std::decay_t<decltype(descriptors)>;
		using AllocatorConfigType = mem::AllocatorConfig<MetapoolDescriptorArray>;

		thread_local static constexpr AllocatorConfigType config {
			.alignment_quantum = mem::alignment_quantum,
			.alignment_shift   = MetapoolBase::alloc_header_size,
			.min_stride_step   = mem::min_stride_step,
			.min_stride        = MetapoolRegistryType::min_stride,
			.max_stride        = MetapoolRegistryType::max_stride
		};

		thread_local static Allocator<AllocatorConfigType> allocator(descriptors);

		return allocator;
	}
};
} // hpr
