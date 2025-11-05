#pragma once

#include "mtpint.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <algorithm>


namespace mtp::err {


struct msg
{
	const char* ascii;
	const char* text;
};


namespace {

	static constexpr size_t frame_width    = 60;
	static constexpr size_t frame_padding  = 4;
	static constexpr size_t buf_size       = 64;
	static constexpr size_t text_max_width = frame_width - frame_padding;
}


inline void print_framed_block(
		const char* line1,
		const char* line2   = nullptr,
		const char* context = nullptr,
		bool dynamic_width  = false)
{
	size_t len1 = std::strlen(line1);
	size_t len2 = line2 ? std::strlen(line2) : 0;
	size_t len3 = context ? std::strlen(context) : 0;

	size_t inner_width = std::max({len1, len2, len3});
	if (inner_width > text_max_width)
		inner_width = text_max_width;

	size_t width = dynamic_width ? inner_width + frame_padding : frame_width;

	for (size_t i = 0; i < width; ++i)
		std::fputc('*', stderr);
	std::fputc('\n', stderr);

	auto print = [width](const char* str) {
		size_t len = std::strlen(str);
		if (len > text_max_width)
			len = text_max_width;

		size_t content_width = len + 2;
		size_t pad_total = width - 2 - content_width;
		size_t pad_left = pad_total / 2;
		size_t pad_right = pad_total - pad_left;

		std::fprintf(stderr, "*%*s %.*s %*s*\n",
			static_cast<int>(pad_left), "",
			static_cast<int>(len), str,
			static_cast<int>(pad_right), ""
		);
	};

	print(line1);
	if (line2) print(line2);
	if (context && context[0] != '\0') print(context);

	for (size_t i = 0; i < width; ++i)
		std::fputc('*', stderr);
	std::fputc('\n', stderr);
}


[[noreturn]] inline void fatal(const msg& message, const char* context = nullptr)
{
	std::fputs(message.ascii, stderr);

	const char* line1 = message.text;
	const char* line2 = nullptr;

	static std::array<char, buf_size> buf1, buf2;

	if (const char* newline = std::strchr(message.text, '\n')) {
		size_t len1 = newline - message.text;
		std::memcpy(buf1.data(), message.text, len1);
		buf1[len1] = '\0';
		line1 = buf1.data();

		const char* rest = newline + 1;
		if (const char* newline2 = std::strchr(rest, '\n')) {
			size_t len2 = newline2 - rest;
			std::memcpy(buf2.data(), rest, len2);
			buf2[len2] = '\0';
			line2 = buf2.data();
		}
		else {
			line2 = rest;
		}
	}

	print_framed_block(line1, line2, context, false);
	std::fputc('\n', stderr);

	std::fflush(stderr);
	std::abort();
}


[[noreturn]] inline void fatal(const char* message, const char* context)
{
	const char* line1 = message;
	const char* line2 = nullptr;

	static std::array<char, buf_size> buf1, buf2;

	if (const char* newline = std::strchr(message, '\n')) {
		size_t len1 = newline - message;
		std::memcpy(buf1.data(), message, len1);
		buf1[len1] = '\0';
		line1 = buf1.data();

		const char* rest = newline + 1;
		if (const char* newline2 = std::strchr(rest, '\n')) {
			size_t len2 = newline2 - rest;
			std::memcpy(buf2.data(), rest, len2);
			buf2[len2] = '\0';
			line2 = buf2.data();
		}
		else {
			line2 = rest;
		}
	}

	std::fputc('\n', stderr);
	print_framed_block(line1, line2, context, true);
	std::fputc('\n', stderr);

	std::fflush(stderr);
	std::abort();
}


inline void dispatch_assert_fail(const char* message)
{
	fatal(message, nullptr);
}

inline void dispatch_assert_fail(const char* message, const char* context)
{
	fatal(message, context);
}

template <typename Message>
inline void dispatch_assert_fail(const Message& message)
{
	fatal(message, nullptr);
}

template <typename Message>
inline void dispatch_assert_fail(const Message& message, const char* context)
{
	fatal(message, context);
}


template <typename... Types>
inline const char* format_ctx(const char* fmt, Types... args)
{
	thread_local std::array<char, buf_size> buffer;
	std::snprintf(buffer.data(), buffer.size(), fmt, args...);
	return buffer.data();
}


#ifndef NDEBUG

	#define MTP_ASSERT(expr, message) \
		do { \
			if (!(expr)) { \
				::mtp::err::dispatch_assert_fail(message); \
			} \
		} while (0)

	#define MTP_ASSERT_CTX(expr, message, context) \
		do { \
			if (!(expr)) { \
				::mtp::err::dispatch_assert_fail(message, context); \
			} \
		} while (0)

#else

	#define MTP_ASSERT(expr, message) ((void)0)
	#define MTP_ASSERT_CTX(expr, message, context) ((void)0)

#endif



inline constexpr const char* ascii_land = R"(

    .     (( )    *      .       .  *   /\     .      .    .
 *     .    .   .                 .    /__\   .    *    .
.        _    __     *  _  /\  . _    /    \    _     .
  .   /\/ \__/  \______/ \/  \__/ \__/      \/\/ \   .  *
_____/     \     /\      /    \      /\     __    \_________
____/  /\   \___/  \____/      \____/  \___/  \    \  ~ ~ ~
 ~ /__/  \   ~ ~ ~ ~ ~ ~ ~      \  /    \      \____\~ ~ ~ ~
~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
 ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
)";

inline constexpr const char* ascii_city = R"(

   .       *        .     *      .     *     .       .     *
*     .    .   .         .____.     .     *     .    *   .
            .      .    .|    |   *     .      .   .    .
.    |‾|       |‾‾|      |‾‾‾‾|     |‾|     |‾‾‾‾‾|   |‾|  *
  *  |.|   |‾| |..|  |‾| |....|    |‾‾|     |  |  |   |.|  .
     |.|  _|‾‾‾‾|.|  |‾‾‾‾|...|‾‾‾||..|‾|...| |.| |‾| |.|
_|‾‾‾‾‾|.| |__  |.|  | __ |...|  _||..| |...| |.| | |_|.|___
_|__|__|.|_|  |_|.|__||__||...|_|__|__|_|...|_|.|_| |_|.|...
 ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' ' '
)";

inline constexpr const char* ascii_sea = R"(

                                           ....:::..
                      |))             ....:::::::::::...
        ( )       |)) |)) |))     ..:::::::::::::::::::::..
                 _|))_|))_|))___
_________________\____________/_______________________________
        ~~~       - - - - - -    - - - - - - - - - - - - -
       ~~~~~     - - - - - - - -      - - - - - - - - -
      ~~~~~~~     - - - - - -              - - - - -
     ~~~~~~~~~     -   -   -                  - - -
)";


inline constexpr msg zero_size_construct
{
	ascii_land,
	"[allocator::construct] sizeof(T) == 0"
};

inline constexpr msg construct_block_null
{
	ascii_city,
	"[allocator::construct] block is nullptr"
};

inline constexpr msg lookup_raw_size_zero
{
	ascii_city,
	"[allocator::lookup] raw_size must be > 0"
};

inline constexpr msg lookup_stride_misaligned
{
	ascii_sea,
	"[allocator::lookup] stride not a multiple of stride step"
};

inline constexpr msg lookup_no_match
{
	ascii_land,
	"[allocator::lookup] no metapool for size / alignment"
};

inline constexpr msg alloc_zero_size
{
	ascii_city,
	"[allocator::alloc] size must be > 0"
};

inline constexpr msg alloc_block_null
{
	ascii_sea,
	"[allocator::alloc] block is nullptr"
};

inline constexpr msg alloc_proxy_oob
{
	ascii_sea,
	"[allocator::alloc] proxy index out of bounds"
};

inline constexpr msg free_proxy_oob

{
	ascii_sea,
	"[allocator::alloc] proxy index out of bounds"
};

inline constexpr msg construct_proxy_oob
{
	ascii_sea,
	"[allocator::construct] proxy index out of bounds"
};

inline constexpr msg destruct_proxy_oob
{
	ascii_sea,
	"[allocator::destruct] proxy index out of bounds"
};

inline constexpr msg init_memory_null
{
	ascii_sea,
	"[freelist::initialize] passed memory is nullptr"
};

inline constexpr msg init_base_misaligned
{
	ascii_land,
	"[freelist::initialize] base misaligned"
};

inline constexpr msg init_next_misaligned
{
	ascii_city,
	"[freelist::initialize] next block misaligned"
};

inline constexpr msg release_block_null
{
	ascii_city,
	"[freelist::release] block is nullptr"
};

inline constexpr msg release_block_outside
{
	ascii_land,
	"[freelist::release] block outside freelist memory"
};

inline constexpr msg int_pow_zero_negative
{
	ascii_land,
	"[math::int_pow] base cannot be zero for negative exponent"
};

inline constexpr msg mem_model_proxy_align_fail
{
	ascii_city,
	"[memory_model] std::align failed for proxy buffer"
};

inline constexpr msg metapool_upstream_null
{
	ascii_sea,
	"[metapool::metapool] upstream is nullptr"
};

inline constexpr msg metapool_freelist_null
{
	ascii_sea,
	"[metapool::freelist_interface] freelist_ptr is nullptr"
};

inline constexpr msg metapool_fetch_block_null
{
	ascii_land,
	"[metapool::fetch] block is nullptr"
};

inline constexpr msg metapool_release_block_null
{
	ascii_city,
	"[metapool::release] block is nullptr"
};

inline constexpr msg metapool_release_index_oob
{
	ascii_sea,
	"[metapool::release] pool index out of bounds"
};

inline constexpr msg arena_alloc_failed
{
	ascii_land,
	"[arena::arena] aligned_alloc failed"
};

inline constexpr msg arena_fetch_no_fit
{
	ascii_sea,
	"[arena::fetch] not enough space to fit aligned allocation"
};

inline constexpr msg arena_fetch_overflow
{
	ascii_city,
	"[arena::fetch] allocation exceeds available capacity"
};


inline constexpr const char* vault_index_oob =
	"[vault] index out of bounds";

inline constexpr const char* vault_back_empty =
	"[vault::back] is empty";

inline constexpr const char* vault_pop_empty =
	"[vault::pop_back] is empty";

inline constexpr const char* vault_self_move_ctor =
	"[vault::move_ctor] self-move is invalid";

inline constexpr const char* vault_self_move_assign =
	"[vault::move_assign] self-move is invalid";

inline constexpr const char* vault_move_null_data =
	"[vault::move] source data is null";

inline constexpr const char* vault_clear_null_data =
	"[vault::clear] non-zero size with null data";


inline constexpr const char* slag_index_oob =
	"[slag] index out of bounds";

inline constexpr const char* slag_back_empty =
	"[slag::back] is empty";

inline constexpr const char* slag_emplace_exceeded =
	"[slag::emplace_back] over capacity";

inline constexpr const char* slag_pop_empty =
	"[slag::pop_back] is empty";

inline constexpr const char* slag_erase_oob =
	"[slag::erase_swap] index out of bounds";

inline constexpr const char* slag_self_move_ctor =
	"[slag::move_ctor] self-move is invalid";

inline constexpr const char* slag_self_move_assign =
	"[slag::move_assign] self-move is invalid";

inline constexpr const char* slag_move_null_data =
	"[slag::move] source data is null";

inline constexpr const char* slag_clear_null_data =
	"[slag::clear] non-zero size with null data";


#define SLAG_REF_MSG R"(

*******************************************************
* [slag] cannot be instantiated with reference / void *
*******************************************************

)"

#define VAULT_REF_MSG R"(

********************************************************
* [vault] cannot be instantiated with reference / void *
********************************************************

)"

#define CORE_TOO_MANY_POOLS_MSG R"(

*********************************************************
* [allocator core] too many metapools for 1-byte lookup *
*********************************************************

)"

#define CORE_ALIGNMENT_POW2_MSG R"(

***************************************************
* [allocator core] alignment mush be power of two *
***************************************************

)"

#define CONFIG_STRIDE_LIMIT_MSG R"(

*******************************************************************
* [allocator config] total stride count exceeds safe cache budget *
*         consider merging stride ranges or using steps           *
*******************************************************************

)"

#define CONFIG_EMPTY_RANGE_MSG R"(

***************************************************
* [allocator config] range list must not be empty *
***************************************************

)"

#define CONFIG_FIRST_ZERO_STEP_MSG R"(

*******************************************************
* [allocator config] first range has zero stride step *
*******************************************************

)"

#define FREELIST_STRIDE_TOO_SMALL_MSG R"(

**********************************************
* [freelist] stride must be >= sizeof(void*) *
**********************************************

)"

#define FREELIST_BLOCK_TOO_LARGE_MSG R"(

***************************************
* [freelist] block larger than stride *
***************************************

)"

#define INT_POW_SIGNED_EXPONENT_MSG R"(

************************************************************
* [int pow] signed exponent not allowed with unsigned base *
************************************************************

)"

#define SET_ARENA_TOO_LARGE_MSG R"(

*********************************************************
* [metapool_set] arena exceeds mtp::cfg::max_arena_size *
*  raise the limit if your target system can handle it  *
*      otherwise expect SIGSEGV / SIGKILL or worse      *
*********************************************************

)"

#define SET_INVALID_SEQUENCE_MSG R"(

**********************************************************
* [metapool set] stride overlaps or invalid stride steps *
**********************************************************

)"

} // mtp::err
