#pragma once
#include <stdexcept>
#include "value.hpp"

namespace cleo
{

struct Exception : std::runtime_error
{
    Exception() : std::runtime_error("clue exception") { }
};

struct Error : std::runtime_error
{
    Error(const std::string& msg) : std::runtime_error(msg) {}
};

struct IllegalArgument : Error
{
    IllegalArgument(const std::string& msg) : Error(msg) {}
};

[[noreturn]] void throw_exception(Force e);
Force catch_exception();

Force new_read_error(Value msg);
Force read_error_message(Value e);

Force new_unexpected_end_of_input();
Force unexpected_end_of_input_message(Value e);

Force new_call_error(Value msg);
Force call_error_message(Value e);

Force new_symbol_not_found(Value msg);
Force symbol_not_found_message(Value e);

}
