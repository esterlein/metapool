#pragma once

#include "../mtp/mtpint.hpp"

#include <new>
#include <cstddef>
#include <utility>
#include <type_traits>

#include "../mtp/fail.hpp"


namespace mtp::cntr {


template <typename T, size_t Size>
class crib
{
	static_assert(!std::is_reference_v<T> && !std::is_void_v<T>,
		CRIB_REF_MSG);

public:

	crib() = default;

	crib(const crib&) = delete;
	crib& operator=(const crib&) = delete;

	crib(crib&&) = delete;
	crib& operator=(crib&&) = delete;

	~crib() = default;

public:

	static constexpr size_t size()
	{
		return Size;
	}

	T* data()
	{
		return reinterpret_cast<T*>(m_storage);
	}

	const T* data() const
	{
		return reinterpret_cast<const T*>(m_storage);
	}

	T& operator[](size_t index)
	{
		return data()[index];
	}

	const T& operator[](size_t index) const
	{
		return data()[index];
	}

	T* begin()
	{
		return data();
	}

	T* end()
	{
		return data() + Size;
	}

	const T* begin() const
	{
		return data();
	}

	const T* end() const
	{
		return data() + Size;
	}

	const T* cbegin() const
	{
		return data();
	}

	const T* cend() const
	{
		return data() + Size;
	}

public:

	template <typename... Types>
	T& construct(size_t index, Types&&... args)
	{
		T* location = data() + index;
		return *new (location) T(std::forward<Types>(args)...);
	}

	void destruct(size_t index) noexcept
	{
		T* location = data() + index;
		location->~T();
	}

	template <typename U = T>
	void destruct(size_t index) noexcept
	requires (std::is_trivially_destructible_v<U>)
	{
		(void) index;
	}

	template <typename U = T>
	void destruct_range(size_t count) noexcept
	requires (!std::is_trivially_destructible_v<U>)
	{
		for (size_t i = 0; i < count; ++i) {
			destruct(i);
		}
	}

	template <typename U = T>
	void destruct_range(size_t count) noexcept
	requires (std::is_trivially_destructible_v<U>)
	{
		(void) count;
	}

private:

	alignas(T) std::byte m_storage[sizeof(T) * Size] {};
};


} // mtp::cntr

