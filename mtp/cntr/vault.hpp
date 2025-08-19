#pragma once

#include <cstddef>
#include <cassert>
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
		clear();
		if (m_allocator && m_beg)
			m_allocator->free(reinterpret_cast<std::byte*>(m_beg));
	}

	vault(const vault&) = delete;
	vault& operator=(const vault&) = delete;

	vault(vault&& other) noexcept
		: m_allocator {other.m_allocator}
		, m_beg       {other.m_beg}
		, m_end       {other.m_end}
		, m_cap       {other.m_cap}
	{
		MTP_ASSERT(this != &other,
			mtp::err::vault_self_move_ctor);

		MTP_ASSERT(m_beg != nullptr,
			mtp::err::vault_move_null_data);

		other.m_allocator = nullptr;
		other.m_beg = nullptr;
		other.m_end = nullptr;
		other.m_cap = nullptr;
	}

	vault& operator=(vault&& other) noexcept
	{
		MTP_ASSERT(this != &other,
			mtp::err::vault_self_move_assign);

		MTP_ASSERT(other.m_beg != nullptr,
			mtp::err::vault_move_null_data);

		if (m_allocator && m_beg && m_beg != other.m_beg) {
			clear();
			m_allocator->free(reinterpret_cast<std::byte*>(m_beg));
		}

		m_allocator = other.m_allocator;
		m_beg = other.m_beg;
		m_end = other.m_end;
		m_cap = other.m_cap;

		other.m_allocator = nullptr;
		other.m_beg = nullptr;
		other.m_end = nullptr;
		other.m_cap = nullptr;

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

	void clear()
	{
		MTP_ASSERT(m_end == m_beg || m_beg != nullptr,
			mtp::err::vault_clear_null_data);
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (T* p = m_beg; p != m_end; ++p)
				p->~T();
		}
		m_end = m_beg;
	}

	void reset(std::size_t new_capacity)
	{
		if constexpr (!std::is_trivially_destructible_v<T>) {
			for (T* p = m_beg; p != m_end; ++p)
				p->~T();
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
			for (T* p = m_beg; p != m_end; ++p)
				p->~T();
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

	std::size_t size() const { return static_cast<std::size_t>(m_end - m_beg); }
	std::size_t capacity() const { return static_cast<std::size_t>(m_cap - m_beg); }
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

private:

	T* m_beg {nullptr};
	T* m_end {nullptr};
	T* m_cap {nullptr};

	RawAllocator* m_allocator [[maybe_unused]] {};
};

} // mtp::cntr

