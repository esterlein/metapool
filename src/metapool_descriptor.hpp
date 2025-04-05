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

	uint32_t lower_bound;
	uint32_t upper_bound;

	uint32_t stride_mult;

	using FetchFunc = std::byte* (*)(void*, std::size_t);
	using ReleaseFunc = void (*)(void*, std::byte*);
	using ConstructFunc = void* (*)(void*, std::size_t, void*);
	using DestructFunc = void (*)(void*, void*);

	template <typename MetapoolType>
	static MetapoolDescriptor make_descriptor(MetapoolType* mpool)
	{
		return {
			mpool->lower_bound,
			mpool->upper_bound,
			mpool->stride_mult,
			mpool,
			[](void* p, std::size_t stride) -> std::byte* {
				return static_cast<MetapoolType*>(p)->fetch(stride);
			},
			[](void* p, std::byte* location) {
				static_cast<MetapoolType*>(p)->release(location);
			},
			[](void* p, std::size_t stride, void* obj) -> void* {
				return static_cast<MetapoolType*>(p)->construct<void>(stride, obj);
			},
			[](void* p, void* obj) {
				static_cast<MetapoolType*>(p)->destruct(obj);
			}
		};
	}

	std::byte* fetch(std::size_t stride) const { return m_fetch(m_metapool_ptr, stride); }
	void release(std::byte* location) const { m_release(m_metapool_ptr, location); }
	void* construct(std::size_t stride, void* obj) const { return m_construct(m_metapool_ptr, stride, obj); }
	void destruct(void* obj) const { m_destruct(m_metapool_ptr, obj); }

private:

	void* m_metapool_ptr = nullptr;

	FetchFunc m_fetch;
	ReleaseFunc m_release;
	ConstructFunc m_construct;
	DestructFunc m_destruct;

	MetapoolDescriptor(uint32_t lb, uint32_t ub, uint32_t sm, void* mpool, FetchFunc ff, ReleaseFunc rf, ConstructFunc cf, DestructFunc df)
		: lower_bound{lb}, upper_bound{ub}, stride_mult{sm}, m_metapool_ptr{mpool}, m_fetch{ff}, m_release{rf}, m_construct{cf}, m_destruct{df}
	{}
};


template <typename T, std::size_t MetapoolCount>
concept metapool_descriptor_array =
	std::same_as<std::remove_cvref_t<T>, std::array<MetapoolDescriptor, MetapoolCount>>;

} // hpr
