#include "error.hpp"
#include "global.hpp"
#include "print.hpp"

namespace cleo
{

const char *Exception::what() const noexcept
{
    if (!*current_exception)
        return "nil";
    try
    {
        Root text{get_value_type(*current_exception).is(*type::IllegalArgument) ?
                  illegal_argument_message(*current_exception) :
                  pr_str(*current_exception)};
        buffer = std::string(cleo::get_string_ptr(*text), cleo::get_string_size(*text));
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

Force cast_error_message(Value e)
{
    return get_static_object_element(e, 0);
}

Force new_read_error(Value msg, Value line, Value column)
{
    return create_object3(*type::ReadError, msg, line, column);
}

Force read_error_message(Value e)
{
    return get_static_object_element(e, 0);
}

Int64 read_error_line(Value e)
{
    return get_static_object_int(e, 1);
}

Int64 read_error_column(Value e)
{
    return get_static_object_int(e, 2);
}

Force new_unexpected_end_of_input(Value line, Value column)
{
    Root msg{create_string("unexpected end of input")};
    return create_object3(*type::UnexpectedEndOfInput, *msg, line, column);
}

Force unexpected_end_of_input_message(Value e)
{
    return get_static_object_element(e, 0);
}

Int64 unexpected_end_of_input_line(Value e)
{
    return get_static_object_int(e, 1);
}

Int64 unexpected_end_of_input_column(Value e)
{
    return get_static_object_int(e, 2);
}

Force new_call_error(Value msg)
{
    return create_object1(*type::CallError, msg);
}

Force call_error_message(Value e)
{
    return get_static_object_element(e, 0);
}

Force new_symbol_not_found(Value msg)
{
    return create_object1(*type::SymbolNotFound, msg);
}

Force symbol_not_found_message(Value e)
{
    return get_static_object_element(e, 0);
}

Force new_illegal_argument(Value msg)
{
    return create_object1(*type::IllegalArgument, msg);
}

Force illegal_argument_message(Value e)
{
    return get_static_object_element(e, 0);
}

Force new_illegal_state(Value msg)
{
    return create_object1(*type::IllegalState, msg);
}

Force illegal_state_message(Value e)
{
    return get_static_object_element(e, 0);
}

Force new_file_not_found(Value msg)
{
    return create_object1(*type::FileNotFound, msg);
}

Force file_not_found_message(Value e)
{
    return get_static_object_element(e, 0);
}

Force new_arithmetic_exception(Value msg)
{
    return create_object1(*type::ArithmeticException, msg);
}

Force arithmetic_exception_message(Value e)
{
    return get_static_object_element(e, 0);
}

Force new_index_out_of_bounds()
{
    return create_object0(*type::IndexOutOfBounds);
}

Force index_out_of_bounds_message(Value)
{
    return nil;
}

Force new_compilation_error(Value msg)
{
    return create_object1(*type::CompilationError, msg);
}

Force compilation_error_message(Value e)
{
    return get_static_object_element(e, 0);
}

Force new_stack_overflow()
{
    return create_object0(*type::StackOverflow);
}

Force stack_overflow_message(Value e)
{
    return nil;
}

}
