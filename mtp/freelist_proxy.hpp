#pragma once

#include "metapool_config.hpp"


namespace mtp::core {


template <mtp::cfg::IsMetapoolConfig Config>
class Metapool;


class FreelistProxy final
{
public:

	FreelistProxy() = delete;

	[[nodiscard]] inline std::byte* fetch() const
	{
		return fn_fetch(m_freelist_ptr);
	}

	inline void release(std::byte* block) const
	{
		fn_release(m_freelist_ptr, block);
	}

	inline void reset() const
	{
		fn_reset(m_freelist_ptr);
	}

private:

	using FreelistFetch   = std::byte* (*)(void*);
	using FreelistRelease = void (*)(void*, std::byte*);
	using FreelistReset   = void (*)(void*);

	void*           m_freelist_ptr  {nullptr};
	FreelistFetch   fn_fetch        {nullptr};
	FreelistRelease fn_release      {nullptr};
	FreelistReset   fn_reset        {nullptr};


	template <mtp::cfg::IsMetapoolConfig>
	friend class Metapool;


	FreelistProxy(
		void*           ptr,
		FreelistFetch   fetch,
		FreelistRelease release,
		FreelistReset   reset)
			: m_freelist_ptr {ptr}
			, fn_fetch       {fetch}
			, fn_release     {release}
			, fn_reset       {reset}
	{}
};

} // mtp::core

