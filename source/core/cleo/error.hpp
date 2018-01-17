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

Force new_read_error(Value msg);
Force read_error_message(Value e);

Force new_unexpected_end_of_input();
Force unexpected_end_of_input_message(Value e);

Force new_call_error(Value msg);
Force call_error_message(Value e);

Force new_symbol_not_found(Value msg);
Force symbol_not_found_message(Value e);

Force new_illegal_argument(Value msg);
Force illegal_argument_message(Value e);

Force new_illegal_state(Value msg);
Force illegal_state_message(Value e);

Force new_file_not_found(Value msg);
Force file_not_found_message(Value e);

Force new_arithmetic_exception(Value msg);
Force arithmetic_exceptio_message(Value e);

}
