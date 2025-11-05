#pragma once

#include "mtpint.hpp"

#include <span>
#include <cassert>

#include <memory_resource>

#include "alloc_tracer.hpp"
#include "freelist_proxy.hpp"
#include "allocator_config.hpp"

#include "fail.hpp"


namespace mtp::core {


struct Native {};
struct StdAdapter {};
struct PmrAdapter {};


template <mtp::cfg::IsAllocatorConfig Config>
class AllocatorCore
{
	static_assert(Config::range_metadata.size() <= 256,
		CORE_TOO_MANY_POOLS_MSG);

public:

	using proxy_index_t = decltype(Config::range_metadata[0].base_proxy_index);

	constexpr AllocatorCore(std::span<FreelistProxy> proxies)
		: m_proxies {proxies}
	{}

	AllocatorCore() = delete;
	virtual ~AllocatorCore() = default;

	AllocatorCore(const AllocatorCore&) = default;

	AllocatorCore(AllocatorCore&&) = delete;
	AllocatorCore& operator=(const AllocatorCore&) = delete;
	AllocatorCore& operator=(AllocatorCore&&) = delete;

public:

	[[nodiscard]] inline std::byte* alloc(uint32_t size, uint32_t alignment)
	{
		MTP_ASSERT(size > 0,
			mtp::err::alloc_zero_size);

		proxy_index_t proxy_index = lookup(size, alignment);

		MTP_ASSERT(proxy_index < Config::total_stride_count,
			mtp::err::alloc_proxy_oob);

		std::byte* block = m_proxies[proxy_index].fetch();

		while (block == nullptr) [[unlikely]] {

			mtp::cfg::AllocTracer::trace_fallback(size, alignment, proxy_index);

			if (++proxy_index >= Config::total_stride_count) [[unlikely]] {

				fatal(mtp::err::alloc_proxy_oob,
					mtp::err::format_ctx("size = %u, align = %u, proxy = %u / %u",
						size, alignment, proxy_index, Config::total_stride_count - 1));
			}

			block = m_proxies[proxy_index].fetch();
		}

		return block;
	}


	inline void free(std::byte* block)
	{
		if (block == nullptr) [[unlikely]]
			return;

		const std::byte* header = block - sizeof(proxy_index_t);
	
		const proxy_index_t proxy_index =
			static_cast<proxy_index_t>(header[0]) |
			(static_cast<proxy_index_t>(header[1]) << 8);

		MTP_ASSERT(proxy_index < Config::total_stride_count,
			mtp::err::free_proxy_oob);

		m_proxies[proxy_index].release(block);
	}


	template <typename T, typename... Types>
	[[nodiscard]] inline T* construct(Types&&... args)
	{
		MTP_ASSERT(sizeof(T) > 0,
			mtp::err::zero_size_construct);

		constexpr uint32_t size = sizeof(T);
		constexpr uint32_t alignment = alignof(T);

		proxy_index_t proxy_index = lookup(size, alignment);
		std::byte* block = m_proxies[proxy_index].fetch();

		while (block == nullptr) [[unlikely]] {

			mtp::cfg::AllocTracer::trace_fallback(size, alignment, proxy_index);

			if (++proxy_index >= Config::total_stride_count) [[unlikely]] {

				fatal(mtp::err::construct_proxy_oob,
					mtp::err::format_ctx("size = %u, align = %u, proxy = %u / %u",
						size, alignment, proxy_index, Config::total_stride_count - 1));
			}

			block = m_proxies[proxy_index].fetch();
		}

		T* object = std::launder(new (block) T(std::forward<Types>(args)...));
		return object;
	}


	template <typename T, size_t ForceAlign, typename... Types>
	[[nodiscard]] inline T* construct_align(Types&&... args)
	{
		static_assert((ForceAlign & (ForceAlign - 1)) == 0,
			CORE_ALIGNMENT_POW2_MSG);

		constexpr uint32_t size = (sizeof(T) + ForceAlign - 1) & ~(ForceAlign - 1);
		constexpr uint32_t alignment = ForceAlign;

		proxy_index_t proxy_index = lookup(size, alignment);
		std::byte* block = m_proxies[proxy_index].fetch();

		while (block == nullptr) [[unlikely]] {

			mtp::cfg::AllocTracer::trace_fallback(size, alignment, proxy_index);

			if (++proxy_index >= Config::total_stride_count) [[unlikely]] {

				fatal(mtp::err::construct_proxy_oob,
					mtp::err::format_ctx("size = %u, align = %u, proxy = %u / %u",
						size, alignment, proxy_index, Config::total_stride_count - 1));
			}

			block = m_proxies[proxy_index].fetch();
		}

		T* object = std::launder(new (block) T(std::forward<Types>(args)...));
		return object;
	}


	template <typename T>
	inline void destruct(T* object)
	{
		if (object == nullptr) [[unlikely]]
			return;

		const std::byte* header = reinterpret_cast<std::byte*>(object) - sizeof(proxy_index_t);
	
		const proxy_index_t proxy_index =
			static_cast<proxy_index_t>(header[0]) |
			(static_cast<proxy_index_t>(header[1]) << 8);

		MTP_ASSERT(proxy_index < Config::total_stride_count,
			mtp::err::destruct_proxy_oob);

		object->~T();

		m_proxies[proxy_index].release(reinterpret_cast<std::byte*>(object));
	}

	inline void reset() noexcept
	{
		for (auto& proxy : m_proxies) {
			proxy.reset();
		}
	}

private:

	static inline constexpr proxy_index_t lookup(uint32_t raw_size, uint32_t alignment)
	{
		MTP_ASSERT(raw_size > 0,
			mtp::err::lookup_raw_size_zero);

		constexpr auto& metadata = Config::range_metadata;
		constexpr uint32_t range_count = Config::range_count;

		const uint32_t alloc_size = raw_size + sizeof(proxy_index_t);
		const uint32_t align_to   = std::max(Config::alignment_quantum, alignment);
		const uint32_t aligned    = (alloc_size + align_to - 1U) & ~(align_to - 1U);

		for (uint32_t mp_index = 0; mp_index < range_count; ++mp_index) {

			const auto& range     = metadata[mp_index];
			const uint32_t mask   = range.stride_step - 1U;
			const uint32_t stride = (aligned + mask) & ~mask;

			if (stride > range.stride_max)
				continue;

			const uint32_t offset = (stride - range.stride_min) &
				-static_cast<int32_t>(stride >= range.stride_min);

			const uint32_t fl_index = offset >> range.stride_shift;

			MTP_ASSERT((offset & mask) == 0,
				mtp::err::lookup_stride_misaligned);

			proxy_index_t proxy_index = range.base_proxy_index + fl_index;

			mtp::cfg::AllocTracer::trace(raw_size, alignment, stride, proxy_index);

			return proxy_index;
		}

		MTP_ASSERT_CTX(false,
			mtp::err::lookup_no_match,
			mtp::err::format_ctx("size = %u, align = %u", raw_size, alignment));

		return 0xFFFF;
	}

private:

	std::span<FreelistProxy> m_proxies;
};


template <mtp::cfg::IsAllocatorConfig Config, typename AdapterPolicy = Native, typename T = void>
class Allocator;


template <mtp::cfg::IsAllocatorConfig Config>
class Allocator<Config, Native> : public AllocatorCore<Config>
{
public:

	using AllocatorCore<Config>::AllocatorCore;
};


template <mtp::cfg::IsAllocatorConfig Config, typename T>
class Allocator<Config, StdAdapter, T> : public AllocatorCore<Config>
{
public:

	using value_type = T;
	using pointer = T*;
	using const_pointer = const T*;
	using reference = T&;
	using const_reference = const T&;
	using size_type = size_t;
	using difference_type = std::ptrdiff_t;

	using is_always_equal = std::true_type;
	using propagate_on_container_move_assignment = std::true_type;
	using propagate_on_container_copy_assignment = std::true_type;
	using propagate_on_container_swap = std::true_type;
	using AllocatorCore<Config>::AllocatorCore;

	Allocator(const Allocator& other) noexcept = default;
	Allocator(Allocator&& other) noexcept = default;
	Allocator& operator=(const Allocator& other) noexcept = default;
	Allocator& operator=(Allocator&& other) noexcept = default;

	template <typename = void>
	Allocator(const Allocator<Config, StdAdapter, void>& other)
		: AllocatorCore<Config>(other)
	{}

	template <typename Rebound>
	Allocator(const Allocator<Config, StdAdapter, Rebound>& other) noexcept
		: AllocatorCore<Config>(static_cast<const AllocatorCore<Config>&>(other))
	{}

	template <typename Rebound>
	bool operator==(const Allocator<Config, StdAdapter, Rebound>&) const noexcept
	{
		return true;
	}

	template <typename Rebound>
	bool operator!=(const Allocator<Config, StdAdapter, Rebound>&) const noexcept
	{
		return false;
	}

	template <typename Rebound>
	struct rebind
	{
		using other = Allocator<Config, StdAdapter, Rebound>;
	};

	template <typename ObjType>
	using rebind_t = Allocator<Config, StdAdapter, ObjType>;

	pointer allocate(size_type count)
	{
		return reinterpret_cast<pointer>(this->alloc(
			static_cast<uint32_t>(count * sizeof(value_type)),
			static_cast<uint32_t>(alignof(value_type))
		));
	}

	void deallocate(pointer ptr, size_type)
	{
		this->free(reinterpret_cast<std::byte*>(ptr));
	}
};

template <mtp::cfg::IsAllocatorConfig Config>
class Allocator<Config, StdAdapter, void> : public AllocatorCore<Config>
{
public:

	using value_type = void;

	using is_always_equal = std::true_type;
	using propagate_on_container_move_assignment = std::true_type;
	using propagate_on_container_copy_assignment = std::true_type;
	using propagate_on_container_swap = std::true_type;

	using AllocatorCore<Config>::AllocatorCore;

	Allocator() = delete;
	~Allocator() = default;

	Allocator(const Allocator&) noexcept = default;
	Allocator(Allocator&&) noexcept = default;
	Allocator& operator=(const Allocator&) noexcept = default;
	Allocator& operator=(Allocator&&) noexcept = default;

	template <typename Rebound>
	Allocator(const Allocator<Config, StdAdapter, Rebound>& other) noexcept
		: AllocatorCore<Config>(static_cast<const AllocatorCore<Config>&>(other))
	{}

	template <typename Rebound>
	bool operator==(const Allocator<Config, StdAdapter, Rebound>&) const noexcept
	{
		return true;
	}

	template <typename Rebound>
	bool operator!=(const Allocator<Config, StdAdapter, Rebound>&) const noexcept
	{
		return false;
	}

	template <typename Rebound>
	struct rebind
	{
		using other = Allocator<Config, StdAdapter, Rebound>;
	};

	template <typename ObjType>
	using rebind_t = Allocator<Config, StdAdapter, ObjType>;
};


template <mtp::cfg::IsAllocatorConfig Config>
class Allocator<Config, PmrAdapter> : public AllocatorCore<Config>, public std::pmr::memory_resource
{
public:

	using AllocatorCore<Config>::AllocatorCore;

protected:

	void* do_allocate(size_t count, size_t alignment) override
	{
		return this->alloc(static_cast<uint32_t>(count), static_cast<uint32_t>(alignment));
	}

	void do_deallocate(void* ptr, size_t, size_t) override
	{
		this->free(reinterpret_cast<std::byte*>(ptr));
	}

	bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
	{
		return this == &other;
	}
};


template <mtp::cfg::IsAllocatorConfig Config>
AllocatorCore(const AllocatorCore<Config>&) -> AllocatorCore<Config>;

} // mtp::core

