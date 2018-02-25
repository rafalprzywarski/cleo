#include <cstdint>
#include <cleo/value.hpp>

extern "C"
{

std::int64_t CLEO_CDECL ret42()
{
    return 42;
}

std::int64_t CLEO_CDECL inc17(std::int64_t x)
{
    return x + 17;
}

}
