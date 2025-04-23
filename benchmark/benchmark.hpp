#pragma once

#include <vector>
#include <cstring>
#include <utility>
#include <iostream>
#include <memory_resource>

#include "memory_model.hpp"



class Benchmark
{
public:

	virtual ~Benchmark() {}

	inline void test_basic_allocation()
	{
		auto& allocator = hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Standard>();

		void* ptr = allocator.allocate(100, 8);
		assert(ptr != nullptr);
		std::memset(ptr, 0xAB, 100);
		allocator.deallocate(ptr, 100, 8);
	}

	inline void test_alignment()
	{
		auto& allocator = hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Standard>();

		for (size_t alignment : {1, 2, 4, 8, 16, 32, 64}) {
			void* ptr = allocator.allocate(64, alignment);
			assert(ptr != nullptr);
			assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
			allocator.deallocate(ptr, 64, alignment);
		}
	}

	inline void test_multiple_allocations()
	{
		auto& allocator = hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Standard>();

		std::vector<std::pair<void*, size_t>> blocks;
		for (size_t size : {8, 16, 32, 64, 128}) {
			void* ptr = allocator.allocate(size, 8);
			assert(ptr != nullptr);
			blocks.push_back({ptr, size});
		}

		while (!blocks.empty()) {
			auto [ptr, size] = blocks.back();
			allocator.deallocate(ptr, size, 8);
			blocks.pop_back();
		}
	}

	inline void test_with_containers()
	{
		auto& allocator = hpr::MemoryModel::get_allocator<hpr::mem::AllocatorType::Standard>();

//		std::pmr::vector<int> vec(&allocator);
		std::pmr::string str(&allocator);
/*
		for (int i = 0; i < 2; ++i)
			vec.push_back(i);
*/
		for (int i = 0; i < 1; ++i)
			str += "a";

//		vec.clear();
		str.clear();
	}

	inline void basic_tests()
	{
		std::cout << "testing basic allocation... ";
		test_basic_allocation();
		std::cout << "OK\n";
		std::cout << "testing alignment... ";
		test_alignment();
		std::cout << "OK\n";
		std::cout << "testing multiple allocations... ";
		test_multiple_allocations();
		std::cout << "OK\n";
		std::cout << "testing with containers... ";
		test_with_containers();
		std::cout << "OK\n";
	}

	virtual void setup() = 0;
	virtual void run() = 0;
	virtual void teardown() = 0;
};
