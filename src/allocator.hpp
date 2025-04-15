#pragma once

#include <cstddef>
#include <cstdint>

#include <memory_resource>

#include "metapool_proxy.hpp"
#include "allocator_config.hpp"


namespace hpr {


template <mem::IsAllocatorConfig Config>
class Allocator : public std::pmr::memory_resource
{
public:

	constexpr Allocator(typename Config::proxy_array_type proxies)
		: m_proxies {std::move(proxies)}
	{
		if (!validate_descriptor_array(m_proxies)) {
			throw std::runtime_error {"invalid descriptor array"};
		}

		m_strides = fill_lookup_table(m_proxies);
	}

	Allocator() = delete;
	virtual ~Allocator() = default;

	Allocator(const Allocator&) = delete;
	Allocator(Allocator&&) = delete;
	Allocator& operator=(const Allocator&) = delete;
	Allocator& operator=(Allocator&&) = delete;

	// overload allocate/deallocate templates

	// implement construct/destruct

	// transparent interface / replace method calls

	// uint32_t enum class error codes in hot paths

public:

	template <typename T, typename... Types>
	[[nodiscard]] T* construct(Types&&... args)
	{
		constexpr uint32_t alignment = (static_cast<uint32_t>(alignof(T)) + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
		constexpr uint32_t stride = (static_cast<uint32_t>(sizeof(T)) + Config::alignment_shift + alignment - 1U) & ~(alignment - 1U);

		if (stride < Config::min_stride || stride > Config::max_stride) { [[unlikely]]
			throw std::bad_alloc{};
		}
	
		const std::size_t table_index = (stride - Config::min_stride) / Config::min_stride_step;
		if (table_index >= m_strides.size()) { [[unlikely]]
			throw std::bad_alloc{};
		}
	
		const uint32_t descriptor_index = m_strides[table_index];
		if (descriptor_index >= Config::metapool_count) { [[unlikely]]
			throw std::bad_alloc{};
		}

		m_descriptors[descriptor_index].fetch(stride);



		if (stride < m_pools.front().stride || stride > m_pools.back().stride)
			throw std::bad_alloc{};

		const auto pool_index = (stride - m_pools.front().stride) / Config::stride_step;

		if (pool_index >= m_pools.size())
			throw std::bad_alloc{};

		std::byte* block = m_pools[pool_index].fl_fetch(&m_pools[pool_index].freelist);
		if (!block)
			throw std::bad_alloc{};

		auto* header = reinterpret_cast<AllocHeader*>(block);
		header->pool_index = pool_index;
		header->magic = 0xABCD;

		std::byte* object_location = block + sizeof(AllocHeader);
		T* object = std::launder(new (object_location) T(std::forward<Types>(args)...));

		return object;
	}

	template <typename T>
	void destruct(T* object)
	{
		if (!object)
			return;

		object->~T();

		std::byte* object_location = reinterpret_cast<std::byte*>(object);
		std::byte* block = object_location - sizeof(AllocHeader);

		auto* header = reinterpret_cast<AllocHeader*>(block);

		if (header->magic != 0xABCD)
			throw std::runtime_error("memory corruption detected");

		const auto pool_index = header->pool_index;
		if (pool_index >= m_pools.size())
			throw std::runtime_error("invalid pool index detected");

		m_pools[pool_index].fl_release(&m_pools[pool_index].freelist, block);
	}

protected:

	virtual void* do_allocate(std::size_t bytes, std::size_t alignment) override;

	virtual void do_deallocate(void* address, std::size_t bytes, std::size_t alignment) override;

	inline virtual bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
	}

private:

	struct LookupEntry
	{
		uint8_t metapool_index;
		uint8_t freelist_index;
	};

	static constexpr bool validate_proxy_array(const DescriptorArray& descriptors)
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

		return (max_stride - min_stride) / step_size;
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

	std::array<uint8_t, compute_lookup_table_size()> m_strides {};

	typename Config::proxy_array_type m_proxies {};
};


template <mem::IsAllocatorConfig Config>
Allocator(const Allocator<Config>&) -> Allocator<Config>;

} // hpr

#include "allocator.tpp"
