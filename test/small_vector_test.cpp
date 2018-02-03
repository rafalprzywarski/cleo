#include <cleo/small_vector.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct small_vector_test : Test
{
    small_vector_test() : Test("cleo.small-vector.test") { }
};

TEST_F(small_vector_test, should_create_an_empty_vector)
{
    Root vector{create_small_vector(nullptr, 0)};
    ASSERT_EQ(0u, get_small_vector_size(*vector));
    ASSERT_TRUE(get_small_vector_elem(*vector, 0).is_nil());
}

TEST_F(small_vector_test, should_create_a_vector_from_elements)
{
    Root elem{create_int64(11)};
    auto elemVal = *elem;
    Root vector{create_small_vector(&elemVal, 1)};
    ASSERT_EQ(1, get_small_vector_size(*vector));
    ASSERT_TRUE(elem->is(get_small_vector_elem(*vector, 0)));
    ASSERT_TRUE(get_small_vector_elem(*vector, 1).is_nil());

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    vector = create_small_vector(elems.data(), elems.size());
    ASSERT_EQ(elems.size(), get_small_vector_size(*vector));
    ASSERT_TRUE(elems[0].is(get_small_vector_elem(*vector, 0)));
    ASSERT_TRUE(elems[1].is(get_small_vector_elem(*vector, 1)));
    ASSERT_TRUE(elems[2].is(get_small_vector_elem(*vector, 2)));
    ASSERT_TRUE(get_small_vector_elem(*vector, 3).is_nil());
}

TEST_F(small_vector_test, seq_should_return_nil_for_an_empty)
{
    Root v{svec()};
    EXPECT_TRUE(Root(small_vector_seq(*v))->is_nil());
}

TEST_F(small_vector_test, seq_should_return_a_sequence_of_the_vector_elements)
{
    Root elem{create_int64(11)};
    Root v{svec(*elem)};
    Root seq{small_vector_seq(*v)};
    ASSERT_TRUE(elem->is(get_small_vector_seq_first(*seq)));
    ASSERT_TRUE(Root(get_small_vector_seq_next(*seq))->is_nil());

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    v = svec(*elem0, *elem1, *elem2);
    seq = small_vector_seq(*v);
    ASSERT_TRUE(elem0->is(get_small_vector_seq_first(*seq)));

    seq = get_small_vector_seq_next(*seq);
    ASSERT_TRUE(elem1->is(get_small_vector_seq_first(*seq)));

    seq = get_small_vector_seq_next(*seq);
    ASSERT_TRUE(elem2->is(get_small_vector_seq_first(*seq)));
    ASSERT_TRUE(Root(get_small_vector_seq_next(*seq))->is_nil());
}

TEST_F(small_vector_test, should_conj_elements_at_the_end_of_the_vector)
{
    Root vec0{create_small_vector(nullptr, 0)};
    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    Root vec1{small_vector_conj(*vec0, *elem0)};
    Root vec2{small_vector_conj(*vec1, *elem1)};
    Root vec3{small_vector_conj(*vec2, *elem2)};

    ASSERT_EQ(1u, get_small_vector_size(*vec1));
    ASSERT_TRUE(elem0->is(get_small_vector_elem(*vec1, 0)));

    ASSERT_EQ(2u, get_small_vector_size(*vec2));
    ASSERT_TRUE(elem0->is(get_small_vector_elem(*vec2, 0)));
    ASSERT_TRUE(elem1->is(get_small_vector_elem(*vec2, 1)));

    ASSERT_EQ(3u, get_small_vector_size(*vec3));
    ASSERT_TRUE(elem0->is(get_small_vector_elem(*vec3, 0)));
    ASSERT_TRUE(elem1->is(get_small_vector_elem(*vec3, 1)));
    ASSERT_TRUE(elem2->is(get_small_vector_elem(*vec3, 2)));
}

}
}
