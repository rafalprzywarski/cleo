#pragma once
#include <stdexcept>
#include "value.hpp"

namespace cleo
{

class Exception : public std::exception
{
public:
    const char *what() const noexcept override;
private:
    mutable std::string buffer;
};

[[noreturn]] void throw_exception(Force e);
Force catch_exception();

Value exception_message(Value e);

Force new_read_error(Value msg, Value line, Value column);
Int64 read_error_line(Value e);
Int64 read_error_column(Value e);

Force new_unexpected_end_of_input(Value line, Value column);
Force new_call_error(Value msg);
Force new_symbol_not_found(Value msg);
Force new_illegal_argument(Value msg);
Force new_illegal_state(Value msg);
Force new_file_not_found(Value msg);
Force new_arithmetic_exception(Value msg);
Force new_index_out_of_bounds();
Force new_compilation_error(Value msg);
Force new_stack_overflow();

}
