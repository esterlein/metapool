#pragma once


namespace mtp {

	template <typename System>
	struct RegistryForSystem;

} // mtp


namespace hpr::mem {

	enum class AllocatorInterface
	{
		native,
		std_adapter,
		pmr_adapter
	};

} // hpr::mem
