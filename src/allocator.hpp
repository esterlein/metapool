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

		uint32_t alignment_quantum;
		uint32_t alignment_shift;
		uint32_t min_stride_step;
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
		: m_descriptors{std::move(descriptors)}
	{
		if (!validate_descriptor_array(m_descriptors)) {
			// error
		}
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
				if ((desc.low & mask) != 0 || (desc.high & mask) != 0) {
					return false;
				}
			}
			else {
				if (desc.low % desc.stride_step != 0 || desc.high % desc.stride_step != 0) {
					return false;
				}
			}
			if (i < arr_size - 1) {
				const auto& next = std::get<i + 1>(descriptors);
				if (desc.high + desc.stride_step != next.low) {
					return false;
				}
			}
		}
		return true;
	}

	static constexpr auto compute_lookup_table_size(const DescriptorArray& descriptors)
	{
		constexpr std::size_t arr_size = std::tuple_size_v<DescriptorArray>;
		uint32_t total_stride_count = 0;

		for (std::size_t i = 0; i < arr_size; ++i) {
			const auto& desc = std::get<i>(descriptors);
			total_stride_count += (desc.range.high - desc.range.low) / Config::min_stride_step;
		}

		return total_stride_count;
	}

	static constexpr auto fill_lookup_table(const DescriptorArray& descriptors)
	{
		constexpr std::size_t table_size = compute_lookup_table_size(descriptors);

		return [table_size, &descriptors = std::as_const(descriptors)]() {
			std::array<uint32_t, table_size> table{};
			uint32_t table_index = 0;

			for (size_t i = 0; i < std::tuple_size_v<DescriptorArray>; ++i) {
				const auto& desc = std::get<i>(descriptors);
				const uint32_t stride_count = (desc.range.high - desc.range.low) / Config::min_stride_step;

				for (uint32_t j = 0; j < stride_count; ++j) {
					table[table_index++] = static_cast<uint32_t>(i);
				}
			}
			return table;
		}();
	}

	template <typename T>
	static inline constexpr std::size_t get_type_stride()
	{
		constexpr std::size_t alignment = ((alignof(T) + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U));
		return (sizeof(T) + Config::alignment_shift + alignment - 1U) & ~(alignment - 1U);
	}

	static inline constexpr std::size_t get_pmr_stride(std::size_t bytes, std::size_t alignment)
	{
		alignment = (alignment + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
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
