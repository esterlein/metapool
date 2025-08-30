#pragma once

#include <cstddef>
#include <cassert>
#include <cstring>
#include <utility>

#include "../memory_model.hpp"

#include "fail.hpp"


namespace mtp::cntr {


template <typename T, typename Set>
class vault
{
	static_assert(!std::is_reference_v<T> && !std::is_void_v<T>,
		VAULT_REF_MSG);

public:

	using AllocatorType = decltype(mtp::core::MemoryModel::create_thread_local_allocator <
		Set,
		mtp::cfg::AllocatorTag::std_adapter
	>());
	using RawAllocator = std::remove_reference_t<AllocatorType>;

	vault()
		: m_allocator {
			&mtp::core::MemoryModel::create_thread_local_allocator <
				Set,
				mtp::cfg::AllocatorTag::std_adapter
			>()
		}
	{}

	vault(std::size_t capacity)
		: m_allocator {
			&mtp::core::MemoryModel::create_thread_local_allocator <
				Set,
				mtp::cfg::AllocatorTag::std_adapter
			>()
		}
	{
		std::byte* block = m_allocator->alloc(sizeof(T) * capacity, alignof(T));
		m_beg = reinterpret_cast<T*>(block);
		m_end = m_beg;
		m_cap = m_beg + capacity;
	}

	template <typename... Types>
	vault(std::size_t count, Types&&... args)
		: m_allocator {
			&mtp::core::MemoryModel::create_thread_local_allocator <
				Set,
				mtp::cfg::AllocatorTag::std_adapter
			>()
		}
	{
		std::byte* block = m_allocator->alloc(sizeof(T) * count, alignof(T));
		m_beg = reinterpret_cast<T*>(block);
		m_end = m_beg + count;
		m_cap = m_beg + count;

		for (std::size_t i = 0; i < count; ++i)
			new (m_beg + i) T(std::forward<Types>(args)...);
	}

	~vault()
	{
		release_storage();
	}

	vault(const vault&) = delete;
	vault& operator=(const vault&) = delete;

	vault(vault&& other) noexcept
		: m_beg{other.m_beg}
		, m_end{other.m_end}
		, m_cap{other.m_cap}
		, m_allocator{other.m_allocator}
	{
		other.m_beg = nullptr;
		other.m_end = nullptr;
		other.m_cap = nullptr;
		other.m_allocator = nullptr;
	}

	vault& operator=(vault&& other) noexcept
	{
		if (this == &other)
			return *this;

		release_storage();

		m_beg = other.m_beg;
		m_end = other.m_end;
		m_cap = other.m_cap;
		m_allocator = other.m_allocator;

		other.m_beg = nullptr;
		other.m_end = nullptr;
		other.m_cap = nullptr;
		other.m_allocator = nullptr;

		return *this;
	}

	T& operator[](std::size_t index)
	{
		MTP_ASSERT(index < size(),
			mtp::err::vault_index_oob);
		return m_beg[index];
	}

	const T& operator[](std::size_t index) const
	{
		MTP_ASSERT(index < size(),
			mtp::err::vault_index_oob);
		return m_beg[index];
	}

	T& back()
	{
		MTP_ASSERT(size() > 0,
			mtp::err::vault_back_empty);
		return *(m_end - 1);
	}

	const T& back() const
	{
		MTP_ASSERT(size() > 0,
			mtp::err::vault_back_empty);
		return *(m_end - 1);
	}

	void pop_back()
	{
		MTP_ASSERT(size() > 0,
			mtp::err::vault_pop_empty);
		--m_end;
		if constexpr (!std::is_trivially_destructible_v<T>)
			m_end->~T();
	}

	void push_back(const T& value)
	{
		if (m_end == m_cap)
			grow();

		new (m_end++) T(value);
	}

	void push_back(T&& value)
	{
		if (m_end == m_cap)
			grow();

		new (m_end++) T(std::move(value));
	}

	template <typename... Types>
	T& emplace_back(Types&&... args)
	{
		if (m_end == m_cap)
			grow();

		return *new (m_end++) T(std::forward<Types>(args)...);
	}

	void assign(std::size_t count, const T& value)
	{
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (T* __restrict__ ptr = m_beg; ptr != m_end; ++ptr)
				ptr->~T();
		}
		if (count > capacity())
			reserve(count);

		T* __restrict__ dst = m_beg;
		for (std::size_t i = 0; i < count; ++i)
			new (dst + i) T(value);

		m_end = dst + count;
	}

	template <typename InputIt>
	requires (!std::is_integral_v<InputIt>)
	void assign(InputIt first, InputIt last)
	{
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (T* __restrict__ ptr = m_beg; ptr != m_end; ++ptr)
				ptr->~T();
		}

		using cat = typename std::iterator_traits<InputIt>::iterator_category;

		if constexpr (std::is_base_of_v<std::forward_iterator_tag, cat>) {
			const std::size_t count = static_cast<std::size_t>(std::distance(first, last));
			if (count > capacity())
				reserve(count);

			T* __restrict__ dst = m_beg;
			for (auto it = first; it != last; ++it, ++dst)
				new (dst) T(*it);

			m_end = m_beg + count;
		}
		else {
			m_end = m_beg;
			for (auto it = first; it != last; ++it) {
				const T& __restrict__ src = *it;
				emplace_back(src);
			}
		}
	}

	void assign(std::initializer_list<T> ilist)
	{
		assign(ilist.begin(), ilist.end());
	}

	void reserve(std::size_t new_cap)
	{
		if (new_cap <= static_cast<std::size_t>(m_cap - m_beg))
			return;

		std::size_t count = static_cast<std::size_t>(m_end - m_beg);

		T* __restrict__ new_beg = reinterpret_cast<T*>(
			m_allocator->alloc(sizeof(T) * new_cap, alignof(T))
		);
		T* __restrict__ new_end = new_beg;

		if constexpr (std::is_trivially_move_constructible_v<T>) {
			std::memcpy(new_beg, m_beg, sizeof(T) * count);
			new_end = new_beg + count;
		}
		else {
			for (std::size_t i = 0; i < count; ++i)
				new (new_beg + i) T(std::move(m_beg[i]));
			new_end = new_beg + count;
		}

		if (m_beg)
			m_allocator->free(reinterpret_cast<std::byte*>(m_beg));

		m_beg = new_beg;
		m_end = new_end;
		m_cap = new_beg + new_cap;
	}

	void resize(std::size_t new_size)
	{
		const std::size_t old_size = size();
		if (new_size > capacity())
			reserve(new_size);

		T* __restrict__ beg = m_beg;

		if (new_size > old_size) {
			if constexpr (!std::is_trivially_default_constructible_v<T>) {
				T* __restrict__ first = beg + old_size;
				T* __restrict__ last  = beg + new_size;
				for (T* ptr = first; ptr != last; ++ptr)
					new (ptr) T();
			}
		}
		else {
			resize_helper(new_size, old_size);
		}

		m_end = beg + new_size;
	}

	void resize(std::size_t new_size, const T& value)
	{
		const std::size_t old_size = size();
		if (new_size > capacity())
			reserve(new_size);

		T* __restrict__ beg = m_beg;

		if (new_size > old_size) {
			T* __restrict__ first = beg + old_size;
			T* __restrict__ last  = beg + new_size;
			for (T* ptr = first; ptr != last; ++ptr)
				new (ptr) T(value);
		}
		else {
			resize_helper(new_size, old_size);
		}

		m_end = beg + new_size;
	}

	void clear() noexcept
	{
		if (!m_beg || m_end == m_beg)
			return;
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (T* ptr = m_beg; ptr != m_end; ++ptr)
				ptr->~T();
		}
		m_end = m_beg;
	}

	void reset(std::size_t new_capacity)
	{
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (T* ptr = m_beg; ptr != m_end; ++ptr)
				ptr->~T();
		}

		if (m_beg)
			m_allocator->free(reinterpret_cast<std::byte*>(m_beg));

		m_beg = reinterpret_cast<T*>(
			m_allocator->alloc(sizeof(T) * new_capacity, alignof(T))
		);
		m_end = m_beg;
		m_cap = m_beg + new_capacity;
	}

	template <typename... Types>
	void reset(std::size_t new_capacity, Types&&... args)
	{
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (T* ptr = m_beg; ptr != m_end; ++ptr)
				ptr->~T();
		}

		if (m_beg)
			m_allocator->free(reinterpret_cast<std::byte*>(m_beg));

		m_beg = reinterpret_cast<T*>(
			m_allocator->alloc(sizeof(T) * new_capacity, alignof(T))
		);
		m_end = m_beg + new_capacity;
		m_cap = m_end;

		for (std::size_t i = 0; i < new_capacity; ++i)
			new (m_beg + i) T(std::forward<Types>(args)...);
	}

public:

	T* data() { return m_beg; }
	const T* data() const { return m_beg; }

	std::size_t size() const { return m_beg ? static_cast<std::size_t>(m_end - m_beg) : 0; }
	std::size_t capacity() const { return m_beg ? static_cast<std::size_t>(m_cap - m_beg) : 0; }
	bool empty() const { return m_end == m_beg; }

	T* begin() { return m_beg; }
	T* end() { return m_end; }
	const T* begin() const { return m_beg; }
	const T* end() const { return m_end; }
	const T* cbegin() const { return m_beg; }
	const T* cend() const { return m_end; }

private:

	void grow()
	{
		const std::size_t count = static_cast<std::size_t>(m_end - m_beg);
		const std::size_t new_cap = count == 0 ? 8 : count * 2;

		T* __restrict__ new_beg = reinterpret_cast<T*>(
			m_allocator->alloc(sizeof(T) * new_cap, alignof(T))
		);
		T* __restrict__ old = m_beg;

		for (std::size_t i = 0; i < count; ++i)
			new (new_beg + i) T(std::move(old[i]));

		if (old)
			m_allocator->free(reinterpret_cast<std::byte*>(old));

		m_beg = new_beg;
		m_end = new_beg + count;
		m_cap = new_beg + new_cap;
	}

	void resize_helper(std::size_t new_size, std::size_t old_size)
	{
		T* __restrict__ beg = m_beg;
		if (new_size < old_size) {
			if constexpr (!std::is_trivially_destructible_v<T>) {
				T* __restrict__ first = beg + new_size;
				T* __restrict__ last  = beg + old_size;
				for (T* ptr = first; ptr != last; ++ptr) {
					std::destroy_at(ptr);
				}
			}
		}
		m_end = beg + new_size;
	}

	void release_storage() noexcept
	{
		if (!m_beg)
			return;
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (T* ptr = m_beg; ptr != m_end; ++ptr)
				ptr->~T();
		}
		m_allocator->free(reinterpret_cast<std::byte*>(m_beg));
		m_beg = m_end = m_cap = nullptr;
	}

private:

	T* m_beg {nullptr};
	T* m_end {nullptr};
	T* m_cap {nullptr};

	RawAllocator* m_allocator [[maybe_unused]] {};
};

} // mtp::cntr

