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

	static inline constexpr uint32_t min_base_block_count = 64;
	static inline constexpr uint32_t min_last_block_count = 32;
	static inline constexpr uint32_t block_count_multiple = 8;
	static inline constexpr uint32_t stride_multiple = 8;

	template <auto BasePoolBlockCount, auto... StridePivots>
	concept valid_metapool_sequence =
		BasePoolBlockCount >= min_base_block_count &&
		BasePoolBlockCount % block_count_multiple == 0 &&
		sizeof...(StridePivots) >= 1 &&
		((StridePivots % stride_multiple == 0) && ...) &&
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


template <auto BasePoolBlockCount, auto... StridePivots>
requires mem::valid_metapool_sequence <BasePoolBlockCount, StridePivots...>
class Metapool final
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

	struct Pool
	{
		std::size_t stride;
		std::size_t block_count;
		FreelistVariant freelist;
	};

public:

	using PoolVariant = FreelistVariant;

	std::byte* fetch(std::size_t stride);
	void release(std::size_t stride, std::byte* location);

	template <typename T, typename... Types>
	T* construct(Types&&... args)
	{
		std::size_t stride = get_type_stride<T>();
		// assert metapool bounds
		return construct_impl(stride, std::forward<Types>(args)...);
	}

	template <typename T>
	void destruct(T* object)
	{
		std::size_t stride = get_type_stride<T>();
		// assert metapool bounds
		destruct_impl(stride, object);
	}

	inline constexpr auto get_bounds()
	{
		return std::pair{m_pools.front().stride, m_pools.back().stride};
	}

protected:

	template <typename T, typename... Types>
	T* construct_impl(std::size_t stride, Types&&... args)
	{
		try {
			auto& freelist_variant = m_pools[(stride - m_pools.front().stride) / 8].freelist;

			return std::visit([&args...](auto&& freelist) -> T* {
				auto* location = freelist.fetch();
				if (!location) {
					throw std::bad_alloc{};
				}
				return std::launder(new (location) T(std::forward<Types>(args)...));

			}, freelist_variant);

			// throw std::runtime_error("no suitable pool found for object construction");
		}
		catch (const std::exception& e) {
			throw;
		}
	}

	template <typename T>
	void destruct_impl(std::size_t stride, T* object)
	{
		if (!object) return;

		auto& freelist_variant = m_pools[(stride - m_pools.front().stride) / 8].freelist;

		object->~T();

		std::visit([location = reinterpret_cast<std::byte*>(object)](auto&& freelist) {
			freelist.release(location);
		}, freelist_variant);

		return;

		// throw std::runtime_error("no suitable pool found for object destruction");
	}

private:

	template <typename T>
	static inline constexpr std::size_t get_type_stride()
	{
		constexpr std::size_t alignment = ((alignof(T) + 7UL) & ~7UL);
		return (sizeof(T) + alignment - 1) & ~(alignment - 1);
	}

	void upstream_allocate(std::size_t size);
	std::optional<std::size_t> get_pool_index(std::size_t stride);

private:

	std::pmr::memory_resource* m_upstream = nullptr;

	std::array<Pool, compute_number_of_pools()> m_pools;
};

} // hpr

#include "metapool.tpp"
