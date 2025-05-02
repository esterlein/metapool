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


	template <typename System>
	struct SystemAllocatorTraits;

	struct BasicTestsSystem {};
	struct BenchmarkSimpleSystem {};
	struct BenchmarkIntermediateSystem {};

	template <> struct SystemAllocatorTraits<BasicTestsSystem>
	{
		static constexpr mem::AllocatorType type = mem::AllocatorType::simple;
	};

	template <> struct SystemAllocatorTraits<BenchmarkSimpleSystem>
	{
		static constexpr mem::AllocatorType type = mem::AllocatorType::simple;
	};

	template <> struct SystemAllocatorTraits<BenchmarkIntermediateSystem>
	{
		static constexpr mem::AllocatorType type = mem::AllocatorType::intermediate;
	};

} // hpr::mem


class MemoryModel final
{
public:

	MemoryModel() = delete;

	template <typename System>
	static auto& get_system_allocator()
	{
		constexpr auto type = mem::SystemAllocatorTraits<System>::type;
		return get_allocator<type, mem::AllocatorInterface::std_adapter>();
	}

	template <typename T, typename System>
	static auto get_adapter_std()
	{
		auto& base = get_system_allocator<System>();

		using Base = std::remove_reference_t<decltype(base)>;
		using Rebound = typename Base::template rebind<T>::other;

		return Rebound {base};
	}


	template <typename System>
	using SysAlloc = decltype(get_system_allocator<System>());

	template <typename T, typename System>
	using SysAdapter = decltype(get_adapter_std<T, System>());


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
