#pragma once

#include <cmath>
#include <array>
#include <cstdint>
#include <variant>
#include <algorithm>

#include "allocator_config.hpp"



namespace hpr {



template <typename... Metapools>
class MetapoolRegistry final
{
public:

	using TupleType  = std::tuple<Metapools...>;
	using VariantPtr = std::variant<Metapools*...>;

	static inline constexpr std::size_t registry_size = sizeof...(Metapools);

	static inline constexpr auto range_metadata_array =
		[]<std::size_t... Is>(
			std::index_sequence<Is...>) -> std::array<mem::RangeMetadata, sizeof...(Is)> {
				return {{
					mem::RangeMetadata {
						.stride_min   = std::tuple_element_t<Is, TupleType>::MetapoolTraits::stride_min,
						.stride_max   = std::tuple_element_t<Is, TupleType>::MetapoolTraits::stride_max,
						.stride_step  = std::tuple_element_t<Is, TupleType>::MetapoolTraits::stride_step,
						.stride_count = std::tuple_element_t<Is, TupleType>::MetapoolTraits::stride_count,
						.stride_shift = math::log2_exact(std::tuple_element_t<Is, TupleType>::MetapoolTraits::stride_step)
					}...
				}};
			}(std::make_index_sequence<registry_size>{});

private:

	template <std::size_t I>
	static constexpr uint32_t get_min_stride()
	{
		return std::tuple_element_t<I, TupleType>::MetapoolTraits::stride_min;
	}

	template <std::size_t I>
	static constexpr uint32_t get_max_stride()
	{
		return std::tuple_element_t<I, TupleType>::MetapoolTraits::stride_max;
	}

	template <std::size_t I>
	static constexpr uint32_t get_stride_step()
	{
		return std::tuple_element_t<I, TupleType>::MetapoolTraits::stride_step;
	}

	template <std::size_t I>
	static constexpr uint32_t get_stride_count()
	{
		return std::tuple_element_t<I, TupleType>::MetapoolTraits::stride_count;
	}


	template <std::size_t... Is>
	static constexpr uint32_t compute_model_min_stride(std::index_sequence<Is...>)
	{
		return std::min({ get_min_stride<Is>()... });
	}

	template <std::size_t... Is>
	static constexpr uint32_t compute_model_max_stride(std::index_sequence<Is...>)
	{
		return std::max({ get_max_stride<Is>()... });
	}


	template <std::size_t... Is>
	static constexpr bool validate_registry_sequence(std::index_sequence<Is...>)
	{
		constexpr auto min_stride = std::array<uint32_t, registry_size>{ get_min_stride<Is>()... };
		constexpr auto max_stride = std::array<uint32_t, registry_size>{ get_max_stride<Is>()... };
		constexpr auto steps      = std::array<uint32_t, registry_size>{ get_stride_step<Is>()... };

		constexpr uint32_t model_min = compute_model_min_stride(std::index_sequence<Is...>{});
		constexpr uint32_t model_max = compute_model_max_stride(std::index_sequence<Is...>{});

		for (std::size_t i = 0; i < registry_size; ++i) {
			for (uint32_t stride = min_stride[i]; stride <= max_stride[i]; stride += steps[i]) {
				auto coverage = 0;
				for (std::size_t j = 0; j < registry_size; ++j) {
					if (stride >= min_stride[j] && stride <= max_stride[j] && ((stride - min_stride[j]) % steps[j] == 0))
						++coverage;
				}
				if (coverage != 1)
					return false;
			}
		}
		return true;
	}

public:

	using AllocatorConfigType = mem::AllocatorConfig<range_metadata_array>;


	static constexpr auto create_allocator_config()
	{
		return mem::AllocatorConfig<range_metadata_array>();
	}

	static_assert(
		[]() constexpr {
			if constexpr (registry_size <= 1) {
				return true;
			}
			return MetapoolRegistry::validate_registry_sequence(std::make_index_sequence<registry_size>{});
		}(),
		"metapool registry has gaps, overlaps, or invalid stride steps"
	);
};
} // hpr
