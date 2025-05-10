#pragma once

#include <cstdint>
#include "alloc_header.hpp"


namespace hpr {


template <mem::IsAllocatorConfig Config>
std::byte* AllocatorCore<Config>::alloc(uint32_t size, uint32_t alignment)
{
	assert((size >= Config::min_stride || size <= Config::max_stride) &&
		"alloc size is out of bounds" && __func__);

	mem::AllocLogger::log(size);

	auto lookup_entry = lookup(size, alignment);

	auto& proxy = m_proxies[lookup_entry.mpool_index];
	std::byte* block = proxy.fetch(lookup_entry.flist_index);

	assert(block != nullptr &&
		"block is nullptr"  && __func__);

	auto* header = reinterpret_cast<mem::AllocHeader*>(block);
	*header = mem::AllocHeader::make(lookup_entry.mpool_index, lookup_entry.flist_index);

	std::byte* object_location = block + sizeof(mem::AllocHeader);

	return object_location;
}


template <mem::IsAllocatorConfig Config>
void AllocatorCore<Config>::free(std::byte* location)
{
	assert(location != nullptr &&
		"location is nullptr"  && __func__);

	auto* block = location - sizeof(mem::AllocHeader);
	auto* header = reinterpret_cast<mem::AllocHeader*>(block);
	
	auto& proxy = m_proxies[header->mpool_index()];
	proxy.release(header->flist_index(), block);
}


template <mem::IsAllocatorConfig Config>
void AllocatorCore<Config>::reset() noexcept
{
	for (auto& proxy : m_proxies) {
		proxy.reset();
	}
}

} // hpr
