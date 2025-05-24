#pragma once

#include <memory_resource>

#include "memory_model.hpp"
#include "allocator_interface.hpp"

#include "mtp_setup.hpp"


namespace mtp {


using hpr::mem::AllocatorInterface;


template <typename Registry, AllocatorInterface Interface>
static auto& get_allocator()
{
	return hpr::MemoryModel::create_thread_local_allocator<Registry, Interface>();
}


template <typename System>
static auto& get_system_allocator()
{
	return get_allocator<RegistryForSystemT<System>, AllocatorInterface::std_adapter>();
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
	auto& allocator = get_allocator<RegistryForSystemT<System>, AllocatorInterface::pmr_adapter>();
	return static_cast<std::pmr::memory_resource*>(&allocator);
}


template <typename System>
using SysAlloc = decltype(get_system_allocator<System>());

template <typename T, typename System>
using SysAdapter = decltype(get_adapter_std<T, System>());

} // mtp
