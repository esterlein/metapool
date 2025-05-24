#pragma once

#include "metapool.hpp"
#include "metapool_config.hpp"
#include "metapool_registry.hpp"
#include "allocator_interface.hpp"


namespace mtp {


using hpr::Metapool;
using hpr::MetapoolRegistry;
using hpr::mem::MetapoolConfig;
using hpr::mem::CapacityFunction;


//----------------------------------------
// registry definitions
//----------------------------------------

using BenchmarkSimpleRegistry =
	MetapoolRegistry <
		2UL * 1024 * 1024 * 1024,
		64UL,
		Metapool<MetapoolConfig<CapacityFunction::Flat, 2048,  8,     8,  2048>>,
		Metapool<MetapoolConfig<CapacityFunction::Flat, 2048, 16,  2048,  4128>>,
		Metapool<MetapoolConfig<CapacityFunction::Flat,    8, 16, 65664, 65680>>
	>;

using BenchmarkIntermediateRegistry =
	MetapoolRegistry <
		2UL * 1024 * 1024 * 1024,
		64UL,
		Metapool<MetapoolConfig<CapacityFunction::Flat, 5120, 16,      48,     112>>,
		Metapool<MetapoolConfig<CapacityFunction::Flat,  512,  8,     112,     136>>,
		Metapool<MetapoolConfig<CapacityFunction::Flat,  512,  8,    1040,    1048>>,
		Metapool<MetapoolConfig<CapacityFunction::Flat,  288,  8,   16400,   16408>>,
		Metapool<MetapoolConfig<CapacityFunction::Flat,   64,  8,   40008,   40016>>,
		Metapool<MetapoolConfig<CapacityFunction::Flat,   32,  8,  512008,  512016>>,
		Metapool<MetapoolConfig<CapacityFunction::Flat,   32,  8, 4194560, 4194568>>
	>;


//----------------------------------------
// system tags
//----------------------------------------

struct BasicTestsSystem {};
struct BenchmarkSimpleSystem {};
struct BenchmarkIntermediateSystem {};


//----------------------------------------
// system-to-registry mapping
//----------------------------------------

template <>
struct RegistryForSystem<BasicTestsSystem> {
	using type = BenchmarkSimpleRegistry;
};

template <>
struct RegistryForSystem<BenchmarkSimpleSystem> {
	using type = BenchmarkSimpleRegistry;
};

template <>
struct RegistryForSystem<BenchmarkIntermediateSystem> {
	using type = BenchmarkIntermediateRegistry;
};

template <typename System>
using RegistryForSystemT = typename RegistryForSystem<System>::type;

} // mtp
