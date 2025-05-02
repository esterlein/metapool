#pragma once

#include <tuple>

#include "metapool.hpp"
#include "metapool_config.hpp"
#include "metapool_registry.hpp"
#include "allocator.hpp"
#include "monotonic_arena.hpp"


namespace hpr {
namespace mem {


	static inline constexpr std::size_t arena_size = 4294967296; // 4GB

	using BenchmarkSimpleRegistry =
		MetapoolRegistry <
			Metapool<mem::MetapoolConfig<mem::CapacityFunction::Flat, 2048, 8, 8, 2048>>
		>;

	using BenchmarkIntermediateRegistry =
		MetapoolRegistry <
			Metapool<mem::MetapoolConfig<mem::CapacityFunction::Flat, 6144,   8,     8,   128>>,
			Metapool<mem::MetapoolConfig<mem::CapacityFunction::Flat, 1024,   8,   128,  2048>>,
			Metapool<mem::MetapoolConfig<mem::CapacityFunction::Flat,  256, 256,  2048, 16384>>,
			Metapool<mem::MetapoolConfig<mem::CapacityFunction::Flat, 4096, 512, 16384, 32768>>
		>;


	enum class AllocatorType
	{
		simple,
		intermediate
	};
	
	enum class AllocatorInterface
	{
		native,
		std_adapter,
		pmr_adapter
	};

	static inline constexpr AllocatorType k_default_allocator_type = mem::AllocatorType::simple;

} // hpr::mem


class MemoryModel final
{
public:

	MemoryModel() = delete;

	template <mem::AllocatorType Type, mem::AllocatorInterface Interface>
	static auto& get_allocator()
	{
		if constexpr (Type == mem::AllocatorType::simple) {
			return create_thread_local_allocator<mem::BenchmarkSimpleRegistry, Interface>();
		}
		else if constexpr (Type == mem::AllocatorType::intermediate) {
			return create_thread_local_allocator<mem::BenchmarkIntermediateRegistry, Interface>();
		}
	}

	template <typename T>
	auto get_std_adapter()
	{
		auto& base = get_allocator<k_default_allocator_type, mem::AllocatorInterface::std_adapter>();

		using Base = std::remove_reference_t<decltype(base)>;
		using Rebound = typename Base::template rebind<T>::other;

		return Rebound{base};
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


	template <typename MetapoolRegistryType, mem::AllocatorInterface Interface>
	static auto& create_thread_local_allocator()
	{
		thread_local static MonotonicArena arena {mem::arena_size, mem::cacheline};
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
};

} // hpr
