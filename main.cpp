#include <map>
#include <string>
#include <memory>
#include <iostream>

#include "benchmark.h"


std::unique_ptr<benchmark> create_benchmark_simple();
std::unique_ptr<benchmark> create_benchmark_intermediate();
std::unique_ptr<benchmark> create_benchmark_complex();


int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "usage: metapool [simple|intermediate|complex]" << std::endl;
		return 1;
	}

	const std::map<std::string, std::unique_ptr<benchmark>(*)()> benchmark_factories = {
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
		std::unique_ptr<benchmark> bench = factory_it->second();

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
