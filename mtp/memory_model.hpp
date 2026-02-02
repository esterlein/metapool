#pragma once

#include "mtpint.hpp"

#include <span>
#include <tuple>
#include <array>
#include <memory>

#include "allocator.hpp"
#include "metaset.hpp"
#include "monotonic_arena.hpp"

#include "fail.hpp"


namespace mtp::cfg {


	enum class AllocatorTag {
		native,
		std_adapter,
		pmr_adapter
	};

} // mtp::cfg


namespace mtp::core {


class MemoryModel final
{
public:

	MemoryModel() = delete;


	template <typename Set, mtp::cfg::AllocatorTag Tag>
	static inline auto& create_thread_local_allocator()
	{
		thread_local static MonotonicArena arena {
			Set::arena_size,
			mtp::cfg::arena_alignment
		};

		thread_local static MetapoolContainer<Set> container {&arena};

		thread_local static std::array<std::byte, k_proxy_buffer_bytes<Set>> proxy_buffer {};

		thread_local static auto proxies = setup_proxy_span<Set>(container, proxy_buffer);

		constexpr auto allocator_config = Set::create_allocator_config();

		if constexpr (Tag == mtp::cfg::AllocatorTag::native) {
			thread_local static Allocator<decltype(allocator_config), Native> allocator {proxies};
			return allocator;
		}
		else if constexpr (Tag == mtp::cfg::AllocatorTag::std_adapter) {
			thread_local static Allocator<decltype(allocator_config), StdAdapter, void> allocator {proxies};
			return allocator;
		}
		else if constexpr (Tag == mtp::cfg::AllocatorTag::pmr_adapter) {
			thread_local static Allocator<decltype(allocator_config), PmrAdapter> allocator {proxies};
			return allocator;
		}
	}

private:

	template <typename Set>
	class MetapoolContainer
	{
	public:

		explicit MetapoolContainer(MonotonicArena* upstream)
			: m_metapool_storage
		{
			create_storage(
				upstream,
				Set::create_allocator_config(),
				std::make_index_sequence<Set::set_size>{}
			)
		}
		{}

	private:

		template <typename Config, size_t... Is>
		static auto create_storage(
			MonotonicArena* upstream,
			const Config& config,
			std::index_sequence<Is...>
		)
		{
			return std::make_tuple(
				std::tuple_element_t<Is, typename Set::TupleType>(
					upstream,
					config.range_metadata[Set::sorted_index_map[Is]].base_proxy_index
				)...
			);
		}

	public:

		std::span<FreelistProxy> make_proxies(void* raw_buffer)
		{
			constexpr auto metadata = Set::create_allocator_config().range_metadata;

			constexpr size_t total_stride_count = [] (const auto& metadata_ref) {
				size_t sum = 0;
				for (const auto& entry : metadata_ref)
					sum += entry.stride_count;
				return sum;
			}(metadata);

			auto* proxy_ptr = reinterpret_cast<FreelistProxy*>(raw_buffer);

			size_t offset = 0;

			const auto fill_proxies = []<size_t... Is>(
				std::index_sequence<Is...>,
				std::tuple<std::tuple_element_t<Is, typename Set::TupleType>...>& pools,
				FreelistProxy* out_ptr,
				size_t& base_offset,
				const std::array<mtp::cfg::RangeMetadata, Set::set_size>& metadata
			) {
				(..., (
					std::get<Is>(pools).make_freelist_proxies(
						reinterpret_cast<FreelistProxy*>(
							reinterpret_cast<std::byte*>(out_ptr) + base_offset
						)
					),
					base_offset += metadata[Is].stride_count * sizeof(FreelistProxy)
				));
			};

			fill_proxies(std::make_index_sequence<Set::set_size>{}, m_metapool_storage, proxy_ptr, offset, metadata);

			return std::span<FreelistProxy>{proxy_ptr, total_stride_count};
		}

	private:

		typename Set::TupleType m_metapool_storage;

	}; // MetapoolContainer<Set>

private:

	template <typename Set>
	static constexpr size_t k_proxy_bytes =
		sizeof(FreelistProxy) * Set::create_allocator_config().total_stride_count;

	template <typename Set>
	static constexpr size_t k_proxy_buffer_bytes =
		alignof(FreelistProxy) + k_proxy_bytes<Set>;

public:

	template <typename Set, mtp::cfg::AllocatorTag Tag = mtp::cfg::AllocatorTag::std_adapter>
	class Shared final
	{
	public:

		Shared()
			: m_arena     {Set::arena_size, mtp::cfg::arena_alignment}
			, m_container {&m_arena}
			, m_proxies   {setup_proxy_span<Set>(m_container, m_proxy_buffer)}
			, m_allocator {m_proxies}
		{}

		Shared(const Shared&) = delete;
		Shared& operator=(const Shared&) = delete;

		Shared(Shared&&) = delete;
		Shared& operator=(Shared&&) = delete;

		[[nodiscard]] auto& get() noexcept
		{
			return m_allocator;
		}

		[[nodiscard]] auto* get_ptr() noexcept
		{
			return &m_allocator;
		}

	private:

		using allocator_config_t = decltype(Set::create_allocator_config());

		using allocator_t =
			std::conditional_t <
				Tag == mtp::cfg::AllocatorTag::native,
				Allocator<allocator_config_t, Native>,
				std::conditional_t <
					Tag == mtp::cfg::AllocatorTag::std_adapter,
					Allocator<allocator_config_t, StdAdapter, void>,
					Allocator<allocator_config_t, PmrAdapter>
				>
			>;

	private:

		MonotonicArena m_arena;

		MetapoolContainer<Set> m_container;

		std::array<std::byte, k_proxy_buffer_bytes<Set>> m_proxy_buffer {};

		std::span<FreelistProxy> m_proxies;

		allocator_t m_allocator;
	};

public:

	template <typename T, typename Set>
	static inline auto get_std_adapter()
	{
		auto& base = create_thread_local_allocator<Set, mtp::cfg::AllocatorTag::std_adapter>();
		return typename std::remove_reference_t<decltype(base)>::template rebind_t<T>{base};
	}

	template <typename T, typename Set>
	static inline auto get_std_adapter(
		mtp::core::MemoryModel::Shared<Set,
		mtp::cfg::AllocatorTag::std_adapter>& shared
	)
	{
		auto& base = shared.get();
		return typename std::remove_reference_t<decltype(base)>::template rebind_t<T>{base};
	}

	template <typename T, typename Set>
	static inline auto get_pmr_adapter()
	{
		auto& base = create_thread_local_allocator<Set, mtp::cfg::AllocatorTag::pmr_adapter>();
		return std::pmr::polymorphic_allocator<T>{static_cast<std::pmr::memory_resource*>(&base)};
	}

	template <typename T, typename Set>
	static inline auto get_pmr_adapter(
		mtp::core::MemoryModel::Shared<Set,
		mtp::cfg::AllocatorTag::pmr_adapter>& shared
	)
	{
		auto& base = shared.get();
		return std::pmr::polymorphic_allocator<T>{
			static_cast<std::pmr::memory_resource*>(&base)
		};
	}

private:

	template <typename Set, typename ProxyBuffer>
	[[nodiscard]] static auto setup_proxy_span(MetapoolContainer<Set>& container, ProxyBuffer& proxy_buffer)
	{
		void* buffer_ptr    = static_cast<void*>(proxy_buffer.data());
		size_t buffer_bytes = proxy_buffer.size();

		void* aligned = std::align(
			alignof(FreelistProxy),
			k_proxy_bytes<Set>,
			buffer_ptr,
			buffer_bytes
		);

		MTP_ASSERT(aligned != nullptr,
			mtp::err::mem_model_proxy_align_fail);

		auto* first_proxy_ptr = reinterpret_cast<FreelistProxy*>(aligned);
		return container.make_proxies(first_proxy_ptr);
	}

}; // MemoryModel

} // mtp::core


namespace mtp::cfg {


	template <typename Set>
	using alloc_for = std::remove_reference_t<decltype(
		mtp::core::MemoryModel::create_thread_local_allocator<Set, AllocatorTag::std_adapter>()
	)>;

	template <typename T, typename Set>
	using rebound_std_adapter = typename alloc_for<Set>::template rebind_t<T>;

} // mtp::cfg

