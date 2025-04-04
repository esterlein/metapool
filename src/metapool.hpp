#pragma once

#include <cstddef>

#include <array>
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
		constexpr std::array<std::size_t, sizeof...(StridePivots)> pivots = {StridePivots...};
		static constexpr auto num_pools = (pivots[pivots.size() - 1] - pivots[0]) / 8;
		return num_pools;
	}

	static constexpr auto& compute_pool_strides()
	{
		constexpr std::array<std::size_t, sizeof...(StridePivots)> pivots = {StridePivots...};
		constexpr std::size_t num_pools = compute_number_of_pools();

		static constexpr auto strides = [&pivots]<std::size_t... I>(std::index_sequence<I...>) {
			return std::array<std::size_t, num_pools>{ (static_cast<std::size_t>(pivots[0] + I * 8))... };
		}(std::make_index_sequence<num_pools>{});

		return strides;
	}

	static constexpr auto& compute_block_count()
	{
		constexpr std::array<std::size_t, sizeof...(StridePivots)> pivots = {StridePivots...};
		constexpr std::size_t num_pools = compute_number_of_pools();
		constexpr auto pool_strides = compute_pool_strides();
	
		static constexpr std::array<std::size_t, num_pools> block_counts = [&pivots, &pool_strides, &num_pools]() constexpr {
			std::array<std::size_t, num_pools> counts{};
			std::size_t curr_count = BasePoolBlockCount;
	
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
		constexpr std::size_t num_pools = compute_number_of_pools();
		constexpr auto& pool_strides = compute_pool_strides();
		constexpr auto& block_count = compute_block_count();

		static constexpr std::array<Pool, num_pools> pools =
			[&pool_strides, &block_count]<std::size_t... I>(std::index_sequence<I...>){
				return std::array<Pool, num_pools>{
					Pool{ pool_strides[I], block_count[I], Freelist<pool_strides[I], block_count[I]>{} }...
				};
			}(std::make_index_sequence<num_pools>{});

		return pools;
	}

	static constexpr auto generate_lookup_table()
	{
		constexpr std::size_t num_pools = compute_number_of_pools();
		constexpr auto& pool_strides = compute_pool_strides();
		constexpr auto& block_count = compute_block_count();

		return []<std::size_t... I>(std::index_sequence<I...>, const auto& strides, const auto& counts) {
			std::array<std::pair<std::size_t, std::pair<FreelistFetch, FreelistRelease>>, sizeof...(I)> table{};

			((table[I] = {
				I,
				{
					&freelist_typed_fetch<strides[I], counts[I]>,
					&freelist_typed_release<strides[I], counts[I]>
				}
			}), ...);

			return table;
		}(std::make_index_sequence<num_pools>{}, pool_strides, block_count);
	}

private:

	template <auto& Strides, auto& BlockCount, typename IndexSequence>
	struct FreelistGenerator;

	template <auto& Strides, auto& BlockCount, std::size_t... I>
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
	static std::byte* freelist_typed_fetch(void* freelist_ptr) {
		auto& freelist = *static_cast<Freelist<Stride, BlockCount>*>(freelist_ptr);
		return freelist.fetch();
	}

	template <std::size_t Stride, std::size_t BlockCount>
	static void freelist_typed_release(void* freelist_ptr, std::byte* block) {
		auto& freelist = *static_cast<Freelist<Stride, BlockCount>*>(freelist_ptr);
		freelist.release(block);
	}

	struct Pool
	{
		size_t stride;
		std::size_t block_count;
		FreelistVariant freelist;
	};

	struct FreelistEntry
	{
		size_t pool_index;
		FreelistFetch fetch;
		FreelistRelease release;
	};

public:

	using PoolVariant = FreelistVariant;

	static inline constexpr std::size_t alloc_header_size = sizeof(AllocHeader);

	std::byte* fetch(std::size_t stride);
	void release(std::byte* location);

	template <typename T, typename... Types>
	T* construct(std::size_t stride, Types&&... args)
	{
		if (stride < m_pools.front().stride || stride > m_pools.back().stride)
			throw std::bad_alloc{};

		const auto pool_index = (stride - m_pools.front().stride) / 8;
		const auto& [_, functions] = m_lookup_table[pool_index];
		const auto& [fetch_func, _] = functions;

		std::byte* block = fetch_func(&m_pools[pool_index].freelist);
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
		if (!object) return;

		object->~T();

		std::byte* object_location = reinterpret_cast<std::byte*>(object);
		std::byte* block = object_location - sizeof(AllocHeader);

		auto* header = reinterpret_cast<AllocHeader*>(block);

		if (header->magic != 0xABCD)
			throw std::runtime_error("memory corruption detected");

		const auto pool_index = header->pool_index;
		const auto& [_, functions] = m_lookup_table[pool_index];
		const auto& [_, release_func] = functions;

		release_func(&m_pools[pool_index].freelist, block);
	}

	inline constexpr auto get_bounds()
	{
		return std::pair{m_pools.front().stride, m_pools.back().stride};
	}

private:

	template <typename T>
	static inline constexpr std::size_t get_type_stride()
	{
		constexpr std::size_t alignment = ((alignof(T) + 7UL) & ~7UL);
		return (sizeof(T) + MetapoolBase::alloc_header_size + alignment - 1) & ~(alignment - 1);
	}

	void upstream_allocate(std::size_t size);
	std::optional<std::size_t> get_pool_index(std::size_t stride);

private:

	std::pmr::memory_resource* m_upstream = nullptr;

	std::array<Pool, compute_number_of_pools()> m_pools = compute_pools();
	const std::array<FreelistEntry, compute_number_of_pools()> m_lookup_table = generate_lookup_table();
};

} // hpr

#include "metapool.tpp"
