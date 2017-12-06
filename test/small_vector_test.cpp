#include <cleo/small_vector.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

TEST(small_vector_test, should_create_an_empty_vector)
{
    Value vector = create_small_vector(nullptr, 0);
    ASSERT_EQ(0u, get_small_vector_size(vector));
    ASSERT_TRUE(nil == get_small_vector_elem(vector, 0));
}

TEST(small_vector_test, should_create_a_vector_from_elements)
{
    Value elem = create_int64(11);
    Value vector = create_small_vector(&elem, 1);
    ASSERT_EQ(1, get_small_vector_size(vector));
    ASSERT_TRUE(elem == get_small_vector_elem(vector, 0));
    ASSERT_TRUE(nil == get_small_vector_elem(vector, 1));

    std::array<Value, 3> elems{{
        create_int64(101),
        create_int64(102),
        create_int64(103)
    }};
    vector = create_small_vector(elems.data(), elems.size());
    ASSERT_EQ(elems.size(), get_small_vector_size(vector));
    ASSERT_TRUE(elems[0] == get_small_vector_elem(vector, 0));
    ASSERT_TRUE(elems[1] == get_small_vector_elem(vector, 1));
    ASSERT_TRUE(elems[2] == get_small_vector_elem(vector, 2));
    ASSERT_TRUE(nil == get_small_vector_elem(vector, 3));
}

TEST(small_vector_test, seq_should_return_nil_for_an_empty)
{
    EXPECT_TRUE(nil == small_vector_seq(svec()));
}

TEST(small_vector_test, seq_should_return_a_sequence_of_the_vector_elements)
{
    Value elem = create_int64(11);
    Value seq = small_vector_seq(svec(elem));
    ASSERT_TRUE(elem == get_small_vector_seq_first(seq));
    ASSERT_TRUE(nil == get_small_vector_seq_next(seq));

    std::array<Value, 3> elems{{
        create_int64(101),
        create_int64(102),
        create_int64(103)
    }};
    seq = small_vector_seq(create_small_vector(elems.data(), elems.size()));
    ASSERT_TRUE(elems[0] == get_small_vector_seq_first(seq));

    seq = get_small_vector_seq_next(seq);
    ASSERT_TRUE(elems[1] == get_small_vector_seq_first(seq));

    seq = get_small_vector_seq_next(seq);
    ASSERT_TRUE(elems[2] == get_small_vector_seq_first(seq));
    ASSERT_TRUE(nil == get_small_vector_seq_next(seq));
}

}
}
