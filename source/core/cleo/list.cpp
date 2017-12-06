#include "list.hpp"
#include "global.hpp"
#include <array>

namespace cleo
{

Value create_list(const Value *elems, std::uint32_t size)
{
    Value cons = nil;
    for (auto i = size; i > 0; --i)
        cons = create_object2(type::CONS, elems[i - 1], cons);

    return create_object2(type::LIST, create_int64(size), cons);
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

Value get_list_next(Value list)
{
    auto size = get_int64_value(get_list_size(list));
    if (size <= 1)
        return nil;
    auto cons = get_object_element(get_object_element(list, 1), 1);
    return create_object2(type::LIST, create_int64(size - 1), cons);
}

Value list_conj(Value list, Value elem)
{
    auto cons = create_object2(type::CONS, elem, get_object_element(list, 1));
    auto size = get_int64_value(get_list_size(list));
    return create_object2(type::LIST, create_int64(size + 1), cons);
}

Value list_seq(Value list)
{
    return get_int64_value(get_list_size(list)) == 0 ? nil : list;
}

}
