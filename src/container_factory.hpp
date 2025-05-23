#pragma once

#include "mtp_memory.hpp"


namespace hpr::cntr {


template <typename T, typename System, typename... Types>
auto make_vector(Types&&... args)
{
	return std::vector<T, mem::SysAdapter<T, System>> {
		std::forward<Types>(args)...,
		mem::get_adapter_std<T, System>()
	};
}

template <typename System, typename... Types>
auto make_string(Types&&... args)
{
	return std::basic_string <
		char,
		std::char_traits<char>,
		mem::SysAdapter<char, System>> {
			std::forward<Types>(args)...,
			mem::get_adapter_std<char, System>()
	};
}


template <typename T, typename System, typename... Types>
auto make_pmr_vector(Types&&... args)
{
	return std::pmr::vector<T> {
		std::forward<Types>(args)...,
		std::pmr::polymorphic_allocator<T> {
			mem::get_adapter_pmr<System>()
		}
	};
}

template <typename System, typename... Types>
auto make_pmr_string(Types&&... args)
{
	return std::pmr::string {
		std::forward<Types>(args)...,
		std::pmr::polymorphic_allocator<char> {
			mem::get_adapter_pmr<System>()
		}
	};
}
} // hpr::cntr
