#pragma once

#include <array>
#include <cstdint>

#include "metapool_config.hpp"


namespace hpr {


class MetapoolDescriptor final
{
public:

	MetapoolDescriptor() = delete;

	struct Range
	{
		uint32_t min_stride, max_stride;
	} range {};

	uint32_t stride_step {0};

	std::byte* fetch(std::size_t stride) const
	{
		return fn_fetch(m_metapool_ptr, stride);
	}

	void release(std::byte* location) const
	{
		fn_release(m_metapool_ptr, location);
	}

private:

	using MetapoolFetch = std::byte* (*)(void*, std::size_t);
	using MetapoolRelease = void (*)(void*, std::byte*);

	void* m_metapool_ptr       {nullptr};
	MetapoolFetch fn_fetch     {nullptr};
	MetapoolRelease fn_release {nullptr};

	template <mem::IsMetapoolConfig Config>
	friend class Metapool;

	MetapoolDescriptor(Range r, uint32_t ss, void* ptr, MetapoolFetch fetch, MetapoolRelease release)
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
