#pragma once

#include "mtpint.hpp"

#include <array>
#include <variant>
#include <algorithm>

#include "freelist.hpp"
#include "freelist_proxy.hpp"
#include "monotonic_arena.hpp"
#include "metapool_config.hpp"

#include "fail.hpp"


namespace mtp::core {


template <mtp::cfg::IsMetapoolConfig Config>
class Metapool final
{
public:

	~Metapool() = default;

	Metapool(const Metapool& other) = delete;
	Metapool& operator=(const Metapool& other) = delete;

	Metapool(Metapool&& other) noexcept = default;
	Metapool& operator=(Metapool&& other) noexcept = default;

	using proxy_index_t = typename mtp::core::Freelist <
		Config::stride_pivots.front(),
		Config::base_block_count
	>::proxy_index_t;

	explicit Metapool(MonotonicArena* upstream, proxy_index_t base_proxy_index)
		: m_upstream {upstream}
	{
		MTP_ASSERT(upstream != nullptr,
			mtp::err::metapool_upstream_null);
	
		for (proxy_index_t pool_index = 0; pool_index < static_cast<proxy_index_t>(m_pools.size()); ++pool_index) {
			auto& pool = m_pools[pool_index];
			size_t pool_size = pool.stride * pool.block_count;
	
			std::byte* pool_memory = m_upstream->fetch(
				pool_size,
				mtp::cfg::MetapoolConstraints::freelist_alignment,
				sizeof(proxy_index_t)
			);
	
			const proxy_index_t proxy_index = base_proxy_index + pool_index;
	
			std::visit(
				[pool_memory, proxy_index](auto& freelist) {
					freelist.initialize(pool_memory, proxy_index);
				},
				pool.freelist
			);
		}
	}

private:

	struct MetapoolStatic
	{
		static constexpr uint32_t stride_count =
			(Config::stride_pivots.back() - Config::stride_pivots.front()) / Config::stride_step + 1;

		static constexpr auto strides = [] <size_t... Is>(std::index_sequence<Is...>) {
			return std::array<uint32_t, stride_count> {
				(Config::stride_pivots.front() + static_cast<uint32_t>(Is) * Config::stride_step)...
			};
		}(std::make_index_sequence<stride_count>{});

		static constexpr auto block_counts = [] {
			std::array<uint32_t, stride_count> blocks_per_stride {};
			uint32_t curr_count = Config::base_block_count;

			for (size_t i = 0; i < stride_count; ++i) {
				blocks_per_stride[i] = curr_count;

				if (i + 1 < stride_count &&
					std::find(
						Config::stride_pivots.begin() + 1,
						Config::stride_pivots.end(),
						strides[i + 1])
							!= Config::stride_pivots.end()
				) {
					switch (Config::capacity_function) {
						case mtp::cfg::CapacityFunction::div8:
							curr_count = std::max(curr_count / 8U, mtp::cfg::MetapoolConstraints::min_last_block_count);
							break;
						case mtp::cfg::CapacityFunction::div4:
							curr_count = std::max(curr_count / 4U, mtp::cfg::MetapoolConstraints::min_last_block_count);
							break;
						case mtp::cfg::CapacityFunction::div2:
							curr_count = std::max(curr_count / 2U, mtp::cfg::MetapoolConstraints::min_last_block_count);
							break;
						case mtp::cfg::CapacityFunction::flat: break;
						case mtp::cfg::CapacityFunction::mul2: curr_count *= 2U; break;
						case mtp::cfg::CapacityFunction::mul4: curr_count *= 4U; break;
						case mtp::cfg::CapacityFunction::mul8: curr_count *= 8U; break;
					}
				}
			}
	
			return blocks_per_stride;
		}();
	};

public:

	struct MetapoolTraits
	{
		static constexpr uint32_t stride_min   = Config::stride_pivots.front();
		static constexpr uint32_t stride_max   = Config::stride_pivots.back();
		static constexpr uint32_t stride_step  = Config::stride_step;
		static constexpr uint32_t stride_count = MetapoolStatic::stride_count;

		static constexpr size_t reserved_bytes = []() constexpr {
			size_t sum = 0;

			for (size_t i = 0; i < stride_count; ++i) {
				sum += static_cast<size_t>(MetapoolStatic::strides[i]) *
					static_cast<size_t>(MetapoolStatic::block_counts[i]);
			}

			return sum + stride_count * mtp::cfg::MetapoolConstraints::freelist_alignment;
		}();
	};

	static constexpr auto& compute_pools()
	{
		static constexpr std::array<Pool, MetapoolTraits::stride_count> pools =
			[]<size_t... Is>(std::index_sequence<Is...>) {
				return std::array<Pool, MetapoolTraits::stride_count> {{

					Pool {
						MetapoolStatic::strides[Is],
						MetapoolStatic::block_counts[Is],
						&freelist_typed_fetch  <MetapoolStatic::strides[Is], MetapoolStatic::block_counts[Is]>,
						&freelist_typed_release<MetapoolStatic::strides[Is], MetapoolStatic::block_counts[Is]>,
						&freelist_typed_reset  <MetapoolStatic::strides[Is], MetapoolStatic::block_counts[Is]>,
						Freelist<MetapoolStatic::strides[Is], MetapoolStatic::block_counts[Is]> {}
					}...

				}};
			}(std::make_index_sequence<MetapoolTraits::stride_count> {});

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
			MetapoolStatic::strides,
			MetapoolStatic::block_counts,
			std::make_index_sequence<MetapoolTraits::stride_count>
		>::type;

	using FreelistFetch = std::byte* (*)(void* freelist);
	using FreelistRelease = void (*)(void* freelist, std::byte* block);
	using FreelistReset = void (*)(void* freelist);

	template <uint32_t Stride, uint32_t BlockCount>
	[[nodiscard]] static inline std::byte* freelist_typed_fetch(void* freelist_ptr)
	{
		MTP_ASSERT(freelist_ptr != nullptr,
			 mtp::err::metapool_freelist_null);

		auto& freelist = *static_cast<Freelist<Stride, BlockCount>*>(freelist_ptr);
		return freelist.fetch();
	}

	template <uint32_t Stride, uint32_t BlockCount>
	static inline void freelist_typed_release(void* freelist_ptr, std::byte* block)
	{
		MTP_ASSERT(freelist_ptr != nullptr,
			 mtp::err::metapool_freelist_null);

		auto& freelist = *static_cast<Freelist<Stride, BlockCount>*>(freelist_ptr);
		freelist.release(block);
	}

	template <uint32_t Stride, uint32_t BlockCount>
	static inline void freelist_typed_reset(void* freelist_ptr)
	{
		MTP_ASSERT(freelist_ptr != nullptr,
			 mtp::err::metapool_freelist_null);

		auto& freelist = *static_cast<Freelist<Stride, BlockCount>*>(freelist_ptr);
		return freelist.reset();
	}


	struct Pool
	{
		uint32_t stride      {0};
		uint32_t block_count {0};

		FreelistFetch   fl_fetch   {nullptr};
		FreelistRelease fl_release {nullptr};
		FreelistReset   fl_reset   {nullptr};

		FreelistVariant freelist;
	};

public:

	using config_type = Config;
	using PoolVariant = FreelistVariant;

	inline void make_freelist_proxies(mtp::core::FreelistProxy* fl_proxies_out)
	{
		for (uint32_t i = 0; i < MetapoolTraits::stride_count; ++i) {
			std::visit([&](auto& freelist) {
				new (fl_proxies_out + i) mtp::core::FreelistProxy {
					&freelist,
					m_pools[i].fl_fetch,
					m_pools[i].fl_release,
					m_pools[i].fl_reset
				};
			}, m_pools[i].freelist);
		}
	}

private:

	std::array<Pool, MetapoolTraits::stride_count> m_pools {compute_pools()};

	MonotonicArena* m_upstream {nullptr};
};

} // mtp::core

