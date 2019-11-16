#include "equality.hpp"
#include "array.hpp"
#include "array_set.hpp"
#include "array_map.hpp"
#include "global.hpp"
#include "var.hpp"
#include "multimethod.hpp"
#include <array>
#include <cstring>

namespace cleo
{

Value are_arrays_equal(Value left, Value right)
{
    auto size = get_array_size(left);
    if (size != get_array_size(right))
        return nil;
    for (decltype(size) i = 0; i < size; ++i)
        if (!are_equal(get_array_elem(left, i), get_array_elem(right, i)))
            return nil;
    return TRUE;
}

Value are_seqs_equal(Value left_, Value right_)
{
    Root left(force(left_));
    Root right(force(right_));
    Root lf, rf;
    for (; *left && *right; left = call_multimethod1(*rt::next, *left), right = call_multimethod1(*rt::next, *right))
    {
        lf = call_multimethod1(*rt::first, *left);
        rf = call_multimethod1(*rt::first, *right);
        if (!are_equal(*lf, *rf))
            return nil;
    }
    return left->is(*right) ? TRUE : nil;
}

Value are_seqables_equal(Value left, Value right)
{
    Root ls{call_multimethod1(*rt::seq, left)};
    Root rs{call_multimethod1(*rt::seq, right)};
    return are_seqs_equal(*ls, *rs);
}

Value are_array_sets_equal(Value left, Value right)
{
    auto size = get_array_set_size(left);
    if (size != get_array_set_size(right))
        return nil;

    for (decltype(size) i = 0; i != size; ++i)
        if (!array_set_contains(right, get_array_set_elem(left, i)))
            return nil;

    return TRUE;
}

Value are_array_maps_equal(Value left, Value right)
{
    auto size = get_array_map_size(left);
    if (size != get_array_map_size(right))
        return nil;

    for (decltype(size) i = 0; i != size; ++i)
    {
        if (!are_equal(array_map_get(right, get_array_map_key(left, i), *SENTINEL), get_array_map_val(left, i)))
            return nil;
    }

    return TRUE;
}

Value are_maps_equal(Value left, Value right)
{
    Root left_count{call_multimethod1(*rt::count, left)};
    Root right_count{call_multimethod1(*rt::count, right)};
    if (*left_count != *right_count)
        return nil;
    for (Root seq{call_multimethod1(*rt::seq, left)}; *seq; seq = call_multimethod1(*rt::next, *seq))
    {
        Root kv{call_multimethod1(*rt::first, *seq)};
        Root other_v{call_multimethod2(*rt::get, right, get_array_elem(*kv, 0))};
        if (*other_v != get_array_elem(*kv, 1))
            return nil;
    }
    return TRUE;
}

Value are_equal(Value left, Value right)
{
    if (left.is(right))
        return TRUE;

    auto left_tag = get_value_tag(left);
    auto right_tag = get_value_tag(right);

    if (left_tag != right_tag)
        return nil;

    switch (left_tag)
    {
        case tag::INT64:
            return get_int64_value(left) == get_int64_value(right) ? TRUE : nil;
        case tag::FLOAT64:
            return get_float64_value(left) == get_float64_value(right) ? TRUE : nil;
        case tag::STRING:
            return
                get_string_len(left) == get_string_len(right) &&
                std::memcmp(get_string_ptr(left), get_string_ptr(right), get_string_len(left)) == 0 ?
                TRUE :
                nil;
        case tag::OBJECT:
            if (left.is_nil() || right.is_nil())
                return nil;
            if (get_object_type(left).is(*type::Array) && get_object_type(right).is(*type::Array))
                return are_arrays_equal(left, right);
            {
                std::array<Value, 2> args{{left, right}};
                Root ret{call_multimethod(*rt::obj_eq, args.data(), args.size())};
                return *ret;
            }
        default:
            return nil;
    }
}

}
