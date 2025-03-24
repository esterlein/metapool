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
	std::size_t lower_bound;
	std::size_t upper_bound;
	DefaultMetapoolList::ptr_variant metapool;
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

