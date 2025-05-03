#pragma once

#include "memory_api.hpp"


namespace hpr::cntr {


template <typename T, typename System>
auto make_vector()
{
	return std::vector<T, mem::SysAdapter<T, System>> {
		mem::get_adapter_std<T, System>()
	};
}

template <typename System>
auto make_string()
{
	return std::basic_string <
		char,
		std::char_traits<char>,
		mem::SysAdapter<char, System>> {
			mem::get_adapter_std<char, System>()
		};
}


template <typename T, typename System>
auto make_pmr_vector()
{
	return std::pmr::vector<T> {
		std::pmr::polymorphic_allocator<T> {
			mem::get_adapter_pmr<System>()
		}
	};
}

template <typename System>
auto make_pmr_string()
{
	return std::pmr::string {
		std::pmr::polymorphic_allocator<char> {
			mem::get_adapter_pmr<System>()
		}
	};
}
} // hpr::cntr
