#pragma once

#include <tuple>

#include "metapool.hpp"
#include "metapool_config.hpp"
#include "metapool_registry.hpp"
#include "allocator.hpp"
#include "monotonic_arena.hpp"


namespace hpr {
namespace mem {


	static inline constexpr std::size_t arena_size = 268435456; // 256 MB


	using StandardMetapoolRegistry =
		MetapoolRegistry <
			Metapool<mem::MetapoolConfig<1024, 8, 16, 32, 64, 128, 256>>
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
			return create_thread_local_allocator<mem::StandardMetapoolRegistry>();
		}
	}

private:

	template <typename MetapoolRegistryType>
	class MetapoolContainer
	{
	public:

		explicit MetapoolContainer(MonotonicArena* arena)
			: metapool_storage{create_storage(arena, std::make_index_sequence<MetapoolRegistryType::registry_size>{})}
		{}

		template <std::size_t... Is>
		static auto create_storage(MonotonicArena* arena, std::index_sequence<Is...>)
		{
			using Tuple = typename MetapoolRegistryType::tuple_type;
			using std::tuple_element_t;

			return typename MetapoolRegistryType::tuple_type {
				(tuple_element_t<Is, Tuple>(arena))...
			};
		}

		auto get_proxies()
		{
			return create_proxies(std::make_index_sequence<MetapoolRegistryType::registry_size>{});
		}

	private:

		template <std::size_t... Is>
		auto create_proxies(std::index_sequence<Is...>)
		{
			return std::array<MetapoolProxy, sizeof...(Is)> {
				(std::get<Is>(metapool_storage).proxy())...
			};
		}


		typename MetapoolRegistryType::tuple_type metapool_storage;

	}; // MemoryModel::MetapoolContainer


	template <typename MetapoolRegistryType>
	static auto& create_thread_local_allocator()
	{
		thread_local static MonotonicArena arena{mem::arena_size, mem::cacheline};
		thread_local static MetapoolContainer<MetapoolRegistryType> container(&arena);

		thread_local static auto& proxies = container.get_proxies();
		
		constexpr auto allocator_config = MetapoolRegistryType::create_allocator_config();
		thread_local static Allocator<decltype(allocator_config)> allocator(proxies);
		return allocator;
	}
};

} // hpr
