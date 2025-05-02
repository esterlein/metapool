#pragma once

#include "memory_model.hpp"


namespace hpr {
namespace mem {


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


	template <typename System>
	static auto& get_system_allocator()
	{
		constexpr auto type = mem::SystemAllocatorTraits<System>::type;
		return MemoryModel::get_allocator<type, mem::AllocatorInterface::std_adapter>();
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

} // hpr::mem
} // hpr

