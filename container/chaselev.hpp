#pragma once

#include "../mtp/mtpint.hpp"
#include "container_common.hpp"

#include <bit>
#include <atomic>
#include <cstring>
#include <utility>
#include <algorithm>
#include <type_traits>

#include "../mtp/fail.hpp"
#include "../mtp/memory_model.hpp"


namespace mtp::cntr {


template <typename T, typename Set>
class chaselev
{
	static_assert(!std::is_reference_v<T> && !std::is_void_v<T>,
		CHASELEV_REF_MSG);

	static_assert(std::is_trivially_copyable_v<T>,
		CHASELEV_TRIVIAL_COPY_MSG);

	static_assert(std::is_trivially_destructible_v<T>,
		CHASELEV_TRIVIAL_DTOR_MSG);

public:

	using SharedAllocator = core::MemoryModel::Shared <
		Set,
		cfg::AllocatorTag::std_adapter
	>;

	using RawAllocator =
		std::remove_reference_t<decltype(std::declval<SharedAllocator&>().get())>;

	explicit chaselev(SharedAllocator& shared)
		: m_allocator {shared.get_ptr()}
	{}

	chaselev(SharedAllocator& shared, size_t capacity)
		: m_allocator {shared.get_ptr()}
	{
		reserve(capacity);
	}

	~chaselev()
	{
		release_storage();
	}

	chaselev(const chaselev&) = delete;
	chaselev& operator=(const chaselev&) = delete;

	chaselev(chaselev&& other) noexcept
		: m_data      {other.m_data}
		, m_capacity  {other.m_capacity}
		, m_top       {other.m_top.load(std::memory_order_relaxed)}
		, m_bottom    {other.m_bottom.load(std::memory_order_relaxed)}
		, m_allocator {other.m_allocator}
	{
		other.m_data = nullptr;
		other.m_capacity = 0;
		other.m_top.store(0, std::memory_order_relaxed);
		other.m_bottom.store(0, std::memory_order_relaxed);
		other.m_allocator = nullptr;
	}

	chaselev& operator=(chaselev&& other) noexcept
	{
		if (this == &other) {
			return *this;
		}

		release_storage();

		m_data = other.m_data;
		m_capacity = other.m_capacity;
		m_top.store(other.m_top.load(std::memory_order_relaxed), std::memory_order_relaxed);
		m_bottom.store(other.m_bottom.load(std::memory_order_relaxed), std::memory_order_relaxed);
		m_allocator = other.m_allocator;

		other.m_data = nullptr;
		other.m_capacity = 0;
		other.m_top.store(0, std::memory_order_relaxed);
		other.m_bottom.store(0, std::memory_order_relaxed);
		other.m_allocator = nullptr;

		return *this;
	}

public:

	bool empty() const
	{
		const size_t top    = m_top.load(std::memory_order_relaxed);
		const size_t bottom = m_bottom.load(std::memory_order_relaxed);

		return bottom <= top;
	}

	size_t size() const
	{
		const size_t top    = m_top.load(std::memory_order_relaxed);
		const size_t bottom = m_bottom.load(std::memory_order_relaxed);

		return (bottom > top) ? (bottom - top) : 0;
	}

	size_t capacity() const
	{
		return m_capacity;
	}

	void clear()
	{
		m_top.store(0, std::memory_order_relaxed);
		m_bottom.store(0, std::memory_order_relaxed);
	}

	bool push_bottom(const T& item)
	{
		const size_t top    = m_top.load(std::memory_order_acquire);
		const size_t bottom = m_bottom.load(std::memory_order_relaxed);

		if (m_capacity == 0 || bottom - top >= m_capacity) {
			return false;
		}

		m_data[slot_index(bottom)] = item;

		m_bottom.store(bottom + 1, std::memory_order_release);
		return true;
	}

	bool push_bottom(T&& item)
	{
		const size_t top    = m_top.load(std::memory_order_acquire);
		const size_t bottom = m_bottom.load(std::memory_order_relaxed);

		if (m_capacity == 0 || bottom - top >= m_capacity) {
			return false;
		}

		m_data[slot_index(bottom)] = std::move(item);

		m_bottom.store(bottom + 1, std::memory_order_release);
		return true;
	}

	bool pop_bottom(T& item)
	{
		size_t bottom = m_bottom.load(std::memory_order_relaxed);
		if (bottom == 0) {
			return false;
		}

		bottom -= 1;
		m_bottom.store(bottom, std::memory_order_relaxed);

		size_t top = m_top.load(std::memory_order_acquire);

		if (top > bottom) {
			m_bottom.store(bottom + 1, std::memory_order_relaxed);
			return false;
		}

		if (top == bottom) {
			const bool claimed_last_item = m_top.compare_exchange_strong(
				top,
				top + 1,
				std::memory_order_acq_rel,
				std::memory_order_relaxed
			);

			m_bottom.store(bottom + 1, std::memory_order_relaxed);

			if (!claimed_last_item) {
				return false;
			}
		}

		item = m_data[slot_index(bottom)];
		return true;
	}

	bool steal_top(T& item)
	{
		size_t top = m_top.load(std::memory_order_acquire);

		const size_t bottom = m_bottom.load(std::memory_order_acquire);

		if (top >= bottom) {
			return false;
		}

		item = m_data[slot_index(top)];

		const bool claimed_item = m_top.compare_exchange_strong(
			top,
			top + 1,
			std::memory_order_acq_rel,
			std::memory_order_relaxed
		);

		return claimed_item;
	}

	void reserve(size_t desired_capacity)
	{
		if (desired_capacity <= m_capacity) { [[unlikely]]
			return;
		}

		MTP_ASSERT(empty(),
			err::chaselev_reserve_non_empty);

		const size_t top    = m_top.load(std::memory_order_relaxed);
		const size_t bottom = m_bottom.load(std::memory_order_relaxed);
		const size_t count  = (bottom > top) ? (bottom - top) : 0;

		const size_t new_capacity   = std::bit_ceil(desired_capacity);
		T* MTP_RESTRICT new_storage = alloc_storage(new_capacity);

		if (m_data) {
			if (count) {
				move_to_new_storage(new_storage, top, bottom);
			}
			free_storage(m_data);
		}

		m_data     = new_storage;
		m_capacity = new_capacity;

		m_top.store(0, std::memory_order_relaxed);
		m_bottom.store(count, std::memory_order_relaxed);
	}

private:

	size_t slot_index(size_t index) const
	{
		return index & (m_capacity - 1);
	}

	T* alloc_storage(size_t capacity)
	{
		std::byte* block = m_allocator->alloc(sizeof(T) * capacity, alignof(T));
		return reinterpret_cast<T*>(block);
	}

	void free_storage(T* block_ptr)
	{
		if (!block_ptr) {
			return;
		}
		m_allocator->free(reinterpret_cast<std::byte*>(block_ptr));
	}

	void move_to_new_storage(T* new_storage, size_t top, size_t bottom)
	{
		T* MTP_RESTRICT src_data = m_data;
		T* MTP_RESTRICT dst_data = new_storage;

		const size_t count = bottom - top;
		if (count == 0) {
			return;
		}

		const size_t index_mask = m_capacity - 1;

		const size_t first_blob_idx = top & index_mask;
		const size_t first_blob_len = std::min(count, m_capacity - first_blob_idx);

		std::memcpy(dst_data, src_data + first_blob_idx, first_blob_len * sizeof(T));

		const size_t second_blob_len = count - first_blob_len;
		if (second_blob_len) {
			std::memcpy(dst_data + first_blob_len, src_data, second_blob_len * sizeof(T));
		}
	}

	void release_storage()
	{
		if (!m_data) {
			return;
		}

		free_storage(m_data);

		m_data = nullptr;
		m_capacity = 0;
		m_top.store(0, std::memory_order_relaxed);
		m_bottom.store(0, std::memory_order_relaxed);
		m_allocator = nullptr;
	}

private:

	T* m_data         {nullptr};
	size_t m_capacity {0};

	alignas(std::hardware_destructive_interference_size) std::atomic<size_t> m_top    {0};
	alignas(std::hardware_destructive_interference_size) std::atomic<size_t> m_bottom {0};

	RawAllocator* m_allocator {};
};


} // mtp::cntr

