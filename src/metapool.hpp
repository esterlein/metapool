#pragma once

#include <cstddef>

#include <array>
#include <cstdint>
#include <utility>
#include <variant>
#include <optional>
#include <concepts>
#include <stdexcept>
#include <algorithm>
#include <memory_resource>

#include "math.hpp"
#include "freelist.hpp"


namespace hpr {
namespace mem {

	// create structs for metapool template instantiation
	// and change consts and concept

	static inline constexpr uint32_t min_base_block_count = 64;
	static inline constexpr uint32_t min_last_block_count = 32;
	static inline constexpr uint32_t block_count_mult = 16;
	static inline constexpr uint32_t min_stride_mult = 16;

	template <auto BasePoolBlockCount, auto... StridePivots>
	concept valid_metapool_sequence =
		BasePoolBlockCount >= min_base_block_count &&
		BasePoolBlockCount % block_count_mult == 0 &&
		sizeof...(StridePivots) >= 1 &&
		((StridePivots % min_stride_mult == 0) && ...) &&
		((sizeof...(StridePivots) > 1)
			? (BasePoolBlockCount * math::int_pow<int32_t>(2, static_cast<int32_t>(-(sizeof...(StridePivots) - 2))) >= min_last_block_count)
			: true
		) &&
		[]() constexpr {
			constexpr auto arr = std::array{ StridePivots... };
			if (arr[0] <= 0)
				return false;
			for (std::size_t i = 1; i < arr.size(); ++i) {
				if (arr[i] <= 0 || arr[i] <= arr[i - 1])
					return false;
			}
			return true;
		}();
} // hpr::mem


class MetapoolBase
{
protected:

	struct AllocHeader
	{
		uint32_t pool_index;
		uint16_t magic = 0xABCD;
	};

public:

	static inline constexpr std::size_t alloc_header_size = sizeof(AllocHeader);

	MetapoolBase() = delete;
	virtual ~MetapoolBase() = default;
};


template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence <BasePoolBlockCount, StridePivots...>
class Metapool final : public MetapoolBase
{
public:

	explicit Metapool(std::pmr::memory_resource* upstream);

	~Metapool();

	Metapool(const Metapool& other) = delete;
	Metapool& operator=(const Metapool& other) = delete;

	Metapool(Metapool&& other) noexcept;
	Metapool& operator=(Metapool&& other) noexcept;

private:

	static constexpr auto& compute_number_of_pools()
	{
		constexpr std::array<uint32_t, sizeof...(StridePivots)> pivots = {StridePivots...};
		static constexpr auto num_pools = (pivots[pivots.size() - 1] - pivots[0]) / 8U;
		return num_pools;
	}

	static constexpr auto& compute_pool_strides()
	{
		constexpr std::array<uint32_t, sizeof...(StridePivots)> pivots = {StridePivots...};
		constexpr uint32_t num_pools = compute_number_of_pools();

		static constexpr auto strides = [&pivots]<uint32_t... I>(std::index_sequence<I...>) {
			return std::array<uint32_t, num_pools>{ (static_cast<uint32_t>(pivots[0] + I * 8U))... };
		}(std::make_index_sequence<num_pools>{});

		return strides;
	}

	static constexpr auto& compute_block_count()
	{
		constexpr std::array<uint32_t, sizeof...(StridePivots)> pivots = {StridePivots...};
		constexpr uint32_t num_pools = compute_number_of_pools();
		constexpr auto pool_strides = compute_pool_strides();
	
		static constexpr std::array<uint32_t, num_pools> block_counts = [&pivots, &pool_strides, &num_pools]() constexpr {
			std::array<uint32_t, num_pools> counts{};
			uint32_t curr_count = BasePoolBlockCount;
	
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

static constexpr auto& compute_pools()
{
	constexpr uint32_t num_pools = compute_number_of_pools();
	constexpr auto& pool_strides = compute_pool_strides();
	constexpr auto& block_count = compute_block_count();

	static constexpr std::array<Pool, num_pools> pools =
		[&pool_strides, &block_count]<uint32_t... I>(std::index_sequence<I...>)
		{
			return std::array<Pool, num_pools>{
				Pool{
					pool_strides[I],
					block_count[I],
					&freelist_typed_fetch<pool_strides[I], block_count[I]>,
					&freelist_typed_release<pool_strides[I], block_count[I]>,
					Freelist<pool_strides[I], block_count[I]>{}
				}...
			};
		}(std::make_index_sequence<num_pools>{});

	return pools;
}

private:

	template <auto& Strides, auto& BlockCount, typename IndexSequence>
	struct FreelistGenerator;

	template <auto& Strides, auto& BlockCount, uint32_t... I>
	struct FreelistGenerator<Strides, BlockCount, std::index_sequence<I...>>
	{
		using type = std::variant<Freelist<Strides[I], BlockCount[I]>...>;
	};

	using FreelistVariant =
		typename FreelistGenerator <
			compute_pool_strides(),
			compute_block_count(),
			std::make_index_sequence<compute_number_of_pools()>
		>::type;

	using FreelistFetch = std::byte* (*)(void* freelist);
	using FreelistRelease = void (*)(void* freelist, std::byte* block);

	template <std::size_t Stride, std::size_t BlockCount>
	[[nodiscard]] inline std::byte* freelist_typed_fetch(void* freelist_ptr)
	{
		assert(freelist_ptr != nullptr);
		auto& freelist = *static_cast<Freelist<Stride, BlockCount>*>(freelist_ptr);
		return freelist.fetch();
	}

	template <std::size_t Stride, std::size_t BlockCount>
	inline void freelist_typed_release(void* freelist_ptr, std::byte* block)
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

	using PoolVariant = FreelistVariant;

	std::byte* fetch(std::size_t stride);
	void release(std::byte* block);

	template <typename T, typename... Types>
	T* construct(std::size_t stride_ul, Types&&... args)
	{
		const uint32_t stride = static_cast<uint32_t>(stride_ul);

		if (stride < m_pools.front().stride || stride > m_pools.back().stride)
			throw std::bad_alloc{};

		const auto pool_index = (stride - m_pools.front().stride) / 8U;
		std::byte* block = m_pools[pool_index].fl_fetch(&m_pools[pool_index].freelist);
		if (!block)
			throw std::bad_alloc{};

		auto* header = reinterpret_cast<AllocHeader*>(block);
		header->pool_index = pool_index;
		header->magic = 0xABCD;

		std::byte* object_location = block + sizeof(AllocHeader);
		T* object = std::launder(new (object_location) T(std::forward<Types>(args)...));

		return object;
	}

	template <typename T>
	void destruct(T* object)
	{
		if (!object)
			return;

		object->~T();

		std::byte* object_location = reinterpret_cast<std::byte*>(object);
		std::byte* block = object_location - sizeof(AllocHeader);

		auto* header = reinterpret_cast<AllocHeader*>(block);

		if (header->magic != 0xABCD)
			throw std::runtime_error("memory corruption detected");

		const auto pool_index = header->pool_index;
		m_pools[pool_index].fl_release(&m_pools[pool_index].freelist, block);
	}

	inline constexpr auto get_bounds()
	{
		return std::pair{m_pools.front().stride, m_pools.back().stride};
	}

private:

	template <typename T>
	static inline constexpr uint32_t get_type_stride()
	{
		constexpr uint32_t alignment = ((alignof(T) + 7U) & ~7U);
		return (sizeof(T) + MetapoolBase::alloc_header_size + alignment - 1U) & ~(alignment - 1U);
	}

	void upstream_allocate(std::size_t size);
	std::optional<std::size_t> get_pool_index(std::size_t stride);

private:

	std::pmr::memory_resource* m_upstream = nullptr;

	std::array<Pool, compute_number_of_pools()> m_pools = compute_pools();
};

} // hpr

#include "metapool.tpp"
