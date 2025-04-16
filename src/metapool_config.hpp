#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <concepts>

#include "math.hpp"



namespace hpr {


class MetapoolBase;
class MetapoolProxy;


namespace mem {


	struct MetapoolConstraints
	{
		static inline constexpr uint32_t min_base_block_count = 64U;
		static inline constexpr uint32_t min_last_block_count = 64U;
		static inline constexpr uint32_t min_stride = 8U;
		static inline constexpr uint32_t min_stride_step = 8U;
		static inline constexpr uint32_t max_stride_step = 524288U; // 512 KB
	};


	struct metapool_config_tag {};

	template <typename T>
	concept IsMetapoolConfig = requires {
		typename T::tag;
		std::same_as<typename T::tag, metapool_config_tag>;
	};


	template <auto BaseBlockCount, auto StrideStep, auto... StridePivots>
	concept ValidMetapoolConfig =
		BaseBlockCount          >= MetapoolConstraints::min_base_block_count  &&
		StrideStep              >= MetapoolConstraints::min_stride_step       &&
		StrideStep              <= MetapoolConstraints::max_stride_step       &&
		sizeof...(StridePivots) >= 2                                          &&
		((StridePivots          %  MetapoolConstraints::min_stride_step == 0) && ...) &&
		(
			(sizeof...(StridePivots) > 1)
				? (BaseBlockCount * math::int_pow<int32_t>(2, static_cast<int32_t>(-(sizeof...(StridePivots) - 2))) >= MetapoolConstraints::min_last_block_count)
				: true
		) &&
		[]() constexpr {
			constexpr auto arr = std::array{ StridePivots... };
			if (arr[0] < MetapoolConstraints::min_stride || arr[0] % StrideStep != 0)
				return false;
			for (std::size_t i = 1; i < arr.size(); ++i) {
				if (arr[i] <= 0 || arr[i] <= arr[i - 1])
					return false;
			}
			return true;
		}();


	template <auto BaseBlockCount, auto StrideStep, auto... StridePivots>
	requires ValidMetapoolConfig <BaseBlockCount, StrideStep, StridePivots...>
	struct MetapoolConfig
	{
		using tag = metapool_config_tag;

		static constexpr uint32_t base_block_count  = BaseBlockCount;
		static constexpr uint32_t stride_step       = StrideStep;

		static constexpr std::array<uint32_t, sizeof...(StridePivots)> stride_pivots = { StridePivots... };

		static constexpr uint32_t stride_min = []{
			constexpr auto& arr = stride_pivots;
			return arr.front();
		}();

		static constexpr uint32_t stride_max = []{
			constexpr auto& arr = stride_pivots;
			return arr.back() - StrideStep;
		}();

		static constexpr uint32_t stride_count = []{
			constexpr auto& arr = stride_pivots;
			return (arr.back() - arr.front()) / StrideStep;
		}();
	};

} // hpr::mem


template <mem::IsMetapoolConfig Config>
class Metapool;


} // hpr
