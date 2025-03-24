#include <map>
#include <string>
#include <memory>
#include <iostream>

#include "benchmark.hpp"
#include "benchmark_simple.hpp"
#include "benchmark_intermediate.hpp"
#include "benchmark_complex.hpp"




std::unique_ptr<Benchmark> create_benchmark_simple();
std::unique_ptr<Benchmark> create_benchmark_intermediate();
std::unique_ptr<Benchmark> create_benchmark_complex();


int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "usage: metapool [simple|intermediate|complex]" << std::endl;
		return 1;
	}

	const std::map<std::string, std::unique_ptr<Benchmark>(*)()> benchmark_factories = {
		{"simple", create_benchmark_simple},
		{"intermediate", create_benchmark_intermediate},
		{"complex", create_benchmark_complex}
	};

	std::string type = argv[1];

	auto factory_it = benchmark_factories.find(type);
	if (factory_it == benchmark_factories.end()) {
		std::cerr << "unknown benchmark type: " << type << std::endl;
		std::cerr << "available types: simple, intermediate, complex" << std::endl;
		return 1;
	}

	try {
		std::unique_ptr<Benchmark> bench = factory_it->second();

		bench->setup();
		bench->run();
		bench->teardown();
	}
	catch (const std::exception& e) {
		std::cerr << "benchmark failed: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}
