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
			constexpr auto sorted_indices = sort_indices(std::make_index_sequence<registry_size>{});
			return validate_registry(sorted_indices, std::make_index_sequence<registry_size - 1>{});
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
	static constexpr auto sort_indices(std::index_sequence<Is...>)
	{
		std::array<std::size_t, registry_size> indices = {Is...};
		constexpr std::array<uint32_t, registry_size> min_strides = {
			std::tuple_element_t<Is, tuple_type>::config_type::stride_min...
		};

		std::sort(indices.begin(), indices.end(),
			[&min_strides](std::size_t left_index, std::size_t right_index) {
				return min_strides[left_index] < min_strides[right_index];
			});

		return indices;
	}

	template <typename SortedIndices, std::size_t... Is>
	static constexpr bool validate_registry(const SortedIndices& indices, std::index_sequence<Is...>)
	{
		return (validate_adjacent_metapools(
			indices[Is], indices[Is + 1]
		) && ...);
	}

	static constexpr bool validate_adjacent_metapools(std::size_t prev_index, std::size_t next_index)
	{
		using PrevMetapool = std::tuple_element_t<prev_index, tuple_type>;
		using NextMetapool = std::tuple_element_t<next_index, tuple_type>;

		using PrevConfig = typename PrevMetapool::config_type;
		using NextConfig = typename NextMetapool::config_type;
	
		constexpr uint32_t prev_max_stride  = PrevConfig::stride_max;
		constexpr uint32_t prev_stride_step = PrevConfig::stride_step;
		constexpr uint32_t next_min_stride  = NextConfig::stride_min;
	
		if (prev_max_stride >= next_min_stride) {
			return false;
		}
		if (prev_max_stride + prev_stride_step != next_min_stride) {
			return false;
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
