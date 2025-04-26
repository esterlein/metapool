#pragma once

#include <ostream>
#include <vector>
#include <cstring>
#include <utility>
#include <iostream>

#include "memory_model.hpp"



class Benchmark
{
public:

	virtual ~Benchmark() {}

	template <hpr::mem::AllocatorType Type>
	inline void test_basic_allocation()
	{
		auto& allocator = hpr::MemoryModel::get_allocator<Type>();

		void* ptr = allocator.allocate(100, 8);
		assert(ptr != nullptr);
		std::memset(ptr, 0xAB, 100);
		allocator.deallocate(ptr, 100, 8);
	}

	template <hpr::mem::AllocatorType Type>
	inline void test_alignment()
	{
		auto& allocator = hpr::MemoryModel::get_allocator<Type>();

		for (size_t alignment : {1, 2, 4, 8, 16, 32, 64}) {
			void* ptr = allocator.allocate(64, alignment);
			assert(ptr != nullptr);
			assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
			allocator.deallocate(ptr, 64, alignment);
		}
	}

	template <hpr::mem::AllocatorType Type>
	inline void test_multiple_allocations()
	{
		auto& allocator = hpr::MemoryModel::get_allocator<Type>();

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

	template <hpr::mem::AllocatorType Type>
	inline void test_with_containers()
	{
		auto& allocator = hpr::MemoryModel::get_allocator<Type>();

		std::pmr::vector<int> vec(&allocator);
		std::pmr::string str(&allocator);

		for (int i = 0; i < 1000; ++i)
			vec.push_back(i);

		for (int i = 0; i < 1000; ++i)
			str += "a";

		vec.clear();
		str.clear();
	}

	template <hpr::mem::AllocatorType Type>
	inline void basic_tests()
	{
		std::cout << "testing basic allocation... " << std::flush;
		test_basic_allocation<Type>();
		std::cout << "OK" << std::endl;
		std::cout << "testing alignment... " << std::flush;
		test_alignment<Type>();
		std::cout << "OK" << std::endl;
		std::cout << "testing multiple allocations... " << std::flush;
		test_multiple_allocations<Type>();
		std::cout << "OK" << std::endl;
		std::cout << "testing with containers... " << std::flush;
		test_with_containers<Type>();
		std::cout << "OK" << std::endl;
		std::cout << std::endl;
	}

	virtual void setup() = 0;
	virtual void run() = 0;
	virtual void teardown() = 0;
};
