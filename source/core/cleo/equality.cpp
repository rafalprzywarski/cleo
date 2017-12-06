#include "equality.hpp"
#include "small_vector.hpp"
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

Value are_seqs_equal(Value left, Value right)
{
    auto first = lookup(FIRST);
    auto next = lookup(NEXT);

    for (; left != nil && right != nil; left = call_multimethod(next, &left, 1), right = call_multimethod(next, &right, 1))
        if (!are_equal(call_multimethod(first, &left, 1), call_multimethod(first, &right, 1)))
            return nil;
    return left == right;
}

Value are_seqables_equal(Value left, Value right)
{
    auto seq = lookup(SEQ);
    return are_seqs_equal(call_multimethod(seq, &left, 1), call_multimethod(seq, &right, 1));
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
            return get_int64_value(left) == get_int64_value(right);
        case tag::FLOAT64:
            return get_float64_value(left) == get_float64_value(right);
        case tag::STRING:
            return
                get_string_len(left) == get_string_len(right) &&
                std::memcmp(get_string_ptr(left), get_string_ptr(right), get_string_len(left)) == 0;
        case tag::OBJECT:
            if (get_object_type(left) == type::SMALL_VECTOR && get_object_type(right) == type::SMALL_VECTOR)
                return are_small_vectors_equal(left, right);
            {
                std::array<Value, 2> args{{left, right}};
                return call_multimethod(lookup(OBJ_EQ), args.data(), args.size());
            }
        default:
            return nil;
    }
}

}
