#pragma once

#include <tuple>

#include "metapool.hpp"
#include "metapool_config.hpp"
#include "metapool_registry.hpp"
#include "allocator.hpp"
#include "allocator_interface.hpp"
#include "monotonic_arena.hpp"

#include "mtp_setup.hpp"


namespace hpr {


class MemoryModel final
{
public:

	MemoryModel() = delete;


	template <typename MetapoolRegistryType, mem::AllocatorInterface Interface>
	static auto& create_thread_local_allocator()
	{
		thread_local static MonotonicArena arena {
			MetapoolRegistryType::arena_size,
			mem::MetapoolConstraints::freelist_alignment
		};

		thread_local static MetapoolContainer<MetapoolRegistryType> container {&arena};

		thread_local static auto proxies = container.get_proxies();
		constexpr auto allocator_config = MetapoolRegistryType::create_allocator_config();

		if constexpr (Interface == mem::AllocatorInterface::native) {
			thread_local static Allocator<decltype(allocator_config), Native> alloc {proxies};
			return alloc;
		}
		else if constexpr (Interface == mem::AllocatorInterface::std_adapter) {
			thread_local static Allocator<decltype(allocator_config), StdAdapter, void> alloc {proxies};
			return alloc;
		}
		else if constexpr (Interface == mem::AllocatorInterface::pmr_adapter) {
			thread_local static Allocator<decltype(allocator_config), PmrAdapter> alloc {proxies};
			return alloc;
		}
	}

private:

	template <typename MetapoolRegistryType>
	class MetapoolContainer
	{
	public:

		explicit MetapoolContainer(MonotonicArena* upstream)
			: m_metapool_storage {
				create_storage(upstream, std::make_index_sequence<MetapoolRegistryType::registry_size>{})
			}
		{}

		template <std::size_t... Is>
		static auto create_storage(MonotonicArena* upstream, std::index_sequence<Is...>)
		{
			return std::make_tuple(
				std::tuple_element_t<Is, typename MetapoolRegistryType::TupleType>(upstream)...
			);
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
				(std::get<Is>(m_metapool_storage).proxy())...
			};
		}

		typename MetapoolRegistryType::TupleType m_metapool_storage;

	}; // MemoryModel::MetapoolContainer<typename MetapoolRegistryType>
};
} // hpr
