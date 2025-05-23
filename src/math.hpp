#include <concepts>
#include <stdexcept>
#include <type_traits>

#include "fail.hpp"


namespace hpr::math {


template <std::integral T>
constexpr auto int_pow(T base, T exp) -> std::conditional_t<std::is_signed_v<T>, double, T>
{
	if constexpr (std::is_signed_v<T>) {
		if (exp < 0) {
			if (base == 0) {
				fatal("int_pow: base cannot be zero for negative exponent");
			}
			return static_cast<double>(1) / int_pow(base, -exp);
		}
	}

	T result = 1;
	while (exp) {
		if (exp & 1) result *= base;
		base *= base;
		exp >>= 1;
	}
	return result;
}


template <std::unsigned_integral T, std::signed_integral U>
constexpr auto int_pow(T, U)
{
	static_assert([] { return false; }(),
		"int_pow: signed exponent not allowed with unsigned base");
}


template<std::integral T>
constexpr T ceil_div(T n, T d)
{
	if constexpr (std::is_signed_v<T>) {
		return (n > 0) ? ((n + d - T(1)) / d) : (n / d);
	}
	else {
		return (n + d - T(1)) / d;
	}
}


template<std::unsigned_integral T>
constexpr T log2_exact(T n)
{
	T result = 0;
	while (n > 1) {
		n >>= 1;
		++result;
	}
	return result;
}

} // hpr::math
