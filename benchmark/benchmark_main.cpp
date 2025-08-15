#include <map>
#include <string>
#include <memory>
#include <iostream>

#include "benchmark.hpp"
#include "benchmark_micro.hpp"
#include "benchmark_selective.hpp"



std::unique_ptr<Benchmark> create_benchmark_selective();
std::unique_ptr<Benchmark> create_benchmark_micro();



int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "usage: metapool [selective|micro]" << std::endl;
		return 1;
	}

	const std::map<std::string, std::unique_ptr<Benchmark>(*)()> benchmark_factories = {
		{"selective", create_benchmark_selective},
		{"micro",     create_benchmark_micro}

	};

	std::string type = argv[1];

	auto factory_it = benchmark_factories.find(type);
	if (factory_it == benchmark_factories.end()) {
		std::cerr << "unknown benchmark type: " << type << std::endl;
		std::cerr << "available types: selective, micro" << std::endl;
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
