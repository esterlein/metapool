#pragma once

#include <array>
#include <cstdint>

#include "metapool_config.hpp"


namespace hpr {


class MetapoolProxy final
{
public:

	MetapoolProxy() = delete;

	std::byte* fetch(uint8_t pool_index) const
	{
		return fn_fetch(m_mpool_ptr, pool_index);
	}

	void release(uint8_t pool_index, std::byte* block) const
	{
		fn_release(m_mpool_ptr, pool_index, block);
	}

	void reset() const
	{
		fn_reset(m_mpool_ptr);
	}

private:

	using MetapoolFetch   = std::byte* (*)(void*, uint8_t);
	using MetapoolRelease = void (*)(void*, uint8_t, std::byte*);
	using MetapoolReset   = void (*)(void*);

	void*           m_mpool_ptr {nullptr};
	MetapoolFetch   fn_fetch    {nullptr};
	MetapoolRelease fn_release  {nullptr};
	MetapoolReset   fn_reset    {nullptr};

	template <mem::IsMetapoolConfig Config>
	friend class Metapool;

	MetapoolProxy(
		void* mpool_ptr,
		MetapoolFetch mpool_fetch,
		MetapoolRelease mpool_release,
		MetapoolReset mpool_reset)
			: m_mpool_ptr {mpool_ptr}
			, fn_fetch    {mpool_fetch}
			, fn_release  {mpool_release}
			, fn_reset    {mpool_reset}
	{}
};


namespace mem {


	template <typename ProxyArray, auto MetapoolCount>
	concept MetapoolProxyArray =
		std::same_as<std::remove_cvref_t<ProxyArray>, std::array<MetapoolProxy, MetapoolCount>>;

} // hpr::mem
} // hpr
