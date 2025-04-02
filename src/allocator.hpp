#pragma once

#include <cstddef>
#include <stdexcept>
#include <memory_resource>

#include "metapool.hpp"
#include "metapool_interface.hpp"


namespace hpr {


template <std::size_t MetapoolCount>
class Allocator final : public std::pmr::memory_resource
{
public:

	template <typename Array>
	requires metapool_descriptor_array<Array, MetapoolCount>
	constexpr Allocator(Array&& descriptors)
		: m_descriptors(std::forward<Array>(descriptors))
	{}

	~Allocator() = default;

	Allocator(const Allocator&) = default;
	Allocator(Allocator&&) = default;
	Allocator& operator=(const Allocator&) = default;
	Allocator& operator=(Allocator&&) = default;

private:


protected:

	void* do_allocate(std::size_t bytes, std::size_t alignment) override;

	void do_deallocate(void* address, std::size_t bytes, std::size_t alignment) override;

	inline bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
	}

private:

	template <typename T>
	static inline constexpr std::size_t get_type_stride()
	{
		constexpr std::size_t alignment = ((alignof(T) + 7UL) & ~7UL);
		return (sizeof(T) + MetapoolBase::alloc_header_size + alignment - 1) & ~(alignment - 1);
	}

	static inline constexpr std::size_t get_pmr_stride(std::size_t bytes, std::size_t alignment)
	{
		alignment = (alignment + 7UL) & ~7UL;
		return (bytes + alignment - 1) & ~(alignment - 1);
	}

private:

	std::array<MetapoolDescriptor, MetapoolCount> m_descriptors;

};


template <std::size_t MetapoolCount>
Allocator(std::array<MetapoolDescriptor, MetapoolCount>&) -> Allocator<MetapoolCount>;


template <std::size_t MetapoolCount>
Allocator(std::array<MetapoolDescriptor, MetapoolCount>&&) -> Allocator<MetapoolCount>;

} // hpr

#include "allocator.tpp"
