#pragma once

#if (defined(MTP_CONTAINERS_MTP) + defined(MTP_CONTAINERS_STD) + defined(MTP_CONTAINERS_BOTH) + defined(MTP_CONTAINERS_NONE)) > 1
#error "mtp: define only one of MTP_CONTAINERS_MTP / MTP_CONTAINERS_STD / MTP_CONTAINERS_BOTH / MTP_CONTAINERS_NONE"
#endif
#if !defined(MTP_CONTAINERS_MTP) && !defined(MTP_CONTAINERS_STD) && !defined(MTP_CONTAINERS_BOTH) && !defined(MTP_CONTAINERS_NONE)
#define MTP_CONTAINERS_NONE 1
#endif
#if defined(MTP_CONTAINERS_MTP) || defined(MTP_CONTAINERS_BOTH)
#define MTP_ENABLE_MTP_CONTAINERS 1
#endif
#if defined(MTP_CONTAINERS_STD) || defined(MTP_CONTAINERS_BOTH)
#define MTP_ENABLE_STD_CONTAINERS 1
#endif

#include "mtp/mtpint.hpp"

#include <string_view>

#include "mtp/metaset.hpp"
#include "mtp/metapool.hpp"
#include "mtp/alloc_tracer.hpp"
#include "mtp/memory_model.hpp"


#if defined(MTP_ENABLE_MTP_CONTAINERS)


#include "container/container_factory.hpp"


#endif


namespace mtp {


template <typename Set>
using shared = core::MemoryModel::Shared<Set, cfg::AllocatorTag::std_adapter>;


using default_set = metaset <

	def<capf::mul2, 512,      32,       32,      512,  2016>,
	def<capf::mul2, 128,     512,     2048,     8192, 32256>,
	def<capf::flat,  64,    8192,    32768,   122880>,
	def<capf::flat,  64,  131072,   131072,   917504>,
	def<capf::flat,  32,  262144,  1048576,  8388608>,
	def<capf::flat,  32,       8, 16777216, 16777216>,
	def<capf::flat,  48,       8, 33554432, 33554432>
>;


template <typename Set>
static inline void init_tls()
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
static inline auto& get_tls_allocator(as_ref_t)
{
	return core::MemoryModel::create_thread_local_allocator <
		Set,
		cfg::AllocatorTag::std_adapter
	>();
}

template <typename Set>
static inline auto* get_tls_allocator(as_ptr_t)
{
	return &core::MemoryModel::create_thread_local_allocator <
		Set,
		cfg::AllocatorTag::std_adapter
	>();
}

template <typename Set>
static inline auto& get_tls_allocator()
{
	return get_tls_allocator<Set>(as_ref);
}


static inline void export_trace(std::string_view filename, bool clear = false)
{
	cfg::AllocTracer::export_trace(filename, clear);
}


#if defined(MTP_ENABLE_MTP_CONTAINERS)


template <typename T, typename Set>
using vault = cntr::vault<T, Set>;

template <typename T, typename Set>
using slag = cntr::slag<T, Set>;


#endif


#if defined(MTP_ENABLE_STD_CONTAINERS)


template <typename T, typename Set, typename... Types>
inline auto make_vector(Types&&... args)
{
	return cntr::make_vector<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_vector(shared<Set>& shared, Types&&... args)
{
	return cntr::make_vector<T, Set>(shared, std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_deque(Types&&... args)
{
	return cntr::make_deque<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_deque(shared<Set>& shared, Types&&... args)
{
	return cntr::make_deque<T, Set>(shared, std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_list(Types&&... args)
{
	return cntr::make_list<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_list(shared<Set>& shared, Types&&... args)
{
	return cntr::make_list<T, Set>(shared, std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_forward_list(Types&&... args)
{
	return cntr::make_forward_list<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_forward_list(shared<Set>& shared, Types&&... args)
{
	return cntr::make_forward_list<T, Set>(shared, std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_set(Types&&... args)
{
	return cntr::make_set<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_set(shared<Set>& shared, Types&&... args)
{
	return cntr::make_set<T, Set>(shared, std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_unordered_set(Types&&... args)
{
	return cntr::make_unordered_set<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_unordered_set(shared<Set>& shared, Types&&... args)
{
	return cntr::make_unordered_set<T, Set>(shared, std::forward<Types>(args)...);
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_map(Types&&... args)
{
	return cntr::make_map<K, V, Set>(std::forward<Types>(args)...);
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_map(shared<Set>& shared, Types&&... args)
{
	return cntr::make_map<K, V, Set>(shared, std::forward<Types>(args)...);
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_unordered_map(Types&&... args)
{
	return cntr::make_unordered_map<K, V, Set>(std::forward<Types>(args)...);
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_unordered_map(shared<Set>& shared, Types&&... args)
{
	return cntr::make_unordered_map<K, V, Set>(shared, std::forward<Types>(args)...);
}

template <typename Set, typename... Types>
inline auto make_string(Types&&... args)
{
	return cntr::make_string<Set>(std::forward<Types>(args)...);
}

template <typename Set, typename... Types>
inline auto make_string(shared<Set>& shared, Types&&... args)
{
	return cntr::make_string<Set>(shared, std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_unique(Types&&... args)
{
	return cntr::make_unique<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_unique(shared<Set>& shared, Types&&... args)
{
	return cntr::make_unique<T, Set>(shared, std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_shared(Types&&... args)
{
	return cntr::make_shared<T, Set>(std::forward<Types>(args)...);
}

template <typename T, typename Set, typename... Types>
inline auto make_shared(shared<Set>& shared, Types&&... args)
{
	return cntr::make_shared<T, Set>(shared, std::forward<Types>(args)...);
}


#endif

} // mtp

