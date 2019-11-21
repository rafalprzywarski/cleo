#include "list.hpp"
#include "global.hpp"
#include <array>

namespace cleo
{

Force create_list(const Value *elems, std::uint32_t size)
{
    if (size == 0)
        return create_static_object(*type::List, 0, nil, nil);

    Root list;
    for (auto i = size; i > 0; --i)
        list = create_static_object(*type::List, size - i + 1, elems[i - 1], *list);

    return force(*list);
}

Int64 get_list_size(Value list)
{
    return get_static_object_int(list, 0);
}

Value get_list_first(Value list)
{
    return get_static_object_element(list, 1);
}

Value get_list_next(Value list)
{
    return get_static_object_element(list, 2);
}

Force list_conj(Value list, Value elem)
{
    auto size = get_list_size(list);
    return create_static_object(*type::List, size + 1, elem, size ? list : nil);
}

Value list_seq(Value list)
{
    return get_list_size(list) == 0 ? nil : list;
}

}
