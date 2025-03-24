#include <vector>
#include <utility>
#include <memory_resource>

#include "memory_model.hpp"



class Benchmark
{
public:

	virtual ~Benchmark() {}

	inline void test_basic_allocation()
	{
		hpr::MemoryModel memory_model;
		auto& allocator = memory_model.get_memory_resource();

		void* ptr = allocator.allocate(100, 8);
		assert(ptr != nullptr);
		std::memset(ptr, 0xAB, 100);
		allocator.deallocate(ptr, 100, 8);
	}

	inline void test_alignment()
	{
		hpr::MemoryModel memory_model;
		auto& allocator = memory_model.get_memory_resource();

		for (size_t alignment : {1, 2, 4, 8, 16, 32, 64}) {
			void* ptr = allocator.allocate(64, alignment);
			assert(ptr != nullptr);
			assert(reinterpret_cast<uintptr_t>(ptr) % alignment == 0);
			allocator.deallocate(ptr, 64, alignment);
		}
	}

	inline void test_multiple_allocations()
	{
		hpr::MemoryModel memory_model;
		auto& allocator = memory_model.get_memory_resource();

		std::vector<std::pair<void*, size_t>> blocks;
		for (size_t size : {8, 16, 32, 64, 128, 256}) {
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
		hpr::MemoryModel memory_model;
		auto& allocator = memory_model.get_memory_resource();

		std::pmr::vector<int> vec(&allocator);
		std::pmr::string str(&allocator);

		for (int i = 0; i < 1000; ++i)
			vec.push_back(i);

		for (int i = 0; i < 1000; ++i)
			str += "a";

		vec.clear();
		str.clear();
	}

	inline void basic_tests()
	{
		test_basic_allocation();
		test_alignment();
		test_multiple_allocations();
		test_with_containers();
	}

	virtual void setup() = 0;
	virtual void run() = 0;
	virtual void teardown() = 0;
};
