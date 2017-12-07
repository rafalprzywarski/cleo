#include <cleo/small_vector.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct small_vector_test : Test {};

TEST_F(small_vector_test, should_create_an_empty_vector)
{
    Root vector{force(create_small_vector(nullptr, 0))};
    ASSERT_EQ(0u, get_small_vector_size(*vector));
    ASSERT_TRUE(nil == get_small_vector_elem(*vector, 0));
}

TEST_F(small_vector_test, should_create_a_vector_from_elements)
{
    Root elem{force(create_int64(11))};
    Root vector{force(create_small_vector(&*elem, 1))};
    ASSERT_EQ(1, get_small_vector_size(*vector));
    ASSERT_TRUE(*elem == get_small_vector_elem(*vector, 0));
    ASSERT_TRUE(nil == get_small_vector_elem(*vector, 1));

    Root elem0, elem1, elem2;
    *elem0 = create_int64(101);
    *elem1 = create_int64(102);
    *elem2 = create_int64(103);
    std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    *vector = create_small_vector(elems.data(), elems.size());
    ASSERT_EQ(elems.size(), get_small_vector_size(*vector));
    ASSERT_TRUE(elems[0] == get_small_vector_elem(*vector, 0));
    ASSERT_TRUE(elems[1] == get_small_vector_elem(*vector, 1));
    ASSERT_TRUE(elems[2] == get_small_vector_elem(*vector, 2));
    ASSERT_TRUE(nil == get_small_vector_elem(*vector, 3));
}

TEST_F(small_vector_test, seq_should_return_nil_for_an_empty)
{
    Root v{force(svec())};
    EXPECT_TRUE(nil == small_vector_seq(*v));
}

TEST_F(small_vector_test, seq_should_return_a_sequence_of_the_vector_elements)
{
    Root elem{force(create_int64(11))};
    Root v{force(svec(*elem))};
    Root seq{force(small_vector_seq(*v))};
    ASSERT_TRUE(*elem == get_small_vector_seq_first(*seq));
    ASSERT_TRUE(nil == get_small_vector_seq_next(*seq));

    Root elem0, elem1, elem2;
    *elem0 = create_int64(101);
    *elem1 = create_int64(102);
    *elem2 = create_int64(103);
    *v = svec(*elem0, *elem1, *elem2);
    *seq = small_vector_seq(*v);
    ASSERT_TRUE(*elem0 == get_small_vector_seq_first(*seq));

    *seq = get_small_vector_seq_next(*seq);
    ASSERT_TRUE(*elem1 == get_small_vector_seq_first(*seq));

    *seq = get_small_vector_seq_next(*seq);
    ASSERT_TRUE(*elem2 == get_small_vector_seq_first(*seq));
    ASSERT_TRUE(nil == get_small_vector_seq_next(*seq));
}

}
}
