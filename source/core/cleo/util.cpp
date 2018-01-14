#include "util.hpp"
#include "global.hpp"
#include "print.hpp"
#include "error.hpp"

namespace cleo
{

std::string to_string(Value val)
{
    Root text{pr_str(val)};
    return {get_string_ptr(*text), get_string_len(*text)};
}

void throw_arity_error(Value name, std::uint8_t n)
{
    Root msg{create_string("Wrong number of args (" + std::to_string(n) + ") passed to: " + to_string(name))};
    throw_exception(new_call_error(*msg));
}

void check_arity(Value name, std::uint8_t num_args, std::uint8_t actual_num_args)
{
    if (num_args != actual_num_args)
        throw_arity_error(name, actual_num_args);
}


}
