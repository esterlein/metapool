#pragma once

#include <array>
#include <cstdint>
#include <tuple>
#include <variant>

#include "metapool.hpp"
#include "ranges"


namespace hpr {


class MetapoolDescriptor
{
public:
	struct Range { uint32_t low, high; } range;
	uint32_t stride_mult;

	using FetchFunc   = std::byte* (*)(void*, std::size_t);
	using ReleaseFunc = void (*)(void*, std::byte*);

	template <typename MetapoolType>
	inline static MetapoolDescriptor make_descriptor(
		MetapoolType* mpool,
		uint32_t low_stride,
		uint32_t high_stride,
		uint32_t stride_mult)
	{
		return {
			mpool->m_pools.front().stride,
			mpool->m_pools.back().stride,
			mpool->stride_mult,
			reinterpret_cast<void*>(mpool),
			[](void* p, std::size_t s) -> std::byte* {
				return static_cast<MetapoolType*>(p)->fetch(s);
			},
			[](void* p, std::byte* b) {
				static_cast<MetapoolType*>(p)->release(b);
			}
		};
	}

	std::byte* fetch(std::size_t stride) const   { return m_fetch(m_pool, stride); }
	void       release(std::byte* block) const   {        m_release(m_pool, block); }

	template <typename T, typename... Args>
	T* construct(std::size_t stride, Args&&... args) const
	{
		return static_cast<MetapoolType*>(m_pool)
			->template construct<T, Args...>(stride, std::forward<Args>(args)...);
	}

	template <typename T>
	void destruct(T* obj) const
	{
		static_cast<MetapoolType*>(m_pool)
			->template destruct<T>(obj);
	}

private:
	void*       m_pool;
	FetchFunc   m_fetch;
	ReleaseFunc m_release;

	MetapoolDescriptor(
		uint32_t low, uint32_t high, uint32_t sm,
		void* pool, FetchFunc f, ReleaseFunc r
	)
		: range{low, high}
		, stride_mult{sm}
		, m_pool{pool}
		, m_fetch{f}
		, m_release{r}
	{}
};

template <typename T, std::size_t MetapoolCount>
concept metapool_descriptor_array =
	std::same_as<std::remove_cvref_t<T>, std::array<MetapoolDescriptor, MetapoolCount>>;

} // hpr
