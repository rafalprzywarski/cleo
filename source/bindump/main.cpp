#include <cleo/value.hpp>
#include <cleo/util.hpp>
#include <iostream>
#include <iomanip>

constexpr auto SIZE = 96;
const auto NAME = cleo::create_symbol("my-fn");

std::int64_t CLEO_CDECL other();
std::int64_t CLEO_CDECL other(std::int64_t x);
std::int64_t CLEO_CDECL other(std::int64_t x, std::int64_t y);

cleo::Value CLEO_CDECL example(const cleo::Value *args, std::uint8_t num_args, cleo::Value& err)
{
    if (num_args != 1)
    {
        err = cleo::create_arity_error(NAME, num_args).value();
        return cleo::nil;
    }
    return cleo::create_int64(other(get_int64_value(args[0]))).value();
}

int main()
{
    auto source = reinterpret_cast<const unsigned char *>(example);
    std::cout << "bindump:\n";
    std::cout << std::hex << std::setfill('0');
    for (auto p = source; p < (source + SIZE); ++p)
        std::cout << std::setw(2) << int(*p) << ' ';
    std::cout << std::endl;
}
