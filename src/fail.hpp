#pragma once

#include <cstdio>
#include <cstdlib>
#include <string_view>


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
