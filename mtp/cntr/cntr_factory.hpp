#pragma once

#include <map>
#include <set>
#include <list>
#include <deque>
#include <vector>
#include <string>
#include <memory>
#include <forward_list>
#include <unordered_map>
#include <unordered_set>

#include "vault.hpp"
#include "slag.hpp"

#include "../memory_model.hpp"


namespace mtp::cntr {


template <typename T, typename Set>
inline auto make_vault()
{
	return vault<T, Set>();
}

template <typename T, typename Set>
inline auto make_vault(std::size_t capacity)
{
	return vault<T, Set>(capacity);
}

template <typename T, typename Set, typename... Types>
inline auto make_vault(std::size_t count, Types&&... args)
{
	return vault<T, Set>(count, std::forward<Types>(args)...);
}

template <typename T, typename Set>
inline auto make_slag(std::size_t capacity)
{
	return slag<T, Set>(capacity);
}

template <typename T, typename Set, typename... Types>
inline auto make_slag(std::size_t count, Types&&... args)
{
	return slag<T, Set>(count, std::forward<Types>(args)...);
}


template <typename T, typename Set, typename... Types>
inline auto make_vector(Types&&... args)
{
	return std::vector<T, mtp::cfg::rebound_std_adapter<T, Set>>{
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_deque(Types&&... args)
{
	return std::deque<T, mtp::cfg::rebound_std_adapter<T, Set>>{
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_list(Types&&... args)
{
	return std::list<T, mtp::cfg::rebound_std_adapter<T, Set>>{
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_forward_list(Types&&... args)
{
	return std::forward_list<T, mtp::cfg::rebound_std_adapter<T, Set>>{
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_set(Types&&... args)
{
	return std::set<T, std::less<T>, mtp::cfg::rebound_std_adapter<T, Set>>{
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_unordered_set(Types&&... args)
{
	return std::unordered_set<T, std::hash<T>, std::equal_to<T>, mtp::cfg::rebound_std_adapter<T, Set>>{
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<T, Set>()
	};
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_map(Types&&... args)
{
	using Pair = std::pair<const K, V>;
	return std::map<K, V, std::less<K>, mtp::cfg::rebound_std_adapter<Pair, Set>>{
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<Pair, Set>()
	};
}

template <typename K, typename V, typename Set, typename... Types>
inline auto make_unordered_map(Types&&... args)
{
	using Pair = std::pair<const K, V>;
	return std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, mtp::cfg::rebound_std_adapter<Pair, Set>>{
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<Pair, Set>()
	};
}

template <typename Set, typename... Types>
inline auto make_string(Types&&... args)
{
	return std::basic_string<char, std::char_traits<char>, mtp::cfg::rebound_std_adapter<char, Set>>{
		std::forward<Types>(args)...,
		mtp::core::MemoryModel::get_std_adapter<char, Set>()
	};
}

template <typename T, typename Set, typename... Types>
inline auto make_unique(Types&&... args)
{
	auto& pool = mtp::core::MemoryModel::create_thread_local_allocator <
		Set,
		mtp::cfg::AllocatorTag::std_adapter
	>();
	T* ptr = pool.template construct<T>(std::forward<Types>(args)...);
	return std::unique_ptr<T, void(*)(T*)> {
		ptr,
		[](T* p) {
			auto& local_pool = mtp::core::MemoryModel::create_thread_local_allocator <
				Set,
				mtp::cfg::AllocatorTag::std_adapter
			>();
			local_pool.template destruct<T>(p);
		}
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

template <typename T, typename Set>
inline auto make_pmr_vector()
{
	auto alloc = mtp::core::MemoryModel::get_pmr_adapter<T, Set>();
	return std::pmr::vector<T>{ alloc };
}

template <typename Set>
inline auto make_pmr_string()
{
	auto alloc = mtp::core::MemoryModel::get_pmr_adapter<char, Set>();
	return std::pmr::string{ alloc };
}

} // mtp::cntr

