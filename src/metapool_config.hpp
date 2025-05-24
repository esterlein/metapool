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
		static constexpr uint32_t min_stride             = 8U;
		static constexpr uint32_t max_stride             = 8388608U; // 8 MB
		static constexpr uint32_t min_stride_step        = 8U;
		static constexpr uint32_t max_stride_step        = 524288U; // 512 KB
		static constexpr uint32_t min_base_block_count   = 8U;
		static constexpr uint32_t min_last_block_count   = 8U;
		static constexpr uint32_t min_freelist_alignment = 64U; // cacheline
	};


	enum class CapacityFunction
	{
		Div8,
		Div4,
		Div2,
		Flat,
		Mul2,
		Mul4,
		Mul8
	};


	struct metapool_config_tag {};

	template <typename T>
	concept IsMetapoolConfig = requires {
		typename T::tag;
		std::same_as<typename T::tag, metapool_config_tag>;
	};


	template <CapacityFunction Func, auto BaseBlockCount, auto StrideStep, auto... StridePivots>
	concept ValidMetapoolConfig =
		BaseBlockCount >= MetapoolConstraints::min_base_block_count &&
		Alignment      >= MetapoolConstraints::min_freelist_alignment &&
		StrideStep     >= MetapoolConstraints::min_stride_step &&
		StrideStep     <= MetapoolConstraints::max_stride_step &&
		sizeof...(StridePivots) >= 2 &&
		((StridePivots % MetapoolConstraints::min_stride_step == 0) && ...) &&
		[]() constexpr {
			constexpr auto pivots = std::array{StridePivots...};

			if (pivots[0] < MetapoolConstraints::min_stride || pivots[0] % StrideStep != 0)
				return false;
			for (std::size_t i = 1; i < pivots.size(); ++i) {
				if (pivots[i] <= pivots[i - 1])
					return false;
			}

			uint32_t count = BaseBlockCount;

			for (std::size_t i = 1; i < pivots.size() - 1; ++i) {
				switch (Func) {
					case CapacityFunction::Div8:
						count = std::max(count / 8U, mem::MetapoolConstraints::min_last_block_count);
						break;
					case CapacityFunction::Div4:
						count = std::max(count / 4U, mem::MetapoolConstraints::min_last_block_count);
						break;
					case CapacityFunction::Div2:
						count = std::max(count / 2U, mem::MetapoolConstraints::min_last_block_count);
						break;
					case CapacityFunction::Flat:
						break;
					case CapacityFunction::Mul2:
						count *= 2U;
						break;
					case CapacityFunction::Mul4:
						count *= 4U;
						break;
					case CapacityFunction::Mul8:
						count *= 8U;
						break;
				}
			}

			return count >= MetapoolConstraints::min_last_block_count;
		}();


	template <CapacityFunction Func, auto BaseBlockCount, auto Alignment, auto StrideStep, auto... StridePivots>
	requires ValidMetapoolConfig <Func, BaseBlockCount, Alignment, StrideStep, StridePivots...>
	struct MetapoolConfig
	{
		using tag = metapool_config_tag;

		static constexpr uint32_t base_block_count  = BaseBlockCount;
		static constexpr uint32_t alignment         = Alignment;
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

		static constexpr CapacityFunction capacity_function = Func;
	};

} // hpr::mem


template <mem::IsMetapoolConfig Config>
class Metapool;


} // hpr
