#pragma once

#include <ostream>
#include <vector>
#include <cstring>
#include <utility>
#include <iostream>
#include <memory_resource>

#include "memory_model.hpp"



class Benchmark
{
public:

	virtual ~Benchmark()
	{}

private:

	static inline auto& adapter_simple_std()
	{
		return hpr::MemoryModel::get_allocator <
			hpr::mem::AllocatorType::simple,
			hpr::mem::AllocatorInterface::std_adapter
		>();
	}

	static inline auto& adapter_simple_pmr()
	{
		return hpr::MemoryModel::get_allocator <
			hpr::mem::AllocatorType::simple,
			hpr::mem::AllocatorInterface::pmr_adapter
		>();
	}

	using AdapterSimpleStdType = std::remove_reference_t<decltype(
		hpr::MemoryModel::get_allocator<
			hpr::mem::AllocatorType::simple,
			hpr::mem::AllocatorInterface::std_adapter
		>()
	)>;

	template <typename T>
	using AdapterStd = std::allocator_traits<AdapterSimpleStdType>::template rebind_alloc<T>;


	inline void test_basic_allocation()
	{
		std::cout << "testing basic allocation..." << std::flush;

		auto& adapter_std = adapter_simple_std();

		std::byte* ptr = adapter_std.allocate<std::byte>(100);
		assert(ptr != nullptr);
		std::memset(ptr, 0xAB, 100);
		adapter_std.deallocate<std::byte>(ptr, 100);

		std::cout << "OK" << std::endl;
	}

	inline void test_alignment()
	{
		std::cout << "testing alignment..." << std::flush;

		auto& adapter_pmr = adapter_simple_pmr();

		for (size_t alignment : {1, 2, 4, 8, 16, 32, 64}) {
			void* ptr = adapter_pmr.allocate(64, alignment);
			assert(ptr != nullptr);
			assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
			adapter_pmr.deallocate(ptr, 64, alignment);
		}

		std::cout << "OK" << std::endl;
	}

	inline void test_multiple_allocations()
	{
		std::cout << "testing multiple allocations..." << std::flush;

		auto& adapter_std = adapter_simple_std();

		std::vector<std::byte*> blocks;
		std::size_t sizes[] = {8, 16, 32, 64, 128, 256};

		for (std::size_t size : sizes) {
			std::byte* ptr = adapter_std.allocate<std::byte>(size);
			assert(ptr != nullptr);
			blocks.push_back(ptr);
		}

		for (std::byte* ptr : blocks) {
			adapter_std.deallocate<std::byte>(ptr, 0);
		}

		std::cout << "OK" << std::endl;
	}

	inline void test_with_containers()
	{
		std::cout << "testing with containers..." << std::flush;

		auto& adapter_std = adapter_simple_std();

		std::vector<int, AdapterStd<int>> vec_std(adapter_std);
		std::basic_string <
			char,
			std::char_traits<char>,
			AdapterStd<char>
		> str_std(adapter_std);
	
		for (int i = 0; i < 1000; ++i)
			vec_std.push_back(i);
	
		for (int i = 0; i < 1000; ++i)
			str_std += 'a';
	
		vec_std.clear();
		str_std.clear();

		auto& adapter_pmr = adapter_simple_pmr();

		std::pmr::polymorphic_allocator<int> int_alloc(&adapter_pmr);
		std::pmr::polymorphic_allocator<char> char_alloc(&adapter_pmr);

		std::pmr::vector<int> vec_pmr(int_alloc);
		std::pmr::string str_pmr(char_alloc);

		for (int i = 0; i < 1000; ++i)
			vec_pmr.push_back(i);

		for (int i = 0; i < 1000; ++i)
			str_pmr += 'a';

		vec_pmr.clear();
		str_pmr.clear();

		std::cout << "OK" << std::endl;
	}

public:

	inline void basic_tests()
	{
		test_basic_allocation();
		test_alignment();
		test_multiple_allocations();
		test_with_containers();

		std::cout << std::endl;
	}

	virtual void setup() = 0;
	virtual void run() = 0;
	virtual void teardown() = 0;
};
