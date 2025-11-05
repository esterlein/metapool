#pragma once

#include <vector>
#include <cstring>
#include <ostream>
#include <iostream>

#include "mtp_memory.hpp"


static constexpr size_t k_qword = 8;


inline size_t align_down8(size_t x)
{
	return x & ~size_t(7);
}


inline void touch_prefix(void* buffer, size_t size_bytes)
{
	std::byte* base = static_cast<std::byte*>(buffer);
	const size_t write_limit = align_down8(size_bytes);

	for (size_t offset = 0; offset < write_limit; offset += k_qword) {
		*reinterpret_cast<uint64_t*>(base + offset) =
			(static_cast<uint64_t>(offset) ^ 0x9E3779B97F4A7C15ULL);
	}
}


inline void touch_step(void* buffer, size_t size_bytes, size_t step_bytes, size_t bytes_limit)
{
	std::byte* base = static_cast<std::byte*>(buffer);
	const size_t step = (step_bytes ? step_bytes : 64);
	size_t bytes_written = 0;

	for (size_t offset = 0; offset < size_bytes && bytes_written < bytes_limit; offset += step) {
		const size_t aligned = align_down8(offset);
		*reinterpret_cast<uint64_t*>(base + aligned) = (static_cast<uint64_t>(offset) * 0x9E37U);
		bytes_written += k_qword;
	}
}


inline void touch_scatter(void* buffer, size_t size_bytes, size_t qword_count, uint32_t seed)
{
	std::byte* base = static_cast<std::byte*>(buffer);
	uint64_t state = seed ? seed : 1;

	auto next = [&state]() -> uint64_t {
		state ^= (state << 13);
		state ^= (state >> 7);
		state ^= (state << 17);
		return state;
	};

	for (size_t i = 0; i < qword_count; ++i) {
		const size_t offset = align_down8(static_cast<size_t>(next() % size_bytes));
		*reinterpret_cast<uint64_t*>(base + offset) =
			(next() ^ (static_cast<uint64_t>(i) << 32));
	}
}



class Benchmark
{
public:

	virtual ~Benchmark()
	{}

private:

	inline void test_basic_allocation()
	{
		std::cout << "testing basic allocation..." << std::flush;

		auto& allocator = mtp::get_allocator<mtp::default_set>();

		std::byte* ptr = allocator.alloc(100, alignof(int));
		assert(ptr != nullptr);
		std::memset(ptr, 0xAB, 100);
		allocator.free(ptr);

		std::cout << "ok" << std::endl;
	}

	inline void test_alignment()
	{
		std::cout << "testing alignment..." << std::flush;

		auto& allocator = mtp::get_allocator<mtp::default_set>();

		for (size_t alignment : {1, 2, 4, 8, 16, 32, 64, 128, 256}) {
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

		auto& allocator = mtp::get_allocator<mtp::default_set>();

		std::vector<std::byte*> vec;
		std::array<size_t, 6> sizes {8, 16, 32, 64, 128, 256};

		for (size_t size : sizes) {
			std::byte* ptr = allocator.alloc(size, alignof(int));
			assert(ptr != nullptr);
			vec.push_back(ptr);
		}

		for (std::byte* ptr : vec) {
			allocator.free(ptr);
		}

		std::cout << "ok" << std::endl;
	}

	inline void test_std_containers()
	{
		std::cout << "testing std / pmr containers adapters..." << std::flush;

		auto vec_std = mtp::make_vector<int, mtp::default_set>();
		vec_std.reserve(1000);

		for (int i = 0; i < 1000; ++i)
			vec_std.push_back(i);
	
		auto str_std = mtp::make_string<mtp::default_set>();

		for (int i = 0; i < 1000; ++i)
			str_std += 'a';
	
		vec_std.clear();
		str_std.clear();

		auto vec_pmr = mtp::cntr::make_pmr_vector<int, mtp::default_set>();

		for (int i = 0; i < 1000; ++i)
			vec_pmr.push_back(i);

		auto str_pmr = mtp::cntr::make_pmr_string<mtp::default_set>();

		for (int i = 0; i < 1000; ++i)
			str_pmr += 'a';

		vec_pmr.clear();
		str_pmr.clear();

		std::cout << "ok" << std::endl;
	}

public:

	inline void run_basic_tests()
	{
		test_basic_allocation();
		test_alignment();
		test_multiple_allocations();
		test_std_containers();

		std::cout << std::endl;
	}

	virtual void setup() = 0;
	virtual void run() = 0;
	virtual void teardown() = 0;
};
