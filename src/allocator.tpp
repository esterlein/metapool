#pragma once


namespace hpr {


template <mem::IsAllocatorConfig Config>
void* Allocator<Config>::do_allocate(std::size_t bytes, std::size_t alignment)
{
	auto alignment_mpool = (alignment + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
	auto stride = (bytes + Config::alignment_shift + alignment_mpool - 1U) & ~(alignment_mpool - 1U);

	const std::size_t table_index = stride / Config::min_stride_step - 1U;
	if (table_index >= m_strides.size() || !m_strides[table_index]) {
		throw std::bad_alloc{};
	}

	return m_strides[table_index]->fetch(stride);
}


template <mem::IsAllocatorConfig Config>
void Allocator<Config>::do_deallocate(void* location, std::size_t bytes, std::size_t alignment)
{
	if (!location) return;

	auto alignment_mpool = (alignment + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
	auto stride = (bytes + Config::alignment_shift + alignment_mpool - 1U) & ~(alignment_mpool - 1U);

	const std::size_t table_index = stride / Config::min_stride_step - 1U;
	if (table_index >= m_stride_table.size() || !m_stride_table[table_index]) {
		throw std::runtime_error("no suitable metapool found for deallocation");
	}

	m_strides[table_index]->release(static_cast<std::byte*>(location));
}
} // hpr
