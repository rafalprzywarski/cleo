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
