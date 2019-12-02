#include "string_seq.hpp"
#include "global.hpp"

namespace cleo
{

Force string_seq(Value str)
{
    if (get_string_size(str) == 0)
        return nil;
    return create_static_object(*type::UTF8StringSeq, str, 0);
}

Value string_seq_first(Value s)
{
    return create_uchar(get_string_char_at_offset(get_static_object_element(s, 0), get_static_object_int(s, 1)));
}

Force string_seq_next(Value s)
{
    auto str = get_static_object_element(s, 0);
    Int64 offset = get_string_next_offset(str, get_static_object_int(s, 1));
    if (offset == get_string_size(str))
        return nil;
    return create_static_object(*type::UTF8StringSeq, str, offset);
}

}
