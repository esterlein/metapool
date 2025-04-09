#pragma once

#include <cstddef>
#include <cstdint>

#include <memory_resource>

#include "metapool_descriptor.hpp"


namespace hpr {
namespace mem {

	struct AllocatorConfig
	{
		uint32_t metapool_count;
		uint32_t alignment_quantum;
		uint32_t alignment_shift;
	};
} // hpr::mem


template <typename DescriptorArray, mem::AllocatorConfig Config>
requires mem::metapool_descriptor_array<
	DescriptorArray,
	std::tuple_size_v<std::remove_cvref_t<DescriptorArray>>
>
class Allocator : public std::pmr::memory_resource
{
public:

	constexpr Allocator(DescriptorArray descriptors)
		: m_descriptors{std::move(descriptors)}
	{}

	Allocator() = delete;
	virtual ~Allocator() = default;

	Allocator(const Allocator&) = default;
	Allocator(Allocator&&) = default;
	Allocator& operator=(const Allocator&) = default;
	Allocator& operator=(Allocator&&) = default;

protected:

	virtual void* do_allocate(std::size_t bytes, std::size_t alignment) override;

	virtual void do_deallocate(void* address, std::size_t bytes, std::size_t alignment) override;

	inline virtual bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
	}

private:

	static constexpr auto compute_lookup_table_size(const auto& descriptors)
	{
		uint32_t total_stride_count = 0;
		for (const auto& desc : descriptors)
			total_stride_count += (desc.range.high - desc.range.low) / desc.stride_step;

		return total_stride_count;
	}

	template <typename T>
	static inline constexpr std::size_t get_type_stride()
	{
		constexpr std::size_t alignment = ((alignof(T) + Config.alignment_quantum - 1U) & ~(Config.alignment_quantum - 1U));
		return (sizeof(T) + Config.alignment_shift + alignment - 1U) & ~(alignment - 1U);
	}

	static inline constexpr std::size_t get_pmr_stride(std::size_t bytes, std::size_t alignment)
	{
		alignment = (alignment + Config.alignment_quantum - 1U) & ~(Config.alignment_quantum - 1U);
		return (bytes + alignment - 1U) & ~(alignment - 1U);
	}

private:

	std::array<uint32_t, compute_lookup_table_size()> m_strides {};

	DescriptorArray m_descriptors {};
};


template <typename DescriptorArray, mem::AllocatorConfig Config>
Allocator(const Allocator<DescriptorArray, Config>&) -> Allocator<DescriptorArray, Config>;

} // hpr

#include "allocator.tpp"
