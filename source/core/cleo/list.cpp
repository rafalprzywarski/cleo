#include "list.hpp"
#include "global.hpp"
#include <array>

namespace cleo
{

Force create_list(const Value *elems, std::uint32_t size)
{
    Root cons;
    for (auto i = size; i > 0; --i)
        cons = create_object2(type::CONS, elems[i - 1], *cons);

    Root size_{create_int64(size)};
    return create_object2(type::LIST, *size_, *cons);
}

Value get_list_size(Value list)
{
    return get_object_element(list, 0);
}

Value get_list_first(Value list)
{
    auto cons = get_object_element(list, 1);
    if (cons == nil)
        return nil;
    return get_object_element(get_object_element(list, 1), 0);
}

Force get_list_next(Value list)
{
    auto size = get_int64_value(get_list_size(list));
    if (size <= 1)
        return nil;
    auto cons = get_object_element(get_object_element(list, 1), 1);
    Root new_size{create_int64(size - 1)};
    return create_object2(type::LIST, *new_size, cons);
}

Force list_conj(Value list, Value elem)
{
    Root cons, size;
    cons = create_object2(type::CONS, elem, get_object_element(list, 1));
    size = create_int64(get_int64_value(get_list_size(list)) + 1);
    return create_object2(type::LIST, *size, *cons);
}

Value list_seq(Value list)
{
    return get_int64_value(get_list_size(list)) == 0 ? nil : list;
}

}
