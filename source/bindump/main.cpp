#include <cleo/value.hpp>
#include <cleo/util.hpp>
#include <iostream>
#include <iomanip>

constexpr auto SIZE = 128;
cleo::Value NAME;

std::int64_t CLEO_CDECL other();
std::int64_t CLEO_CDECL other(std::int64_t x);
std::int64_t CLEO_CDECL other(std::int64_t x, std::int64_t y);
std::int64_t CLEO_CDECL other(std::int64_t x, std::int64_t y, std::int64_t z);
std::int64_t CLEO_CDECL other(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10, std::int64_t a11,
    std::int64_t a12);
std::int64_t CLEO_CDECL other(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10, std::int64_t a11,
    std::int64_t a12, std::int64_t a13, std::int64_t a14, std::int64_t a15);

#define CLEO_ARG(n, type) \
    auto arg##n = static_cast<type>(args[n]); \

cleo::Value CLEO_CDECL example(const std::uint64_t *args, std::uint8_t num_args)
{
    CLEO_ARG(0, std::int64_t)
    CLEO_ARG(1, std::int64_t)
    CLEO_ARG(2, std::int64_t)
    CLEO_ARG(3, std::int64_t)
    CLEO_ARG(4, std::int64_t)
    CLEO_ARG(5, std::int64_t)
    CLEO_ARG(6, std::int64_t)
    CLEO_ARG(7, std::int64_t)
    CLEO_ARG(8, std::int64_t)
    CLEO_ARG(9, std::int64_t)
    CLEO_ARG(10, std::int64_t)
    CLEO_ARG(11, std::int64_t)
    CLEO_ARG(12, std::int64_t)
    return cleo::create_int64(other(
        arg0, arg1, arg2, arg3,
        arg4, arg5, arg6, arg7,
        arg8, arg9, arg10, arg11,
        arg12)).value();
}

int main()
{
    NAME = cleo::create_symbol("my-fn");
    auto source = reinterpret_cast<const unsigned char *>(example);
    std::cout << "bindump:\n";
    std::cout << std::hex << std::setfill('0');
    for (auto p = source; p < (source + SIZE); ++p)
        std::cout << std::setw(2) << int(*p) << ' ';
    std::cout << std::endl;
}
