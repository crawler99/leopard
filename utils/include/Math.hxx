#ifndef LEOPARD_UTILS_MATH_HXX_
#define LEOPARD_UTILS_MATH_HXX_

#include <cstdint>
#include <type_traits>

namespace leopard { namespace utils {

class Math
{
public:
    template <typename T>
    static typename std::enable_if<std::is_unsigned<T>::value, T>::type NextPowerOf2(T n)
    {
        T count = 0;

        // First n in the below condition is for the case where n is 0.
        if (n && !(n & (n - 1)))
        {
            return n;
        }

        while (n != 0)
        {
            n >>= 1;
            count += 1;
        }
        return 1 << count;
    }

    static uint32_t NextPowerOf2(uint32_t n)
    {
        --n;
        n |= n >> 1;
        n |= n >> 2;
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        return ++n;
    }
};

}} // namespace utils::leopard

#endif // LEOPARD_UTILS_MATH_HXX_