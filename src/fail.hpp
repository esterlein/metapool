#pragma once

#include <cstdio>
#include <cstdlib>
#include <string_view>


#ifdef __GNUC__
    #define THIS_FUNC __PRETTY_FUNCTION__
#elif defined(_MSC_VER)
    #define THIS_FUNC __FUNCSIG__
#else
    #define THIS_FUNC __func__
#endif


namespace hpr {


[[noreturn]] inline void fatal(std::string_view msg) noexcept
{
	std::fputs("fatal: ", stderr);
	std::fwrite(msg.data(), 1, msg.size(), stderr);
	std::fputc('\n', stderr);
	std::fflush(stderr);
	std::abort();
}
} // hpr
