#pragma once

#include <cstddef>
#include <stdexcept>
#include <memory_resource>

#include "metapool.hpp"
#include "metapool_interface.hpp"


namespace hpr {


template <std::size_t MetapoolCount>
class Allocator final : public std::pmr::memory_resource
{
public:

	template <typename Array>
	requires metapool_descriptor_array<Array, MetapoolCount>
	constexpr Allocator(Array&& descriptors)
		: m_descriptors(std::forward<Array>(descriptors))
	{}

	~Allocator() = default;

	Allocator(const Allocator&) = default;
	Allocator(Allocator&&) = default;
	Allocator& operator=(const Allocator&) = default;
	Allocator& operator=(Allocator&&) = default;

private:

	using MetapoolAllocate = void* (*)(void* metapool, std::size_t stride);
	using MetapoolDeallocate = void (*)(void* metapool, std::byte* block);
	template <typename T, typename... Args>
	using MetapoolConstruct = T* (*)(void* metapool, std::size_t stride, Args&&...);
	template <typename T>
	using MetapoolDestruct = void (*)(void* metapool, T* object);

	struct MetapoolEntry
	{
		void* metapool;
	};

	template <typename T, typename... Types>
	T* construct(Types&&... args)
	{
		std::size_t stride = get_type_stride<T>();

		for (auto& descriptor : m_descriptors) {
			if (stride >= descriptor.lower_bound && stride <= descriptor.upper_bound) {

				return std::visit([stride, &args...](auto* pool) -> T* {
					T* object = pool->template construct<T>(stride, std::forward<Types>(args)...);
					return std::launder(object);
				}, descriptor.metapool);
			}
		}
		throw std::runtime_error("no suitable metapool found for object construction");
	}

	template <typename T>
	void destruct(T* object)
	{
		std::size_t stride = get_type_stride<T>();

		for (auto& descriptor : m_descriptors) {
			if (stride >= descriptor.lower_bound && stride <= descriptor.upper_bound) {

				std::visit([stride, object](auto* pool) {
					pool->template destruct<T>(stride, object);
				}, descriptor.metapool);
				return;
			}
		}
		throw std::runtime_error("no suitable metapool found for object destruction");
	}

protected:

	void* do_allocate(std::size_t bytes, std::size_t alignment) override;

	void do_deallocate(void* address, std::size_t bytes, std::size_t alignment) override;

	inline bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
	}

private:

	template <typename T>
	static inline constexpr std::size_t get_type_stride()
	{
		constexpr std::size_t alignment = ((alignof(T) + 7UL) & ~7UL);
		return (sizeof(T) + alignment - 1) & ~(alignment - 1);
	}

	static inline constexpr std::size_t get_pmr_stride(std::size_t bytes, std::size_t alignment)
	{
		alignment = (alignment + 7UL) & ~7UL;
		return (bytes + alignment - 1) & ~(alignment - 1);
	}

private:

	std::array<MetapoolDescriptor, MetapoolCount> m_descriptors;

};


template <std::size_t MetapoolCount>
Allocator(std::array<MetapoolDescriptor, MetapoolCount>&) -> Allocator<MetapoolCount>;


template <std::size_t MetapoolCount>
Allocator(std::array<MetapoolDescriptor, MetapoolCount>&&) -> Allocator<MetapoolCount>;

} // hpr

#include "allocator.tpp"
