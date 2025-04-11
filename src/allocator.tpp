#pragma once


namespace hpr {


template <mem::IsAllocatorConfig Config>
void* Allocator<Config>::do_allocate(std::size_t bytes, std::size_t alignment)
{
	auto alignment_mpool = (alignment + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
	auto stride = (bytes + Config::alignment_shift + alignment_mpool - 1U) & ~(alignment_mpool - 1U);

	if (stride < Config::min_stride || stride > Config::max_stride) {
		throw std::bad_alloc{};
	}

	const std::size_t table_index = (stride - Config::min_stride) / Config::min_stride_step;
	if (table_index >= m_strides.size()) {
		throw std::bad_alloc{};
	}

	const uint32_t descriptor_index = m_strides[table_index];
	if (descriptor_index >= Config::metapool_count) {
		throw std::bad_alloc{};
	}

	return m_descriptors[descriptor_index].fetch(stride);
}


template <mem::IsAllocatorConfig Config>
void Allocator<Config>::do_deallocate(void* location, std::size_t bytes, std::size_t alignment)
{
	if (!location) return;

	auto alignment_mpool = (alignment + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
	auto stride = (bytes + Config::alignment_shift + alignment_mpool - 1U) & ~(alignment_mpool - 1U);

	if (stride < Config::min_stride || stride > Config::max_stride) {
		throw std::runtime_error("stride out of bounds for deallocation");
	}

	const std::size_t table_index = (stride - Config::min_stride) / Config::min_stride_step;
	if (table_index >= m_strides.size()) {
		throw std::runtime_error("table index out of bounds for deallocation");
	}

	const uint32_t descriptor_index = m_strides[table_index];
	if (descriptor_index >= Config::metapool_count) {
		throw std::runtime_error("descriptor index out of bounds for deallocation");
	}

	m_descriptors[descriptor_index].release(static_cast<std::byte*>(location));
}
} // hpr
