#pragma once

#include <array>
#include <cstdint>

#include "metapool.hpp"


namespace hpr {


class MetapoolDescriptor final
{
public:

	MetapoolDescriptor() = delete;

	struct Range
	{
		uint32_t min_stride, max_stride;
	} range {};

	uint32_t stride_step {};

	std::byte* fetch(std::size_t stride) const
	{
		return fn_fetch(m_metapool_ptr, stride);
	}

	void release(std::byte* location) const
	{
		fn_release(m_metapool_ptr, location);
	}

	template <typename T, typename... Args>
	T* construct(std::size_t stride, Args&&... args) const
	{
		return static_cast<MetapoolBase*>(m_metapool_ptr)->template construct<T>(
			stride, std::forward<Args>(args)...
		);
	}

	template <typename T>
	void destruct(T* object) const
	{
		static_cast<MetapoolBase*>(m_metapool_ptr)->template destruct<T>(object);
	}

private:

	using FetchFunc = std::byte* (*)(void*, std::size_t);
	using ReleaseFunc = void (*)(void*, std::byte*);

	void* m_metapool_ptr   {nullptr};
	FetchFunc fn_fetch     {nullptr};
	ReleaseFunc fn_release {nullptr};

	template <mem::IsMetapoolConfig Config>
	friend class Metapool;

	MetapoolDescriptor(Range r, uint32_t ss, void* ptr, FetchFunc fetch, ReleaseFunc release)
		: range          {r}
		, stride_step    {ss}
		, m_metapool_ptr {ptr}
		, fn_fetch       {fetch}
		, fn_release     {release}
	{}
};


namespace mem {

	template <typename DescriptorArray, auto MetapoolCount>
	concept MetapoolDescriptorArray =
		std::same_as<std::remove_cvref_t<DescriptorArray>, std::array<MetapoolDescriptor, MetapoolCount>>;

} // hpr::mem
} // hpr
