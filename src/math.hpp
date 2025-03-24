#include <concepts>
#include <stdexcept>
#include <type_traits>


namespace hpr {
namespace math {

	template <std::integral T>
	constexpr auto int_pow(T base, T exp) -> std::enable_if_t<std::is_signed_v<T> || exp >= 0,
		std::conditional_t<std::is_signed_v<T>, double, T>>
	{
		if constexpr (std::is_signed_v<T>) {
			if (exp < 0) {
				if (base == 0) {
					throw std::invalid_argument("base cannot be zero for negative exponents");
				}
				return static_cast<double>(1) / int_pow(base, -exp);
			}
		}
	
		if (exp == 0) return 1;
		else if (exp == 1) return base;
		else if (exp == 2) return base * base;
		else if (exp == 3) return base * base * base;
		else if (exp == 4) return base * base * base * base;

		T result = 1;
		while (exp) {
			if (exp & 1) result *= base;
			base *= base;
			exp >>= 1;
		}
		return result;
	}
	
	template <std::unsigned_integral T>
	constexpr auto int_pow(T, T exp)
	{
		static_assert(exp >= 0, "negative exponents are not allowed for unsigned types");
	}
} // math
} // hpr
