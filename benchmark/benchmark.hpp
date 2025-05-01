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

	inline auto& simple_allocator()
	{
		return hpr::MemoryModel::get_allocator <
			hpr::mem::AllocatorType::simple,
			hpr::mem::AllocatorInterface::std_adapter
		>();
	}

	inline void test_basic_allocation()
	{
		std::cout << "testing basic allocation..." << std::flush;

		auto& allocator = simple_allocator();

		void* ptr = allocator.allocate(100, 8);
		assert(ptr != nullptr);
		std::memset(ptr, 0xAB, 100);
		allocator.deallocate(ptr, 100, 8);

		std::cout << "OK" << std::endl;
	}

	inline void test_alignment()
	{
		std::cout << "testing alignment..." << std::flush;

		auto& allocator = simple_allocator();

		for (size_t alignment : {1, 2, 4, 8, 16, 32, 64}) {
			void* ptr = allocator.allocate(64, alignment);
			assert(ptr != nullptr);
			assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
			allocator.deallocate(ptr, 64, alignment);
		}

		std::cout << "OK" << std::endl;
	}

	inline void test_multiple_allocations()
	{
		std::cout << "testing multiple allocations..." << std::flush;

		auto& allocator = simple_allocator();

		std::vector<void*> blocks;
		std::size_t sizes[] = {8, 16, 32, 64, 128, 256};

		for (std::size_t size : sizes) {
			void* ptr = allocator.allocate(size, 8);
			assert(ptr != nullptr);
			blocks.push_back(ptr);
		}

		for (void* ptr : blocks) {
			allocator.deallocate(ptr, 0, 8);
		}

		std::cout << "OK" << std::endl;
	}

	inline void test_with_containers()
	{
		std::cout << "testing with containers..." << std::flush;

		auto& allocator = simple_allocator();

		std::pmr::polymorphic_allocator<int> int_alloc(&allocator);
		std::pmr::polymorphic_allocator<char> char_alloc(&allocator);

		std::pmr::vector<int> vec(int_alloc);
		std::pmr::string str(char_alloc);

		for (int i = 0; i < 1000; ++i)
			vec.push_back(i);

		for (int i = 0; i < 1000; ++i)
			str += 'a';

		vec.clear();
		str.clear();

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
