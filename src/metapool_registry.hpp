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

	using tuple_type = std::tuple<Metapools...>;
	using variant_ptr = std::variant<Metapools*...>;

	static inline constexpr std::size_t registry_size = sizeof...(Metapools);

	static constexpr uint32_t min_stride = []() {
		return extract_min_stride(std::make_index_sequence<registry_size>{});
	}();

	static constexpr uint32_t max_stride = []() {
		return extract_max_stride(std::make_index_sequence<registry_size>{});
	}();

	static constexpr auto create_allocator_config()
	{
		constexpr auto metadata = create_range_metadata();
		return AllocatorConfig<registry_size>(metadata);
	}

	static_assert(
		[]() constexpr {
			if constexpr (registry_size <= 1) {
				return true;
			}
			return validate_registry_sequence(std::make_index_sequence<registry_size>{});
		}(),
		"metapool registry has gaps, overlaps, or invalid stride steps"
	);

private:

	template <std::size_t... Is>
	static constexpr uint32_t extract_min_stride(std::index_sequence<Is...>)
	{
		return std::min({std::tuple_element_t<Is, tuple_type>::config_type::stride_min...});
	}

	template <std::size_t... Is>
	static constexpr uint32_t extract_max_stride(std::index_sequence<Is...>)
	{
		return std::max({std::tuple_element_t<Is, tuple_type>::config_type::stride_max...});
	}

	template <std::size_t... Is>
	static constexpr bool validate_registry_sequence(std::index_sequence<Is...>)
	{
		constexpr std::array<uint32_t, registry_size> min_strides = { get_min_stride<Is>()... };
		constexpr std::array<uint32_t, registry_size> max_strides = { get_max_stride<Is>()... };
		constexpr std::array<uint32_t, registry_size> step_sizes  = { get_stride_step<Is>()... };

		for (uint32_t stride = min_stride; stride <= max_stride; stride++) {
			int covered_count = 0;
			for (std::size_t i = 0; i < registry_size; i++) {
				if (stride >= min_strides[i] &&
					stride <= max_strides[i] &&
					(stride - min_strides[i]) % step_sizes[i] == 0) {
						covered_count++;
					}
			}
			if (covered_count != 1) {
				return false;
			}
		}
		return true;
	}

	static constexpr auto create_range_metadata()
	{
		return []<std::size_t... Is>(std::index_sequence<Is...>) {
			return std::array<mem::RangeMetadata, sizeof...(Is)> {
				{
					mem::RangeMetadata {
						std::tuple_element_t<Is, tuple_type>::traits::stride_min,
						std::tuple_element_t<Is, tuple_type>::traits::stride_max,
						std::tuple_element_t<Is, tuple_type>::traits::stride_step
					}...
				}
			};
		}(std::make_index_sequence<registry_size>{});
	}
};
} // hpr
