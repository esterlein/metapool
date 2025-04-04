#pragma once

#include <array>
#include <tuple>
#include <variant>

#include "metapool.hpp"


namespace hpr {

class MetapoolDescriptor
{
public:

	std::size_t lower_bound;
	std::size_t upper_bound;

	using FetchFunc = std::byte* (*)(void*, std::size_t);
	using ReleaseFunc = void (*)(void*, std::byte*);
	using ConstructFunc = void* (*)(void*, std::size_t, void*);
	using DestructFunc = void (*)(void*, void*);

	template <typename MetapoolType>
	static MetapoolDescriptor make_descriptor(MetapoolType* pool)
	{
		return {
			pool->lower_bound,
			pool->upper_bound,
			pool,
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

	MetapoolDescriptor(std::size_t lb, std::size_t ub, void* pool, FetchFunc f, ReleaseFunc r, ConstructFunc c, DestructFunc d)
		: lower_bound(lb), upper_bound(ub), m_metapool_ptr(pool), m_fetch(f), m_release(r), m_construct(c), m_destruct(d) {}
};


template <typename T, std::size_t MetapoolCount>
concept metapool_descriptor_array =
	std::same_as<std::remove_cvref_t<T>, std::array<MetapoolDescriptor, MetapoolCount>>;

} // hpr
