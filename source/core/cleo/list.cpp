#include "list.hpp"
#include "global.hpp"
#include <array>

namespace cleo
{

Value create_list(const Value *elems, std::uint32_t size)
{
    Value cons = nil;
    for (auto i = size; i > 0; --i)
    {
        std::array<Value, 2> cons_elems{{elems[i - 1], cons}};
        cons = create_object(type::CONS, cons_elems.data(), cons_elems.size());
    }

    std::array<Value, 2> list_elems{{create_int64(size), cons}};
    return create_object(type::LIST, list_elems.data(), list_elems.size());
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
    std::array<Value, 2> list_elems{{create_int64(size - 1), cons}};
    return create_object(type::LIST, list_elems.data(), list_elems.size());
}

Value list_conj(Value list, Value elem)
{
    std::array<Value, 2> cons_elems{{elem, get_object_element(list, 1)}};
    auto cons = create_object(type::CONS, cons_elems.data(), cons_elems.size());

    auto size = get_int64_value(get_list_size(list));
    std::array<Value, 2> list_elems{{create_int64(size + 1), cons}};
    return create_object(type::LIST, list_elems.data(), list_elems.size());
}

}
