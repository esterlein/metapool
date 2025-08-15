#pragma once

#include <array>
#include <cstdint>
#include <algorithm>

#include "allocator_config.hpp"

#include "math.hpp"
#include "fail.hpp"


namespace mtp::cfg {


	struct RangeMetadata
	{
		uint32_t stride_min       {0};
		uint32_t stride_max       {0};
		uint32_t stride_step      {0};
		uint32_t stride_count     {0};
		uint32_t stride_shift     {0};
		uint16_t base_proxy_index {0xFFFF};
	};

	static constexpr std::size_t max_arena_size  = 4ULL << 30; // 2 GB
	static constexpr std::size_t arena_alignment = 4096U;

} // mtp::cfg


namespace mtp::core {


	template <typename... Metapools>
	class Metaset final
	{
	public:

		using TupleType  = std::tuple<Metapools...>;

		static constexpr std::size_t set_size = sizeof...(Metapools);

	private:

		template <std::size_t... Is>
		static constexpr auto sort_indices(std::index_sequence<Is...>)
		{
			std::array<std::size_t, set_size> indices{Is...};
			std::array<uint32_t, set_size> stride_min_array {
				std::tuple_element_t<Is, TupleType>::MetapoolTraits::stride_min...
			};

			for (std::size_t i = 0; i < set_size; ++i)
				for (std::size_t j = i + 1; j < set_size; ++j)
					if (stride_min_array[indices[j]] < stride_min_array[indices[i]])
						std::swap(indices[i], indices[j]);

			return indices;
		}


		template <typename Tuple, std::size_t... Is>
		static consteval auto calculate_proxy_offsets(std::index_sequence<Is...>)
		{
			std::array<uint16_t, set_size> offsets{};

			auto calculate_offset = [&offsets]<std::size_t Index>() consteval {
				if constexpr (Index == 0) {
					offsets[Index] = 0;
				} else {
					constexpr std::size_t prev_index = sorted_indices[Index - 1];
					using PrevMpool = std::tuple_element_t<prev_index, Tuple>;
					offsets[Index] = offsets[Index - 1] + static_cast<uint16_t>(PrevMpool::MetapoolTraits::stride_count);
				}
			};

			(calculate_offset.template operator()<Is>(), ...);
			return offsets;
		}


		template <typename Tuple, std::size_t Index>
		static consteval auto build_range_entry(uint16_t proxy_base)
		{
			constexpr std::size_t tuple_index = sorted_indices[Index];
			using Mpool = std::tuple_element_t<tuple_index, Tuple>;

			return mtp::cfg::RangeMetadata{
				.stride_min       = Mpool::MetapoolTraits::stride_min,
				.stride_max       = Mpool::MetapoolTraits::stride_max,
				.stride_step      = Mpool::MetapoolTraits::stride_step,
				.stride_count     = Mpool::MetapoolTraits::stride_count,
				.stride_shift     = mtp::math::log2_exact(Mpool::MetapoolTraits::stride_step),
				.base_proxy_index = proxy_base
			};
		}


		template <typename Tuple>
		static consteval auto build_range_array()
		{
			constexpr auto proxy_offsets = calculate_proxy_offsets<Tuple>(std::make_index_sequence<set_size>{});

			return [&proxy_offsets]<std::size_t... Is>(std::index_sequence<Is...>) consteval {
				return std::array<mtp::cfg::RangeMetadata, set_size>{
					build_range_entry<Tuple, Is>(proxy_offsets[Is])...
				};
			}(std::make_index_sequence<set_size>{});
		}


		template <std::size_t Index>
		static constexpr uint32_t get_min_stride()
		{
			return std::tuple_element_t<Index, TupleType>::MetapoolTraits::stride_min;
		}

		template <std::size_t Index>
		static constexpr uint32_t get_max_stride()
		{
			return std::tuple_element_t<Index, TupleType>::MetapoolTraits::stride_max;
		}

		template <std::size_t Index>
		static constexpr uint32_t get_stride_step()
		{
			return std::tuple_element_t<Index, TupleType>::MetapoolTraits::stride_step;
		}


		template <std::size_t... Is>
		static constexpr bool validate_stride_ranges(std::index_sequence<Is...>)
		{
			constexpr auto stride_min  = std::array<uint32_t, set_size>{ get_min_stride<Is>()... };
			constexpr auto stride_end  = std::array<uint32_t, set_size>{ get_max_stride<Is>()... };
			constexpr auto stride_step = std::array<uint32_t, set_size>{ get_stride_step<Is>()... };

			for (std::size_t i = 0; i < set_size; ++i) {
				for (uint32_t s = stride_min[i]; s < stride_end[i]; s += stride_step[i]) {
					std::size_t cover = 0;
					for (std::size_t j = 0; j < set_size; ++j) {
						if (s >= stride_min[j] && s < stride_end[j] &&
							(s - stride_min[j]) % stride_step[j] == 0)
						{
							++cover;
						}
					}
					if (cover != 1) return false;
				}
			}
			return true;
		}

	public:

		static constexpr auto sorted_indices = sort_indices(std::make_index_sequence<set_size>{});

		static constexpr std::array<std::size_t, set_size> sorted_index_map = []{
			std::array<std::size_t, set_size> inverted_indices {};
			for (std::size_t sorted_pos = 0; sorted_pos < set_size; ++sorted_pos)
				inverted_indices[sorted_indices[sorted_pos]] = sorted_pos;
			return inverted_indices;
		}();

		static constexpr auto range_metadata_array = build_range_array<TupleType>();

		static constexpr std::size_t arena_size =
			([]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
				return (0 + ... + std::tuple_element_t<Is, TupleType>::MetapoolTraits::reserved_bytes);
			})(std::make_index_sequence<set_size>{});

		static_assert(arena_size <= mtp::cfg::max_arena_size,
			SET_ARENA_TOO_LARGE_MSG);

		using AllocatorConfigType = mtp::cfg::AllocatorConfig<range_metadata_array>;


		static constexpr auto create_allocator_config()
		{
			return mtp::cfg::AllocatorConfig<range_metadata_array>();
		}

		static_assert(
			[]() constexpr {
				if constexpr (set_size <= 1) return true;
				return Metaset::validate_stride_ranges(std::make_index_sequence<set_size>{});
			}(),
			SET_INVALID_SEQUENCE_MSG);
	};

} // mtp::core


namespace mtp {


	using capf = cfg::CapacityFunction;

	template <capf Fn, auto Base, auto Step, auto... Pivots>
	using def = core::Metapool<cfg::MetapoolConfig<Fn, Base, Step, Pivots...>>;

	template <typename... Metapools>
	using metaset = core::Metaset<Metapools...>;

} // mtp

