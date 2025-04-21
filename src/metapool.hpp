#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>
#include <concepts>
#include <stdexcept>
#include <algorithm>

#include "freelist.hpp"
#include "metapool_proxy.hpp"
#include "monotonic_arena.hpp"


namespace hpr {


template <mem::IsMetapoolConfig Config>
class Metapool final
{
public:

	explicit Metapool(MonotonicArena* upstream);

	~Metapool();

	Metapool(const Metapool& other) = delete;
	Metapool& operator=(const Metapool& other) = delete;

	Metapool(Metapool&& other) noexcept = default;
	Metapool& operator=(Metapool&& other) noexcept = default;

	struct MetapoolTraits
	{
		static constexpr uint32_t stride_min = []() constexpr {
			return Config::stride_pivots.front();
		}();
		static constexpr uint32_t stride_max = []() constexpr {
			return Config::stride_pivots.back() - Config::stride_step;
		}();
		static constexpr uint32_t stride_step = Config::stride_step;
		static constexpr uint32_t stride_count = (stride_max - stride_min) / stride_step;
	};

private:

	static inline constexpr uint32_t compute_number_of_pools()
	{
		return MetapoolTraits::stride_count;
	}

	static inline constexpr const auto& compute_pool_strides()
	{
		static constexpr uint32_t num_pools = compute_number_of_pools();
		
		static constexpr auto strides =
			[]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
				return std::array<uint32_t, num_pools> {
					(MetapoolTraits::stride_min + static_cast<uint32_t>(Is) * MetapoolTraits::stride_step)...
				};
			}(std::make_index_sequence<num_pools>{});

		return strides;
	}

	static inline constexpr const auto& compute_block_count()
	{
		static constexpr uint32_t num_pools = compute_number_of_pools();
		static constexpr auto& pool_strides = compute_pool_strides();

		static constexpr std::array<uint32_t, num_pools> block_counts = []() {
			std::array<uint32_t, num_pools> counts {};
			uint32_t curr_count = Config::base_block_count;

			for (std::size_t i = 0; i < num_pools; ++i) {
				if (i > 0 &&
					std::find(
						Config::stride_pivots.begin() + 1,
						Config::stride_pivots.end() - 1,
						pool_strides[i])
							!= Config::stride_pivots.end()) {
								curr_count /= 2;
							}
				counts[i] = curr_count;
			}
			return counts;
		}();

		return block_counts;
	}

	static inline constexpr auto& compute_pools()
	{
		constexpr std::size_t num_pools = compute_number_of_pools();
	
		static constexpr std::array<Pool, num_pools> pools =
			[]<std::size_t... Is>(std::index_sequence<Is...>) constexpr {
				return std::array<Pool, num_pools> {{

					Pool {

						compute_pool_strides()[Is],
						compute_block_count()[Is],

						&freelist_typed_fetch <
							compute_pool_strides()[Is], compute_block_count()[Is]
						>,
						&freelist_typed_release <
							compute_pool_strides()[Is], compute_block_count()[Is]
						>,

						Freelist <
							compute_pool_strides()[Is], compute_block_count()[Is]
						>{}
					}...
				}};
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

	MetapoolProxy create_proxy();

public:

	using config_type = Config;
	using PoolVariant = FreelistVariant;

	[[nodiscard]] std::byte* fetch(uint8_t pool_index);
	void release(uint8_t pool_index, std::byte* block);

	[[nodiscard]] static inline constexpr auto bounds()
	{
		return std::pair{Config::stride_min, Config::stride_max};
	}

	[[nodiscard]] inline MetapoolProxy proxy() noexcept
	{
		return create_proxy();
	}

private:

	std::array<Pool, compute_number_of_pools()> m_pools {compute_pools()};

	MonotonicArena* m_upstream {nullptr};
};
} // hpr

#include "metapool.tpp"
