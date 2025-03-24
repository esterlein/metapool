#pragma once

#include <variant>


namespace hpr {


template <std::size_t MetapoolCount>
void* Allocator<MetapoolCount>::do_allocate(std::size_t bytes, std::size_t alignment)
{
	auto alignment_mpool = (alignment + 7UL) & ~7UL;
	auto stride = (bytes + alignment_mpool - 1) & ~(alignment_mpool - 1);

	for (auto& descriptor : m_descriptors) {
		if (stride >= descriptor.lower_bound && stride <= descriptor.upper_bound) {

			return std::visit([stride](auto* pool) -> void* {
				return pool->fetch(stride);
			}, descriptor.metapool);
		}
	}

	return std::pmr::get_default_resource()->allocate(bytes, alignment);
}


template <std::size_t MetapoolCount>
void Allocator<MetapoolCount>::do_deallocate(void* location, std::size_t bytes, std::size_t alignment)
{
	auto alignment_mpool = (alignment + 7UL) & ~7UL;
	auto stride = (bytes + alignment_mpool - 1) & ~(alignment_mpool - 1);

	for (auto& descriptor : m_descriptors) {
		if (stride >= descriptor.lower_bound && stride <= descriptor.upper_bound) {

			std::visit([stride, location](auto* pool) {
				pool->release(stride, static_cast<std::byte*>(location));
			}, descriptor.metapool);
			return;
		}
	}

	std::pmr::get_default_resource()->deallocate(location, bytes, alignment);
}
} // hpr
