#include <cleo/value.hpp>
#include <cleo/util.hpp>
#include <iostream>
#include <iomanip>

constexpr auto SIZE = 196;
cleo::Value NAME;

std::int64_t CLEO_CDECL other();
std::int64_t CLEO_CDECL other(std::int64_t x);
std::int64_t CLEO_CDECL other(std::int64_t x, std::int64_t y);
std::int64_t CLEO_CDECL other(std::int64_t x, std::int64_t y, std::int64_t z);

cleo::Value CLEO_CDECL example(const cleo::Value *args, std::uint8_t num_args, cleo::Value& err)
{
    if (num_args != 2)
    {
        err = cleo::create_arity_error(NAME, num_args).value();
        return cleo::nil;
    }
    auto arg0 = args[0];
    if (get_value_tag(arg0) != cleo::tag::INT64)
    {
        err = cleo::create_arg_type_error(arg0, 0).value();
        return cleo::nil;
    }
    auto arg1 = args[1];
    if (get_value_tag(arg1) != cleo::tag::INT64)
    {
        err = cleo::create_arg_type_error(arg1, 1).value();
        return cleo::nil;
    }
    auto arg2 = args[2];
    if (get_value_tag(arg2) != cleo::tag::INT64)
    {
        err = cleo::create_arg_type_error(arg2, 2).value();
        return cleo::nil;
    }
    return cleo::create_int64(other(get_int64_value(arg0), get_int64_value(arg1), get_int64_value(arg2))).value();
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
