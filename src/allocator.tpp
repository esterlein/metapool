#pragma once


namespace hpr {

template <std::size_t MetapoolCount>
void* Allocator<MetapoolCount>::do_allocate(std::size_t bytes, std::size_t alignment)
{
	auto alignment_mpool = (alignment + 7UL) & ~7UL;
	auto stride = (bytes + MetapoolBase::alloc_header_size + alignment_mpool - 1) & ~(alignment_mpool - 1);

	const std::size_t table_index = stride / 8 - 1;
	if (table_index >= m_stride_table.size() || !m_stride_table[table_index]) {
		throw std::bad_alloc{};
	}

	return m_stride_table[table_index]->fetch_func(stride);
}


template <std::size_t MetapoolCount>
void Allocator<MetapoolCount>::do_deallocate(void* location, std::size_t bytes, std::size_t alignment)
{
	if (!location) return;

	auto alignment_mpool = (alignment + 7UL) & ~7UL;
	auto stride = (bytes + MetapoolBase::alloc_header_size + alignment_mpool - 1) & ~(alignment_mpool - 1);

	const std::size_t table_index = stride / 8;
	if (table_index >= m_stride_table.size() || !m_stride_table[table_index]) {
		throw std::runtime_error("no suitable metapool found for deallocation");
	}

	m_stride_table[table_index]->release_func(static_cast<std::byte*>(location));
}
} // hpr
