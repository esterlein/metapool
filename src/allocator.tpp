#pragma once


namespace hpr {


template <mem::IsAllocatorConfig Config>
void* Allocator<Config>::do_allocate(std::size_t bytes_ul, std::size_t alignment_ul)
{
	const uint32_t bytes = static_cast<uint32_t>(bytes_ul);
	const uint32_t alignment = static_cast<uint32_t>(alignment_ul);

	auto alignment_mpool = (alignment + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
	auto stride = (bytes + Config::alignment_shift + alignment_mpool - 1U) & ~(alignment_mpool - 1U);

	if (stride < Config::min_stride || stride > Config::max_stride) [[unlikely]]
		throw std::bad_alloc{};

	auto lookup_entry = lookup(stride);

	auto& proxy = m_proxies[lookup_entry.mpool_index];
	std::byte* block = proxy.fetch(lookup_entry.flist_index);

	if (!block) [[unlikely]]
		throw std::bad_alloc{};

	auto* header = reinterpret_cast<mem::AllocHeader*>(block);
	*header = mem::AllocHeader::make(lookup_entry.mpool_index, lookup_entry.flist_index);

	std::byte* object_location = block + sizeof(mem::AllocHeader);

	return static_cast<void*>(object_location);
}


template <mem::IsAllocatorConfig Config>
void Allocator<Config>::do_deallocate(void* location, std::size_t bytes, std::size_t alignment)
{
	if (!location) [[unlikely]]
		return;

	std::byte* block = reinterpret_cast<std::byte*>(location) - sizeof(mem::AllocHeader);
	
	auto* header = reinterpret_cast<mem::AllocHeader*>(block);
	
	auto& proxy = m_proxies[header->mpool_index];
	proxy.release(header->flist_index, block);
}
} // hpr
