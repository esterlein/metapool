#pragma once

#include <array>
#include <tuple>
#include <variant>

#include "metapool.hpp"


namespace hpr {

class MetapoolDescriptor
{
public:

	using MetapoolAllocate = void* (*)(void* metapool, std::size_t stride);
	using MetapoolDeallocate = void (*)(void* metapool, std::byte* block);

	using MetapoolFetch = void* (*)(void* metapool, std::size_t stride);
	using MetapoolRelease = void (*)(void* metapool, std::byte* location);

	template <typename T, typename... Args>
	using MetapoolConstruct = T* (*)(void* metapool, std::size_t stride, Args&&...);
	template <typename T>
	using MetapoolDestruct = void (*)(void* metapool, T* object);

	std::size_t lower_bound;
	std::size_t upper_bound;

	MetapoolAllocate allocate_func = fetch_func;
	MetapoolDeallocate deallocate_func = release_func;

	MetapoolFetch fetch_func;
	MetapoolRelease release_func;

	template <typename T, typename... Args>
	T* construct(Args&&... args) const
	{
		std::size_t stride = compute_stride<T>();
		auto construct_fn = get_construct_func<T, Args...>();
		return construct_func(stride, std::forward<Args>(args)...);
	}

	template <typename T>
	void destruct(T* object) const
	{
		auto destruct_fn = get_destruct_func<T>();
		destruct_func(object);
	}

private:

	template <typename T, typename... Types>
	MetapoolConstruct<T, Types...> get_construct_func() const
	{

	}

	template <typename T>
	MetapoolDestruct<T> get_destruct_func() const
	{

	}
};


template <typename T, std::size_t MetapoolCount>
concept metapool_descriptor_array =
	std::same_as<std::remove_cvref_t<T>, std::array<MetapoolDescriptor, MetapoolCount>>;

} // hpr
