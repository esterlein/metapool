#pragma once

#include <cmath>
#include <array>
#include <cstdint>
#include <variant>


namespace hpr {


template <typename... Metapools>
struct MetapoolList
{
	using tuple_type = std::tuple<Metapools...>;
	using variant_ptr = std::variant<Metapools*...>;
	static inline constexpr std::size_t count = sizeof...(Metapools);

	static constexpr uint32_t min_stride = []() {
		return min_stride_impl(std::make_index_sequence<count>{});
	}();

	static constexpr uint32_t max_stride = []() {
		return max_stride_impl(std::make_index_sequence<count>{});
	}();

	static constexpr bool is_valid = []() {
		if constexpr (count <= 1) {
			return true;
		}
		constexpr auto sorted_indices = sort_indices(std::make_index_sequence<count>{});
		return check_valid_impl(sorted_indices, std::make_index_sequence<count-1>{});
	}();
	
	static_assert(is_valid, "metapool registry has gaps, overlaps, or invalid stride steps");
	
private:

	template <std::size_t... Is>
	static constexpr uint32_t min_stride_impl(std::index_sequence<Is...>)
	{
		return std::min({std::tuple_element_t<Is, tuple_type>::config_type::stride_min...});
	}

	template <std::size_t... Is>
	static constexpr uint32_t max_stride_impl(std::index_sequence<Is...>)
	{
		return std::max({std::tuple_element_t<Is, tuple_type>::config_type::stride_max...});
	}

	template <std::size_t... Is>
	static constexpr auto sort_indices(std::index_sequence<Is...>)
	{
		std::array<std::size_t, count> indices = {Is...};

		constexpr std::array<uint32_t, count> min_strides = {
			std::tuple_element_t<Is, tuple_type>::config_type::stride_min...
		};

		for (std::size_t i = 0; i < count - 1; ++i) {
			for (std::size_t j = 0; j < count - i - 1; ++j) {
				if (min_strides[indices[j]] > min_strides[indices[j + 1]]) {
					std::size_t temp = indices[j];
					indices[j] = indices[j + 1];
					indices[j + 1] = temp;
				}
			}
		}

		return indices;
	}

	template <typename SortedIndices, std::size_t... Is>
	static constexpr bool check_valid_impl(const SortedIndices& indices, std::index_sequence<Is...>)
	{
		return (check_adjacent_metapools(
			indices[Is], indices[Is + 1]
		) && ...);
	}

	static constexpr bool check_adjacent_metapools(std::size_t prev_index, std::size_t next_index)
	{
		using PrevMetapool = std::tuple_element_t<prev_index, tuple_type>;
		using NextMetapool = std::tuple_element_t<next_index, tuple_type>;
		using PrevConfig = typename PrevMetapool::config_type;
		using NextConfig = typename NextMetapool::config_type;
	
		constexpr uint32_t prev_max_stride = PrevConfig::stride_max;
		constexpr uint32_t prev_stride_step = PrevConfig::stride_step;
		constexpr uint32_t next_min_stride = NextConfig::stride_min;
	
		if (prev_max_stride >= next_min_stride) {
			return false;
		}
	
		if (prev_max_stride + prev_stride_step != next_min_stride) {
			return false;
		}
	
		return true;
	}
};
} // hpr
