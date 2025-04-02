#pragma once

#include <array>
#include <tuple>
#include <variant>

#include "metapool.hpp"


namespace hpr {


template <typename... Metapools>
struct MetapoolList
{
	using tuple_type = std::tuple<Metapools...>;
	using ptr_variant = std::variant<Metapools*...>;
	static inline constexpr std::size_t count = sizeof...(Metapools);
};


template <typename T>
struct MetapoolCounter;

template <typename... Metapools>
struct MetapoolCounter<MetapoolList<Metapools...>>
{
	static constexpr std::size_t value = sizeof...(Metapools);
};


using DefaultMetapoolList =
	MetapoolList<
		Metapool<1024, 8, 32, 64, 128, 256, 264>
	>;


struct MetapoolDescriptor
{
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


namespace std {

	template <>
	struct tuple_size<hpr::MetapoolDescriptor> : std::integral_constant<std::size_t, 3>
	{};

	template <std::size_t I>
	struct tuple_element<I, hpr::MetapoolDescriptor>
	{
		static_assert(I < 3, "MetapoolDescriptor index out of bounds");

		using type = std::conditional_t<
			I == 0, std::size_t,
			std::conditional_t<
				I == 1, std::size_t,
				decltype(std::declval<hpr::MetapoolDescriptor>().metapool)
			>
		>;
	};

	template <std::size_t I>
	decltype(auto) get(hpr::MetapoolDescriptor& descriptor)
	{
		if constexpr (I == 0) return descriptor.lower_bound;
		if constexpr (I == 1) return descriptor.upper_bound;
		if constexpr (I == 2) return descriptor.metapool;
	}

	template <std::size_t I>
	decltype(auto) get(const hpr::MetapoolDescriptor& descriptor)
	{
		if constexpr (I == 0) return descriptor.lower_bound;
		if constexpr (I == 1) return descriptor.upper_bound;
		if constexpr (I == 2) return descriptor.metapool;
	}

	template <std::size_t I>
	decltype(auto) get(hpr::MetapoolDescriptor&& descriptor)
	{
		if constexpr (I == 0) return std::move(descriptor.lower_bound);
		if constexpr (I == 1) return std::move(descriptor.upper_bound);
		if constexpr (I == 2) return std::move(descriptor.metapool);
	}
}

