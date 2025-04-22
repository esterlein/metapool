#pragma once


#include "alloc_header.hpp"
namespace hpr {


template <mem::IsAllocatorConfig Config>
void* Allocator<Config>::do_allocate(std::size_t size, std::size_t alignment_min)
{
	constexpr uint32_t alignment = compute_alignment(static_cast<uint32_t>(alignment_min));
	constexpr uint32_t stride    = compute_stride(static_cast<uint32_t>(size), alignment);

	if constexpr (std::is_constant_evaluated()) {
		static_assert(stride >= Config::min_stride || stride <= Config::max_stride,
			"stride is out of bounds in allocate");
	}
	else {
		assert((stride >= Config::min_stride || stride <= Config::max_stride) &&
			"stride is out of bounds" && __func__);
	}

	auto lookup_entry = lookup(stride);

	auto& proxy = m_proxies[lookup_entry.mpool_index];
	std::byte* block = proxy.fetch(lookup_entry.flist_index);

	assert(block != nullptr &&
		"block is nullptr"  && __func__);

	auto* header = reinterpret_cast<mem::AllocHeader*>(block);
	*header = mem::AllocHeader::make(lookup_entry.mpool_index, lookup_entry.flist_index);

	std::byte* object_location = block + sizeof(mem::AllocHeader);

	return static_cast<void*>(object_location);
}


template <mem::IsAllocatorConfig Config>
void Allocator<Config>::do_deallocate(void* location, std::size_t bytes, std::size_t alignment)
{
	if constexpr (std::is_constant_evaluated()) {
		static_assert(location != nullptr,
			"location is nullptr in allocate");
	}
	else {
		assert(location != nullptr &&
			"location is nullptr"  && __func__);
	}

	std::byte* block = reinterpret_cast<std::byte*>(location) - sizeof(mem::AllocHeader);
	
	auto* header = reinterpret_cast<mem::AllocHeader*>(block);
	
	auto& proxy = m_proxies[header->mpool_index()];
	proxy.release(header->flist_index(), block);
}
} // hpr
