#include "error.hpp"
#include "global.hpp"
#include "print.hpp"

namespace cleo
{

const char *Exception::what() const noexcept
{
    if (*current_exception == nil)
        return "nil";
    try
    {
        Root text{pr_str(*current_exception)};
        buffer = std::string(cleo::get_string_ptr(*text), cleo::get_string_len(*text));
    }
    catch (...)
    {
        return "<what() error>";
    }
    return buffer.c_str();
}

void throw_exception(Force e)
{
    current_exception = e;
    throw Exception();
}

Force catch_exception()
{
    Root e{*current_exception};
    current_exception = nil;
    return *e;
}

Force new_read_error(Value msg)
{
    return create_object1(type::ReadError, msg);
}

Force read_error_message(Value e)
{
    return get_object_element(e, 0);
}

Force new_unexpected_end_of_input()
{
    return create_object0(type::UnexpectedEndOfInput);
}

Force unexpected_end_of_input_message(Value)
{
    return create_string("unexpected end of input");
}

Force new_call_error(Value msg)
{
    return create_object1(type::CallError, msg);
}

Force call_error_message(Value e)
{
    return get_object_element(e, 0);
}

Force new_symbol_not_found(Value msg)
{
    return create_object1(type::SymbolNotFound, msg);
}

Force symbol_not_found_message(Value e)
{
    return get_object_element(e, 0);
}

Force new_illegal_argument(Value msg)
{
    return create_object1(type::IllegalArgument, msg);
}

Force illegal_argument_message(Value e)
{
    return get_object_element(e, 0);
}

Force new_illegal_state(Value msg)
{
    return create_object1(type::IllegalState, msg);
}

Force illegal_state_message(Value e)
{
    return get_object_element(e, 0);
}


}
