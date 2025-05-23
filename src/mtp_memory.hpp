#pragma once

#include <memory_resource>

#include "memory_model.hpp"

#include "mtp_setup.hpp"


namespace mtp {


	template <typename System>
	static auto& get_system_allocator()
	{
		constexpr auto type = hpr::mem::SystemAllocatorTraits<System>::type;
		return MemoryModel::get_allocator<type, AllocatorInterface::std_adapter>();
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
	static inline std::pmr::memory_resource* get_adapter_pmr()
	{
		constexpr auto type = mem::SystemAllocatorTraits<System>::type;
		return &MemoryModel::get_allocator<type, AllocatorInterface::pmr_adapter>();
	}


	template <typename System>
	using SysAlloc = decltype(get_system_allocator<System>());

	template <typename T, typename System>
	using SysAdapter = decltype(get_adapter_std<T, System>());

} // mtp
