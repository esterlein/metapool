#pragma once

#include <cstddef>
#include <cstdint>

#include <memory_resource>

#include "metapool_descriptor.hpp"


namespace hpr {
namespace mem {


	struct allocator_config_tag {};

	template <typename DescriptorArray>
	struct AllocatorConfig
	{
		using tag = allocator_config_tag;
		using descriptor_array_type = DescriptorArray;

		static constexpr auto metapool_count = std::tuple_size_v<DescriptorArray>;

		uint32_t alignment_quantum {};
		uint32_t alignment_shift {};
		uint32_t min_stride_step {};
		uint32_t min_stride {};
		uint32_t max_stride {};

	};

	template <typename T>
	concept IsAllocatorConfig = requires {

		typename T::tag;
		typename T::descriptor_array_type;

		std::same_as<typename T::tag, allocator_config_tag>;

		requires MetapoolDescriptorArray <
			typename T::descriptor_array_type,
			std::tuple_size_v<typename T::descriptor_array_type>
		>;
	};

} // hpr::mem


template <mem::IsAllocatorConfig Config>
class Allocator : public std::pmr::memory_resource
{
public:

	using DescriptorArray = typename Config::descriptor_array_type;

	constexpr Allocator(DescriptorArray descriptors)
		: m_descriptors {std::move(descriptors)}
	{
		if (!validate_descriptor_array(m_descriptors)) {
			throw std::runtime_error("invalid descriptor array");
		}

		m_strides = fill_lookup_table(m_descriptors);
	}

	Allocator() = delete;
	virtual ~Allocator() = default;

	Allocator(const Allocator&) = default;
	Allocator(Allocator&&) = default;
	Allocator& operator=(const Allocator&) = default;
	Allocator& operator=(Allocator&&) = default;

	// overload allocate/deallocate templates

protected:

	virtual void* do_allocate(std::size_t bytes, std::size_t alignment) override;

	virtual void do_deallocate(void* address, std::size_t bytes, std::size_t alignment) override;

	inline virtual bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
	}

private:

	static constexpr bool validate_descriptor_array(const DescriptorArray& descriptors)
	{
		constexpr size_t arr_size = std::tuple_size_v<DescriptorArray>;
		if constexpr (arr_size == 0) {
			return false;
		}

		for (size_t i = 0; i < arr_size; ++i) {

			const auto& desc = std::get<i>(descriptors);
			if (desc.stride_step == 0) {
				return false;
			}
			if ((desc.stride_step & (desc.stride_step - 1)) == 0) {
				const uint32_t mask = desc.stride_step - 1;
				if ((desc.min_stride & mask) != 0 || (desc.max_stride & mask) != 0) {
					return false;
				}
			}
			else {
				if (desc.min_stride % desc.stride_step != 0 || desc.max_stride % desc.stride_step != 0) {
					return false;
				}
			}
			if (i < arr_size - 1) {
				const auto& next = std::get<i + 1>(descriptors);
				if (desc.max_stride + desc.stride_step != next.min_stride) {
					return false;
				}
			}
		}
		return true;
	}

	static constexpr auto compute_lookup_table_size()
	{
		constexpr uint32_t min_stride = Config::min_stride;
		constexpr uint32_t max_stride = Config::max_stride;
		constexpr uint32_t step_size = Config::min_stride_step;

		return (max_stride - min_stride) / step_size + 1U;
	}

	static auto fill_lookup_table(const DescriptorArray& descriptors)
	{
		constexpr std::size_t table_size = compute_lookup_table_size();
		std::array<uint32_t, table_size> table{};
		
		uint32_t table_index = 0;
		for (size_t desc_index = 0; desc_index < std::tuple_size_v<DescriptorArray>; ++desc_index) {
			const auto& desc = std::get<desc_index>(descriptors);
			const uint32_t stride_count =
				(desc.range.max_stride - desc.range.min_stride) / Config::min_stride_step;
			
			std::fill_n(table.begin() + table_index, stride_count, static_cast<uint32_t>(desc_index));
			table_index += stride_count;
		}
		
		return table;
	}

	template <typename T>
	static inline constexpr uint32_t get_type_stride()
	{
		constexpr uint32_t  alignment = ((alignof(T) + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U));
		return (sizeof(T) + Config::alignment_shift + alignment - 1U) & ~(alignment - 1U);
	}

	static inline constexpr uint32_t get_pmr_stride(uint32_t bytes, uint32_t alignment)
	{
		alignment = (alignment + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
		return (bytes + alignment - 1U) & ~(alignment - 1U);
	}

private:

	std::array<uint32_t, compute_lookup_table_size()> m_strides {};

	DescriptorArray m_descriptors {};
};


template <mem::IsAllocatorConfig Config>
Allocator(const Allocator<Config>&) -> Allocator<Config>;

} // hpr

#include "allocator.tpp"
