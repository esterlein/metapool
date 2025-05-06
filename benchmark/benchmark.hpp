#pragma once

#include <ostream>
#include <vector>
#include <cstring>
#include <iostream>
#include <memory_resource>

#include "memory_api.hpp"
#include "container_factory.hpp"



class Benchmark
{
public:

	virtual ~Benchmark()
	{}

	using System = hpr::mem::BenchmarkSimpleSystem;

private:

	inline void test_basic_allocation()
	{
		std::cout << "testing basic allocation..." << std::flush;

		auto& allocator = hpr::mem::get_system_allocator<System>();

		std::byte* ptr = allocator.alloc(100, alignof(int));
		assert(ptr != nullptr);
		std::memset(ptr, 0xAB, 100);
		allocator.free(ptr);

		std::cout << "ok" << std::endl;
	}

	inline void test_alignment()
	{
		std::cout << "testing alignment..." << std::flush;

		auto& allocator = hpr::mem::get_system_allocator<System>();

		for (size_t alignment : {1, 2, 4, 8, 16, 32, 64}) {
			std::byte* ptr = allocator.alloc(64, alignment);
			assert(ptr != nullptr);
			assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
			allocator.free(ptr);
		}

		std::cout << "ok" << std::endl;
	}

	inline void test_multiple_allocations()
	{
		std::cout << "testing multiple allocations..." << std::flush;

		auto& allocator = hpr::mem::get_system_allocator<System>();

		std::vector<std::byte*> vec;
		std::array<std::size_t, 6> sizes {8, 16, 32, 64, 128, 256};

		for (std::size_t size : sizes) {
			std::byte* ptr = allocator.alloc(size, alignof(int));
			assert(ptr != nullptr);
			vec.push_back(ptr);
		}

		for (std::byte* ptr : vec) {
			allocator.free(ptr);
		}

		std::cout << "ok" << std::endl;
	}

	inline void test_with_containers()
	{
		std::cout << "testing containers..." << std::flush;

		auto vec_std = hpr::cntr::make_vector<int, System>();
		vec_std.reserve(1000);

		for (int i = 0; i < 1000; ++i)
			vec_std.push_back(i);
	
		auto str_std = hpr::cntr::make_string<System>();

		for (int i = 0; i < 1000; ++i)
			str_std += 'a';
	
		vec_std.clear();
		str_std.clear();

		auto vec_pmr = hpr::cntr::make_pmr_vector<int, System>();

		for (int i = 0; i < 1000; ++i)
			vec_pmr.push_back(i);

		auto str_pmr = hpr::cntr::make_pmr_string<System>();

		for (int i = 0; i < 1000; ++i)
			str_pmr += 'a';

		vec_pmr.clear();
		str_pmr.clear();

		std::cout << "ok" << std::endl;
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
