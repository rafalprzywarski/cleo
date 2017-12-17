#include "error.hpp"
#include "global.hpp"

namespace cleo
{

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

Force new_unexpected_end_of_input()
{
    return create_object0(type::UnexpectedEndOfInput);
}

Force read_error_message(Value e)
{
    return get_object_element(e, 0);
}

Force unexpected_end_of_input_message(Value)
{
    return create_string("unexpected end of input");
}

}
