#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>
#include <optional>
#include <concepts>
#include <stdexcept>
#include <algorithm>

#include "freelist.hpp"
#include "monotonic_arena.hpp"
#include "metapool_proxy.hpp"


namespace hpr {


template <mem::IsMetapoolConfig Config>
class Metapool final
{
public:

	explicit Metapool(MonotonicArena* upstream);

	~Metapool();

	Metapool(const Metapool& other) = delete;
	Metapool& operator=(const Metapool& other) = delete;
	Metapool(Metapool&& other) = delete;
	Metapool& operator=(Metapool&& other) = delete;

	struct traits
	{
		static constexpr uint32_t stride_min  = StrideMin;
		static constexpr uint32_t stride_max  = StrideMax;
		static constexpr uint32_t stride_step = StrideStep;
	};

private:

	static inline constexpr uint32_t compute_number_of_pools()
	{
		constexpr auto& pivots = Config::stride_pivots;
		static constexpr uint32_t value = (pivots.back() - pivots.front()) / Config::stride_step;
		return value;
	}

	static inline constexpr const auto& compute_pool_strides()
	{
		constexpr auto& pivots = Config::stride_pivots;
		static constexpr uint32_t num_pools = compute_number_of_pools();

		static constexpr auto strides =
			[num_pools, &pivots]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
				return std::array<uint32_t, num_pools> {
					(pivots.front() + static_cast<uint32_t>(Is) * Config::stride_step)...
				};
			}(std::make_index_sequence<num_pools>{});

		return strides;
	}

	static inline constexpr const auto& compute_block_count()
	{
		constexpr auto& pivots = Config::stride_pivots;
		static constexpr uint32_t num_pools = compute_number_of_pools();
		static constexpr auto& pool_strides = compute_pool_strides();

		static constexpr std::array<uint32_t, num_pools> block_counts = []() {
			std::array<uint32_t, num_pools> counts{};
			uint32_t curr_count = Config::base_block_count;

			for (std::size_t i = 0; i < num_pools; ++i) {
				bool is_pivot = false;
				for (std::size_t j = 1; j < pivots.size() - 1; ++j) {
					if (pivots[j] == pool_strides[i]) {
						is_pivot = true;
						break;
					}
				}
				if (i > 0 && is_pivot) curr_count /= 2;
				counts[i] = curr_count;
			}
			return counts;
		}();

		return block_counts;
	}

	static inline constexpr auto& compute_pools()
	{
		constexpr uint32_t num_pools = compute_number_of_pools();
		constexpr auto& pool_strides = compute_pool_strides();
		constexpr auto& block_count = compute_block_count();

		static constexpr std::array<Pool, num_pools> pools =
			[&pool_strides, &block_count]<uint32_t... Is>(std::index_sequence<Is...>)
			{
				return std::array<Pool, num_pools> {
					Pool {
						pool_strides[Is],
						block_count[Is],
						&freelist_typed_fetch<pool_strides[Is], block_count[Is]>,
						&freelist_typed_release<pool_strides[Is], block_count[Is]>,
						Freelist<pool_strides[Is], block_count[Is]>{}
					}...
				};
			}(std::make_index_sequence<num_pools>{});

		return pools;
	}

private:

	template <const auto& Strides, const auto& BlockCount, typename IndexSequence>
	struct FreelistGenerator;

	template <const auto& Strides, const auto& BlockCount, uint32_t... Is>
	struct FreelistGenerator<Strides, BlockCount, std::index_sequence<Is...>>
	{
		using type = std::variant<Freelist<Strides[Is], BlockCount[Is]>...>;
	};

	using FreelistVariant =
		typename FreelistGenerator <
			compute_pool_strides(),
			compute_block_count(),
			std::make_index_sequence<compute_number_of_pools()>
		>::type;

	using FreelistFetch = std::byte* (*)(void* freelist);
	using FreelistRelease = void (*)(void* freelist, std::byte* block);

	template <uint32_t Stride, uint32_t BlockCount>
	[[nodiscard]] static inline std::byte* freelist_typed_fetch(void* freelist_ptr)
	{
		assert(freelist_ptr != nullptr);
		auto& freelist = *static_cast<Freelist<Stride, BlockCount>*>(freelist_ptr);
		return freelist.fetch();
	}

	template <uint32_t Stride, uint32_t BlockCount>
	static inline void freelist_typed_release(void* freelist_ptr, std::byte* block)
	{
		assert(freelist_ptr != nullptr);
		auto& freelist = *static_cast<Freelist<Stride, BlockCount>*>(freelist_ptr);
		freelist.release(block);
	}

	struct Pool
	{
		uint32_t stride;
		uint32_t block_count;

		FreelistFetch fl_fetch;
		FreelistRelease fl_release;

		FreelistVariant freelist;
	};

public:

	using config_type = Config;
	using PoolVariant = FreelistVariant;

	[[nodiscard]] std::byte* fetch(std::size_t stride);
	void release(std::byte* block);

	[[nodiscard]] static inline constexpr auto bounds()
	{
		return std::pair{Config::stride_min, Config::stride_max};
	}

	[[nodiscard]] inline MetapoolProxy proxy() noexcept
	{
		return create_proxy();
	}

private:

	template <typename T>
	static inline constexpr uint32_t get_type_stride()
	{
		constexpr uint32_t alignment = ((alignof(T) + (mem::alignment_quantum - 1U)) & ~(mem::alignment_quantum - 1U));
		return (sizeof(T) + Config::alloc_header_size + alignment - 1U) & ~(alignment - 1U);
	}

	MetapoolProxy create_proxy();

private:

	std::array<Pool, compute_number_of_pools()> m_pools {compute_pools()};

	MonotonicArena* m_upstream {nullptr};
};
} // hpr

#include "metapool.tpp"
