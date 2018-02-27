#include <cleo/value.hpp>

std::int64_t CLEO_CDECL other()
{
    return 13;
}

std::int64_t CLEO_CDECL other(std::int64_t x)
{
    return x + 13;
}

std::int64_t CLEO_CDECL other(std::int64_t x, std::int64_t y)
{
    return x + y + 13;
}

std::int64_t CLEO_CDECL other(std::int64_t x, std::int64_t y, std::int64_t z)
{
    return x + y + z + 13;
}

std::int64_t CLEO_CDECL other(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10, std::int64_t a11,
    std::int64_t a12)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11 + a12;
}

std::int64_t CLEO_CDECL other(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10, std::int64_t a11,
    std::int64_t a12, std::int64_t a13, std::int64_t a14, std::int64_t a15)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11 + a12 + a13 + a14 + a15;
}
