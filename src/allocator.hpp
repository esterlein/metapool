#pragma once

#include <cstddef>
#include <cstdint>

#include <memory_resource>

#include "alloc_header.hpp"
#include "metapool_proxy.hpp"
#include "allocator_config.hpp"


namespace hpr {


template <mem::IsAllocatorConfig Config>
class Allocator : public std::pmr::memory_resource
{
public:

	using ProxyArray = typename Config::ProxyArrayType;

	constexpr Allocator(ProxyArray proxies)
		: m_proxies {std::move(proxies)}
	{}

	Allocator() = delete;
	virtual ~Allocator() = default;

	Allocator(const Allocator&) = delete;
	Allocator(Allocator&&) = delete;
	Allocator& operator=(const Allocator&) = delete;
	Allocator& operator=(Allocator&&) = delete;

	// transparent interface / replace method calls

	// uint32_t enum class error codes in hot paths

public:

	template <typename T, typename... Types>
	[[nodiscard]] T* construct(Types&&... args)
	{
		constexpr uint32_t alignment =
			(static_cast<uint32_t>(alignof(T)) + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
		constexpr uint32_t stride =
			(static_cast<uint32_t>(sizeof(T)) + mem::alloc_header_size + alignment - 1U) & ~(alignment - 1U);

		if (stride < Config::min_stride || stride > Config::max_stride) [[unlikely]]
			throw std::bad_alloc{};

		auto lookup_entry = lookup(stride);

		auto& proxy = m_proxies[lookup_entry.mpool_index];
		std::byte* block = proxy.fetch(lookup_entry.flist_index);

		if (!block) [[unlikely]]
			throw std::bad_alloc{};

		auto* header = reinterpret_cast<mem::AllocHeader*>(block);
		*header = mem::AllocHeader::make(lookup_entry.mpool_index, lookup_entry.flist_index);

		std::byte* object_location = block + sizeof(mem::AllocHeader);
		T* object = std::launder(new (object_location) T(std::forward<Types>(args)...));

		return object;
	}

	template <typename T>
	void destruct(T* object)
	{
		if (!object) [[unlikely]]
			return;

		std::byte* block = reinterpret_cast<std::byte*>(object) - sizeof(mem::AllocHeader);
		auto* header = reinterpret_cast<const mem::AllocHeader*>(block);
	
		auto& proxy = m_proxies[header->mpool_index()];

		object->~T();

		proxy.release(header->flist_index(), block);
	}

protected:

	virtual void* do_allocate(std::size_t bytes, std::size_t alignment) override;

	virtual void do_deallocate(void* address, std::size_t bytes, std::size_t alignment) override;

	inline virtual bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
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

	static constexpr LookupEntry lookup(uint32_t stride)
	{
		constexpr auto& range_meta = Config::range_metadata;

		std::size_t low = 0;
		std::size_t high = range_meta.size() - 1;

		while (low < high) {
			std::size_t mid = (low + high + 1) >> 1;
			if (stride >= range_meta[mid].stride_min) {
				low = mid;
			}
			else {
				high = mid - 1;
			}
		}

		auto const& mp_range = range_meta[low];
		uint32_t fl_index = (stride - mp_range.stride_min) / mp_range.stride_step;

		return LookupEntry {
			static_cast<uint8_t>(low),
			static_cast<uint8_t>(fl_index)
		};
	}

	template <typename T>
	static inline constexpr uint32_t get_type_stride()
	{
		constexpr uint32_t  alignment = ((alignof(T) + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U));
		return (sizeof(T) + mem::alloc_header_size + alignment - 1U) & ~(alignment - 1U);
	}

	static inline constexpr uint32_t get_pmr_stride(uint32_t bytes, uint32_t alignment)
	{
		alignment = (alignment + Config::alignment_quantum - 1U) & ~(Config::alignment_quantum - 1U);
		return (bytes + alignment - 1U) & ~(alignment - 1U);
	}

private:

	std::array<LookupEntry, compute_number_of_entries()> m_lookup_table {create_lookup_table()};

	ProxyArray m_proxies {};
};


template <mem::IsAllocatorConfig Config>
Allocator(const Allocator<Config>&) -> Allocator<Config>;

} // hpr

#include "allocator.tpp"
