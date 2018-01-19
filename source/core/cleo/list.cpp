#include "list.hpp"
#include "global.hpp"
#include <array>

namespace cleo
{

Force create_list(const Value *elems, std::uint32_t size)
{
    if (size == 0)
    {
        Root size_{create_int64(0)};
        return create_object3(*type::List, *size_, nil, nil);
    }

    Root list, size_;
    for (auto i = size; i > 0; --i)
    {
        size_ = create_int64(size - i + 1);
        list = create_object3(*type::List, *size_, elems[i - 1], *list);
    }

    return force(*list);
}

Value get_list_size(Value list)
{
    return get_object_element(list, 0);
}

Value get_list_first(Value list)
{
    return get_object_element(list, 1);
}

Value get_list_next(Value list)
{
    return get_object_element(list, 2);
}

Force list_conj(Value list, Value elem)
{
    auto size = get_int64_value(get_list_size(list));
    if (size == 0)
        list = nil;
    Root new_size{create_int64(size + 1)};
    return create_object3(*type::List, *new_size, elem, list);
}

Value list_seq(Value list)
{
    return get_int64_value(get_list_size(list)) == 0 ? nil : list;
}

}
