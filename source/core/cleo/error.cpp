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
        Value text = exception_message(*current_exception);
        if (!get_value_type(text).is(*type::UTF8String))
            return "";
        buffer = std::string(cleo::get_string_ptr(text), cleo::get_string_size(text));
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

Value exception_message(Value e)
{
    return get_static_object_element(e, 0);
}

Value logic_exception_callstack(Value e)
{
    return get_static_object_element(e, 1);
}

Force new_read_error(Value msg, Value line, Value column)
{
    return create_object3(*type::ReadError, msg, line, column);
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

Force new_call_error(Value msg)
{
    Root s{current_callstack()};
    return create_object2(*type::CallError, msg, *s);
}

Force new_symbol_not_found(Value msg)
{
    return create_object1(*type::SymbolNotFound, msg);
}

Force new_illegal_argument(Value msg)
{
    Root s{current_callstack()};
    return create_object2(*type::IllegalArgument, msg, *s);
}

Force new_illegal_state(Value msg)
{
    Root s{current_callstack()};
    return create_object2(*type::IllegalState, msg, *s);
}

Force new_file_not_found(Value msg)
{
    return create_object1(*type::FileNotFound, msg);
}

Force new_arithmetic_exception(Value msg)
{
    Root s{current_callstack()};
    return create_object2(*type::ArithmeticException, msg, *s);
}

Force new_index_out_of_bounds()
{
    Root s{current_callstack()};
    return create_static_object(*type::IndexOutOfBounds, nil, *s);
}

Force new_compilation_error(Value msg)
{
    return create_object1(*type::CompilationError, msg);
}

Force new_stack_overflow()
{
    Root s{current_callstack()};
    return create_static_object(*type::StackOverflow, nil, *s);
}

}
