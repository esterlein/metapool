#pragma once

#include "mtpint.hpp"

#include <array>
#include <concepts>


namespace mtp::core {

class MetapoolProxy;

} // mtp::core


namespace mtp::cfg {


struct MetapoolConstraints
{
	static constexpr uint32_t min_stride           = 8U;
	static constexpr uint32_t max_stride           = 1'073'741'824U; // 1 GiB
	static constexpr uint32_t min_stride_step      = 8U;
	static constexpr uint32_t max_stride_step      = 536'870'912U; // 512 MiB
	static constexpr uint32_t min_base_block_count = 1U;
	static constexpr uint32_t min_last_block_count = 1U;
	static constexpr uint32_t freelist_alignment   = 4096U;
};

enum class CapacityFunction
{
	div8,
	div4,
	div2,
	flat,
	mul2,
	mul4,
	mul8
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
	StrideStep     >= MetapoolConstraints::min_stride_step &&
	StrideStep     <= MetapoolConstraints::max_stride_step &&
	sizeof...(StridePivots) >= 2 &&
	((StridePivots % MetapoolConstraints::min_stride_step == 0) && ...) &&
	[]() constexpr {
		constexpr auto pivots = std::array{StridePivots...};

		for (size_t i = 0; i < pivots.size(); ++i) {
			if (static_cast<uint32_t>(pivots[i]) < MetapoolConstraints::min_stride)
				return false;

			if (static_cast<uint32_t>(pivots[i]) > MetapoolConstraints::max_stride)
				return false;

			if (pivots[i] % StrideStep != 0)
				return false;
		}

		bool is_single_stride = pivots.size() == 2 && pivots.front() == pivots.back();

		if (!is_single_stride) {
			for (size_t i = 1; i < pivots.size(); ++i) {
				if (pivots[i] <= pivots[i - 1])
					return false;
			}
		}

		if constexpr (Func == CapacityFunction::flat) {
			if ((pivots.back() - pivots.front()) % StrideStep != 0)
				return false;
		}

		uint32_t count = BaseBlockCount;

		for (size_t i = 1; i < pivots.size(); ++i) {
			switch (Func) {
				case CapacityFunction::div8:
					count = std::max(count / 8U, MetapoolConstraints::min_last_block_count);
					break;
				case CapacityFunction::div4:
					count = std::max(count / 4U, MetapoolConstraints::min_last_block_count);
					break;
				case CapacityFunction::div2:
					count = std::max(count / 2U, MetapoolConstraints::min_last_block_count);
					break;
				case CapacityFunction::flat:
					break;
				case CapacityFunction::mul2:
					count *= 2U;
					break;
				case CapacityFunction::mul4:
					count *= 4U;
					break;
				case CapacityFunction::mul8:
					count *= 8U;
					break;
			}
		}

		return count >= MetapoolConstraints::min_last_block_count;
	}();


template <CapacityFunction Func, auto BaseBlockCount, auto StrideStep, auto... StridePivots>
requires ValidMetapoolConfig <Func, BaseBlockCount, StrideStep, StridePivots...>
struct MetapoolConfig
{
	using tag = metapool_config_tag;

	static constexpr uint32_t base_block_count = BaseBlockCount;
	static constexpr uint32_t stride_step      = StrideStep;

	static constexpr std::array<uint32_t, sizeof...(StridePivots)> stride_pivots = {StridePivots...};

	static constexpr uint32_t stride_min = []{
		constexpr auto& arr = stride_pivots;
		return arr.front();
	}();

	static constexpr uint32_t stride_max = []{
		constexpr auto& arr = stride_pivots;
		return arr.back();
	}();

	static constexpr uint32_t stride_count = []{
		constexpr auto& arr = stride_pivots;
		return (arr.back() - arr.front()) / StrideStep + 1;
	}();

	static constexpr CapacityFunction capacity_function = Func;
};

} // mtp::cfg


namespace mtp::core {
	
template <mtp::cfg::IsMetapoolConfig Config>
class Metapool;

} // mtp::core
