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

#include "../mtp/mtpint.hpp"
#include "../mtp/memory_model.hpp"


#if defined(MTP_ENABLE_MTP_CONTAINERS)

	#include "vault.hpp"
	#include "slag.hpp"

#endif

#if defined(MTP_ENABLE_STD_CONTAINERS)

	#include <map>
	#include <set>
	#include <list>
	#include <deque>
	#include <vector>
	#include <string>
	#include <memory>
	#include <type_traits>
	#include <forward_list>
	#include <unordered_map>
	#include <unordered_set>


namespace mtp::cntr {


template <typename T, typename Set, typename... Types>
inline auto make_vector(Types&&... args)
{
	return std::vector<T, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_vector(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	return std::vector<T, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>(shared)
	};
}


template <typename T, typename Set, typename... Types>
inline auto make_deque(Types&&... args)
{
	return std::deque<T, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_deque(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	return std::deque<T, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>(shared)
	};
}


template <typename T, typename Set, typename... Types>
inline auto make_list(Types&&... args)
{
	return std::list<T, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_list(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	return std::list<T, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>(shared)
	};
}


template <typename T, typename Set, typename... Types>
inline auto make_forward_list(Types&&... args)
{
	return std::forward_list<T, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_forward_list(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	return std::forward_list<T, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>(shared)
	};
}


template <typename T, typename Set, typename... Types>
inline auto make_set(Types&&... args)
{
	return std::set<T, std::less<T>, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_set(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	return std::set<T, std::less<T>, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>(shared)
	};
}


template <typename T, typename Set, typename... Types>
inline auto make_unordered_set(Types&&... args)
{
	return std::unordered_set<T, std::hash<T>, std::equal_to<T>, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_unordered_set(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	return std::unordered_set<T, std::hash<T>, std::equal_to<T>, mtp::cfg::rebound_std_adapter<T, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>(shared)
	};
}


template <typename K, typename V, typename Set, typename... Types>
inline auto make_map(Types&&... args)
{
	using Pair = std::pair<const K, V>;
	return std::map<K, V, std::less<K>, mtp::cfg::rebound_std_adapter<Pair, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<Pair, Set>()
	};
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_map(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	using Pair = std::pair<const K, V>;
	return std::map<K, V, std::less<K>, mtp::cfg::rebound_std_adapter<Pair, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<Pair, Set>(shared)
	};
}


template <typename K, typename V, typename Set, typename... Types>
inline auto make_unordered_map(Types&&... args)
{
	using Pair = std::pair<const K, V>;
	return std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, mtp::cfg::rebound_std_adapter<Pair, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<Pair, Set>()
	};
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_unordered_map(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	using Pair = std::pair<const K, V>;
	return std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, mtp::cfg::rebound_std_adapter<Pair, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<Pair, Set>(shared)
	};
}


template <typename Set, typename... Types>
inline auto make_string(Types&&... args)
{
	return std::basic_string<char, std::char_traits<char>, mtp::cfg::rebound_std_adapter<char, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<char, Set>()
	};
}

template <typename Set, typename... Types>
inline auto make_string(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	return std::basic_string<char, std::char_traits<char>, mtp::cfg::rebound_std_adapter<char, Set>> {
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<char, Set>(shared)
	};
}


template <typename T, typename Set, typename... Types>
inline auto make_unique(Types&&... args)
{
	auto& metapool = mtp::core::MemoryModel::create_thread_local_allocator <
		Set,
		mtp::cfg::AllocatorTag::std_adapter
	>();
	T* object_ptr = metapool.template construct<T>(std::forward<Types>(args)...);
	return std::unique_ptr<T, void(*)(T*)> {
		object_ptr,
		[](T* ptr) {
			auto& metapool = mtp::core::MemoryModel::create_thread_local_allocator <
				Set,
				mtp::cfg::AllocatorTag::std_adapter
			>();
			metapool.template destruct<T>(ptr);
		}
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_unique(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	auto& metapool = shared.get();

	T* object_ptr = metapool.template construct<T>(std::forward<Types>(args)...);

	struct deleter final
	{
		std::remove_reference_t<decltype(metapool)>* m_metapool {};

		void operator()(T* ptr) const noexcept
		{
			m_metapool->template destruct<T>(ptr);
		}
	};

	return std::unique_ptr<T, deleter> {
		object_ptr,
		deleter {&metapool}
	};
}


template <typename T, typename Set, typename... Types>
inline auto make_shared(Types&&... args)
{
	return std::allocate_shared<T>(
		mtp::core::MemoryModel::get_std_adapter<T, Set>(),
		std::forward<Types>(args)...
	);
}

template <typename T, typename Set, typename... Types>
inline auto make_shared(
	mtp::core::MemoryModel::Shared<Set, mtp::cfg::AllocatorTag::std_adapter>& shared,
	Types&&... args
)
{
	return std::allocate_shared<T>(
		mtp::core::MemoryModel::get_std_adapter<T, Set>(shared),
		std::forward<Types>(args)...
	);
}


#endif

} // mtp::cntr

