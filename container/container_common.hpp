#pragma once

#if !defined(MTP_RESTRICT)
	#if defined(_MSC_VER)
		#define MTP_RESTRICT __restrict
	#elif defined(__clang__) || defined(__GNUC__)
		#define MTP_RESTRICT __restrict__
	#else
		#define MTP_RESTRICT
	#endif
#endif

