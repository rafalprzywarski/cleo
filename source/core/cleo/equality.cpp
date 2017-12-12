#include "equality.hpp"
#include "small_vector.hpp"
#include "small_map.hpp"
#include "global.hpp"
#include "var.hpp"
#include "multimethod.hpp"
#include <array>

namespace cleo
{

Value are_small_vectors_equal(Value left, Value right)
{
    auto size = get_small_vector_size(left);
    if (size != get_small_vector_size(right))
        return nil;
    for (decltype(size) i = 0; i < size; ++i)
        if (!are_equal(get_small_vector_elem(left, i), get_small_vector_elem(right, i)))
            return nil;
    return TRUE;
}

Value are_seqs_equal(Value left_, Value right_)
{
    Root left(force(left_));
    Root right(force(right_));
    auto first = lookup(FIRST);
    auto next = lookup(NEXT);

    Root lf, rf;
    for (; *left != nil && *right != nil; left = call_multimethod1(next, *left), right = call_multimethod1(next, *right))
    {
        lf = call_multimethod1(first, *left);
        rf = call_multimethod1(first, *right);
        if (!are_equal(*lf, *rf))
            return nil;
    }
    return *left == *right ? TRUE : nil;
}

Value are_seqables_equal(Value left, Value right)
{
    auto seq = lookup(SEQ);
    Root ls{call_multimethod1(seq, left)};
    Root rs{call_multimethod1(seq, right)};
    return are_seqs_equal(*ls, *rs);
}

Value are_small_maps_equal(Value left, Value right)
{
    auto size = get_small_map_size(left);
    if (size != get_small_map_size(right))
        return nil;

    for (decltype(size) i = 0; i != size; ++i)
    {
        if (are_equal(small_map_get(right, get_small_map_key(left, i)), get_small_map_val(left, i)) == nil)
            return nil;
    }

    return TRUE;
}

Value are_equal(Value left, Value right)
{
    if (left == right)
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
            if (get_object_type(left) == type::SMALL_VECTOR && get_object_type(right) == type::SMALL_VECTOR)
                return are_small_vectors_equal(left, right);
            {
                std::array<Value, 2> args{{left, right}};
                Root ret{call_multimethod(lookup(OBJ_EQ), args.data(), args.size())};
                return *ret;
            }
        default:
            return nil;
    }
}

}
