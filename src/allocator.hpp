#pragma once

#include <cstddef>
#include <cstdint>
#include <cassert>

#include <memory_resource>

#include "alloc_header.hpp"
#include "alloc_logger.hpp"
#include "metapool_proxy.hpp"
#include "allocator_config.hpp"


namespace hpr {


struct Native {};
struct StdAdapter {};
struct PmrAdapter {};


template <mem::IsAllocatorConfig Config>
class AllocatorCore
{
public:

	using ProxyArray = typename Config::ProxyArrayType;

	constexpr AllocatorCore(ProxyArray proxies)
		: m_proxies {std::move(proxies)}
	{}

	AllocatorCore() = delete;
	virtual ~AllocatorCore() = default;

	AllocatorCore(const AllocatorCore&) = default;

	AllocatorCore(AllocatorCore&&) = delete;
	AllocatorCore& operator=(const AllocatorCore&) = delete;
	AllocatorCore& operator=(AllocatorCore&&) = delete;

public:

	[[nodiscard]] std::byte* alloc(uint32_t size, uint32_t alignment);
	void free(std::byte* block);
	void reset() noexcept;

	template <typename T, typename... Types>
	[[nodiscard]] T* construct(Types&&... args)
	{
		if constexpr (std::is_constant_evaluated()) {
			static_assert(sizeof(T) >= Config::min_stride || sizeof(T) <= Config::max_stride,
				"sizeof(T) out of bounds in construct");
		}
		else {
			assert((sizeof(T) >= Config::min_stride || sizeof(T) <= Config::max_stride) &&
				"sizeof(T) out of bounds" && __func__);
		}

		mem::AllocLogger::log(sizeof(T));

		auto lookup_entry = lookup(sizeof(T), alignof(T));

		auto& proxy = m_proxies[lookup_entry.mpool_index];
		std::byte* block = proxy.fetch(lookup_entry.flist_index);

		assert(block != nullptr &&
			"block is nullptr"  && __func__);

		auto* header = reinterpret_cast<mem::AllocHeader*>(block);
		*header = mem::AllocHeader::make(lookup_entry.mpool_index, lookup_entry.flist_index);

		std::byte* object_location = block + sizeof(mem::AllocHeader);
		T* object = std::launder(new (object_location) T(std::forward<Types>(args)...));

		return object;
	}


	template <typename T>
	void destruct(T* object)
	{
		assert(object != nullptr &&
			"object is nullptr"  && __func__);

		std::byte* block = reinterpret_cast<std::byte*>(object) - sizeof(mem::AllocHeader);
		auto* header = reinterpret_cast<const mem::AllocHeader*>(block);
	
		auto& proxy = m_proxies[header->mpool_index()];

		object->~T();

		proxy.release(header->flist_index(), block);
	}

private:

	static_assert(Config::range_metadata.size() <= 256,
		"too many metapools for a 1 byte lookup index");


	struct LookupEntry
	{
		uint8_t mpool_index;
		uint8_t flist_index;
	};


	static constexpr std::size_t compute_number_of_entries()
	{
		std::size_t total = 0;
		const auto& range_meta = Config::range_metadata;
		for (std::size_t i = 0; i < range_meta.size(); ++i) {

			static_assert(Config::range_metadata.size() <= 256,
				"too many freelists for a 1 byte lookup index");

			total += range_meta[i].stride_count;
		}
		return total;
	}

	static constexpr auto create_lookup_table()
	{
		std::size_t offset = 0;
		std::array<LookupEntry, compute_number_of_entries()> table = {};

		constexpr auto& range_meta = Config::range_metadata;
		for (std::size_t mp_index = 0; mp_index < range_meta.size(); ++mp_index) {

			const std::size_t freelist_count = range_meta[mp_index].stride_count;

			for (std::size_t fl_index = 0; fl_index < freelist_count; ++fl_index) {
				table[offset + fl_index] = LookupEntry {
					static_cast<uint8_t>(mp_index),
					static_cast<uint8_t>(fl_index)
				};
			}
			offset += freelist_count;
		}
		return table;
	}

private:

	static inline constexpr LookupEntry lookup(uint32_t raw_size, uint32_t alignment)
	{
		constexpr auto& range_meta = Config::range_metadata;
		constexpr uint32_t metapool_count = range_meta.size();

		const uint32_t alloc_size  = raw_size + mem::alloc_header_size;

		for (uint32_t i = 0; i < metapool_count; ++i) {

			const auto& range = range_meta[i];
			const uint32_t step = range.stride_step;

			const uint32_t align_to = std::max(step, alignment);
			const uint32_t stride = (alloc_size + align_to - 1U) & ~(align_to - 1U);

			if (stride >= range.stride_max)
				continue;

			const uint32_t stride_floor = std::max(stride, range.stride_min);
			const uint32_t offset = stride_floor - range.stride_min;
			const uint32_t index = offset >> range.stride_shift;

			assert((offset & (step - 1U)) == 0 &&
				"allocator lookup: stride unaligned");

			return LookupEntry {
				static_cast<uint8_t>(i),
				static_cast<uint8_t>(index)
			};
		}

		assert(false && "allocator lookup: no suitable metapool for stride and alignment");
		return LookupEntry {0xFF, 0xFF};
	}


	static constexpr uint32_t compute_alignment(uint32_t alignment_min) noexcept
	{
		return (alignment_min + Config::alignment_quantum - 1U)
			& ~(Config::alignment_quantum - 1U);
	}
	
	static constexpr uint32_t compute_stride(uint32_t size, uint32_t step) noexcept
	{
		return (size + mem::alloc_header_size + step - 1U)
			& ~(step - 1U);
	}


	template <typename T>
	static inline constexpr uint32_t get_type_stride()
	{
		constexpr uint32_t alignment = ((alignof(T) + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U));
		return (sizeof(T) + mem::alloc_header_size + alignment - 1U) & ~(alignment - 1U);
	}

	static inline constexpr uint32_t get_pmr_stride(uint32_t bytes, uint32_t alignment)
	{
		alignment = (alignment + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
		return (bytes + alignment - 1U) & ~(alignment - 1U);
	}

protected:

	std::array<LookupEntry, compute_number_of_entries()> m_lookup_table {create_lookup_table()};

	ProxyArray m_proxies {};
};



template <mem::IsAllocatorConfig Config, typename AdapterPolicy = Native, typename T = void>
class Allocator;


template <mem::IsAllocatorConfig Config>
class Allocator<Config, Native> : public AllocatorCore<Config>
{
public:

	using AllocatorCore<Config>::AllocatorCore;
};


template <mem::IsAllocatorConfig Config, typename T>
class Allocator<Config, StdAdapter, T> : public AllocatorCore<Config>
{
public:

	using value_type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;
	using size_type = std::size_t;
	using difference_type = std::ptrdiff_t;

	using AllocatorCore<Config>::AllocatorCore;

	Allocator(const Allocator& other) noexcept = default;
	Allocator(Allocator&& other) noexcept = default;
	Allocator& operator=(const Allocator& other) noexcept = default;
	Allocator& operator=(Allocator&& other) noexcept = default;

	template <typename = void>
	Allocator(const Allocator<Config, StdAdapter, void>& other)
		: AllocatorCore<Config>(other)
	{}

	template <typename ObjType>
	struct rebind
	{
		using other = Allocator<Config, StdAdapter, ObjType>;
	};

	pointer allocate(size_type bytes)
	{
		return reinterpret_cast<pointer>(this->alloc(
			static_cast<uint32_t>(bytes * sizeof(value_type)),
			static_cast<uint32_t>(alignof(value_type))
		));
	}

	void deallocate(pointer ptr, size_type)
	{
		this->free(reinterpret_cast<std::byte*>(ptr));
	}
};

template <mem::IsAllocatorConfig Config>
class Allocator<Config, StdAdapter, void> : public AllocatorCore<Config>
{
public:

	using value_type = void;

	using AllocatorCore<Config>::AllocatorCore;

	Allocator() = delete;
	~Allocator() = default;

	Allocator(const Allocator&) noexcept = default;
	Allocator(Allocator&&) noexcept = default;
	Allocator& operator=(const Allocator&) noexcept = default;
	Allocator& operator=(Allocator&&) noexcept = default;

	template <typename ObjType>
	struct rebind
	{
		using other = Allocator<Config, StdAdapter, ObjType>;
	};
};


template <mem::IsAllocatorConfig Config>
class Allocator<Config, PmrAdapter> : public AllocatorCore<Config>, public std::pmr::memory_resource
{
public:

	using AllocatorCore<Config>::AllocatorCore;

protected:

	void* do_allocate(std::size_t bytes, std::size_t alignment) override
	{
		return this->alloc(static_cast<uint32_t>(bytes), static_cast<uint32_t>(alignment));
	}

	void do_deallocate(void* ptr, std::size_t, std::size_t) override
	{
		this->free(reinterpret_cast<std::byte*>(ptr));
	}

	bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
	}
};


template <mem::IsAllocatorConfig Config>
AllocatorCore(const AllocatorCore<Config>&) -> AllocatorCore<Config>;

} // hpr

#include "allocator.tpp"
