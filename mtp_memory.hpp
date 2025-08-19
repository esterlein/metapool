#pragma once

#include <string_view>

#include "mtp/metapool.hpp"
#include "mtp/alloc_tracer.hpp"
#include "mtp/memory_model.hpp"
#include "mtp/cntr/cntr_factory.hpp"

#include "mtp_setup.hpp"


namespace mtp {


template <typename T, typename Set>
using vault = cntr::vault<T, Set>;

template <typename T, typename Set>
using slag = cntr::slag<T, Set>;


template <typename Set>
static inline void init()
{
	(void) core::MemoryModel::create_thread_local_allocator <
		Set,
		cfg::AllocatorTag::std_adapter
	>();
}


struct as_ref_t { constexpr as_ref_t() noexcept = default; };
struct as_ptr_t { constexpr as_ptr_t() noexcept = default; };

inline constexpr as_ref_t as_ref {};
inline constexpr as_ptr_t as_ptr {};


template <typename Set>
static inline auto& get_allocator(as_ref_t)
{
	return core::MemoryModel::create_thread_local_allocator <
		Set,
		cfg::AllocatorTag::std_adapter
	>();
}

template <typename Set>
static inline auto* get_allocator(as_ptr_t)
{
	return core::MemoryModel::create_thread_local_allocator <
		Set,
		cfg::AllocatorTag::std_adapter
	>();
}

template <typename Set>
static inline auto& get_allocator()
{
	return get_allocator<Set>(as_ref);
}


static inline void export_trace(std::string_view filename, bool clear = false)
{
	cfg::AllocTracer::export_trace(filename, clear);
}


template <typename T, typename Set>
inline auto make_vault()
{
	return cntr::make_vault<T, Set>();
}

template <typename T, typename Set>
inline auto make_vault(std::size_t capacity)
{
	return cntr::make_vault<T, Set>(capacity);
}

template <typename T, typename Set, typename... Types>
inline auto make_vault(std::size_t count, Types&&... args)
{
	return cntr::make_vault<T, Set>(count, std::forward<Types>(args)...);
}

template <typename T, typename Set>
inline auto make_slag(std::size_t capacity)
{
	return cntr::make_slag<T, Set>(capacity);
}

template <typename T, typename Set, typename... Types>
inline auto make_slag(std::size_t count, Types&&... args)
{
	return cntr::make_slag<T, Set>(count, std::forward<Types>(args)...);
}


template <typename T, typename Set, typename... Types>
inline auto make_vector(Types&&... args)
{
	return cntr::make_vector<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_deque(Types&&... args)
{
	return cntr::make_deque<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_list(Types&&... args)
{
	return cntr::make_list<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_forward_list(Types&&... args)
{
	return cntr::make_forward_list<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_set(Types&&... args)
{
	return cntr::make_set<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_unordered_set(Types&&... args)
{
	return cntr::make_unordered_set<T, Set>(std::forward<Types>(args)...);
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_map(Types&&... args)
{
	return cntr::make_map<K, V, Set>(std::forward<Types>(args)...);
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_unordered_map(Types&&... args)
{
	return cntr::make_unordered_map<K, V, Set>(std::forward<Types>(args)...);
}

template <typename Set, typename... Types>
inline auto make_string(Types&&... args)
{
	return cntr::make_string<Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_unique(Types&&... args)
{
	return cntr::make_unique<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_shared(Types&&... args)
{
	return cntr::make_shared<T, Set>(std::forward<Types>(args)...);
}

} // mtp
