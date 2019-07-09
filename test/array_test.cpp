#include <cleo/array.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct array_test : Test
{
    array_test() : Test("cleo.array.test") { }
};

TEST_F(array_test, should_create_an_empty_vector)
{
    Root vector{create_array(nullptr, 0)};
    ASSERT_EQ(0u, get_array_size(*vector));
    ASSERT_TRUE(get_array_elem(*vector, 0).is_nil());
}

TEST_F(array_test, should_create_a_vector_from_elements)
{
    Root elem{create_int64(11)};
    auto elemVal = *elem;
    Root vector{create_array(&elemVal, 1)};
    ASSERT_EQ(1u, get_array_size(*vector));
    ASSERT_TRUE(elem->is(get_array_elem(*vector, 0)));
    ASSERT_TRUE(get_array_elem(*vector, 1).is_nil());

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    vector = create_array(elems.data(), elems.size());
    ASSERT_EQ(elems.size(), get_array_size(*vector));
    ASSERT_TRUE(elems[0].is(get_array_elem(*vector, 0)));
    ASSERT_TRUE(elems[1].is(get_array_elem(*vector, 1)));
    ASSERT_TRUE(elems[2].is(get_array_elem(*vector, 2)));
    ASSERT_TRUE(get_array_elem(*vector, 3).is_nil());
}

TEST_F(array_test, seq_should_return_nil_for_an_empty)
{
    Root v{array()};
    EXPECT_TRUE(Root(array_seq(*v))->is_nil());
}

TEST_F(array_test, seq_should_return_a_sequence_of_the_vector_elements)
{
    Root elem{create_int64(11)};
    Root v{array(*elem)};
    Root seq{array_seq(*v)};
    ASSERT_TRUE(elem->is(get_array_seq_first(*seq)));
    ASSERT_TRUE(Root(get_array_seq_next(*seq))->is_nil());

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    v = array(*elem0, *elem1, *elem2);
    seq = array_seq(*v);
    ASSERT_TRUE(elem0->is(get_array_seq_first(*seq)));

    seq = get_array_seq_next(*seq);
    ASSERT_TRUE(elem1->is(get_array_seq_first(*seq)));

    seq = get_array_seq_next(*seq);
    ASSERT_TRUE(elem2->is(get_array_seq_first(*seq)));
    ASSERT_TRUE(Root(get_array_seq_next(*seq))->is_nil());
}

TEST_F(array_test, should_conj_elements_at_the_end_of_the_vector)
{
    Root vec0{create_array(nullptr, 0)};
    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    Root vec1{array_conj(*vec0, *elem0)};
    Root vec2{array_conj(*vec1, *elem1)};
    Root vec3{array_conj(*vec2, *elem2)};

    ASSERT_EQ(1u, get_array_size(*vec1));
    ASSERT_TRUE(elem0->is(get_array_elem(*vec1, 0)));

    ASSERT_EQ(2u, get_array_size(*vec2));
    ASSERT_TRUE(elem0->is(get_array_elem(*vec2, 0)));
    ASSERT_TRUE(elem1->is(get_array_elem(*vec2, 1)));

    ASSERT_EQ(3u, get_array_size(*vec3));
    ASSERT_TRUE(elem0->is(get_array_elem(*vec3, 0)));
    ASSERT_TRUE(elem1->is(get_array_elem(*vec3, 1)));
    ASSERT_TRUE(elem2->is(get_array_elem(*vec3, 2)));
}

struct transient_array_test : Test
{
    transient_array_test() : Test("cleo.transient-array.test") { }
};

TEST_F(transient_array_test, should_create_an_empty_vector)
{
    Root p{create_array(nullptr, 0)};
    Root vector{transient_array(*p)};
    ASSERT_EQ(0, get_transient_array_size(*vector));
    ASSERT_TRUE(get_transient_array_elem(*vector, 0).is_nil());
}

TEST_F(transient_array_test, should_create_a_persistent_vector_from_a_vector)
{
    Root elem{create_int64(11)};
    auto elemVal = *elem;
    Root p{create_array(&elemVal, 1)};
    Root vector{transient_array(*p)};
    ASSERT_EQ(1, get_transient_array_size(*vector));
    ASSERT_TRUE(elem->is(get_transient_array_elem(*vector, 0)));
    ASSERT_TRUE(get_transient_array_elem(*vector, 1).is_nil());

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    p = create_array(elems.data(), elems.size());
    vector = transient_array(*p);
    ASSERT_EQ(Int64(elems.size()), get_transient_array_size(*vector));
    ASSERT_TRUE(elems[0].is(get_transient_array_elem(*vector, 0)));
    ASSERT_TRUE(elems[1].is(get_transient_array_elem(*vector, 1)));
    ASSERT_TRUE(elems[2].is(get_transient_array_elem(*vector, 2)));
    ASSERT_TRUE(get_transient_array_elem(*vector, 3).is_nil());
}

TEST_F(transient_array_test, should_conj_elements_at_the_end_of_the_vector)
{
    Root p{create_array(nullptr, 0)};
    Root vec0{transient_array(*p)};
    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    Root vec1{transient_array_conj(*vec0, *elem0)};

    ASSERT_EQ(1, get_transient_array_size(*vec1));
    ASSERT_TRUE(elem0->is(get_transient_array_elem(*vec1, 0)));

    Root vec2{transient_array_conj(*vec1, *elem1)};

    ASSERT_EQ(2, get_transient_array_size(*vec2));
    ASSERT_TRUE(elem0->is(get_transient_array_elem(*vec2, 0)));
    ASSERT_TRUE(elem1->is(get_transient_array_elem(*vec2, 1)));

    Root vec3{transient_array_conj(*vec2, *elem2)};

    ASSERT_EQ(3, get_transient_array_size(*vec3));
    ASSERT_TRUE(elem0->is(get_transient_array_elem(*vec3, 0)));
    ASSERT_TRUE(elem1->is(get_transient_array_elem(*vec3, 1)));
    ASSERT_TRUE(elem2->is(get_transient_array_elem(*vec3, 2)));
}

TEST_F(transient_array_test, conj_should_reallocate_when_capacity_is_exceeded)
{
    Root p{create_array(nullptr, 0)};
    Root vector{transient_array(*p)};
    Root n;
    for (int i = 0; i < 129; ++i)
    {
        n = i64(i + 1000);
        vector = transient_array_conj(*vector, *n);
    }

    ASSERT_EQ(129, get_transient_array_size(*vector));
    ASSERT_EQ(1000, get_int64_value(get_transient_array_elem(*vector, 0)));
    ASSERT_EQ(1001, get_int64_value(get_transient_array_elem(*vector, 1)));
    ASSERT_EQ(1031, get_int64_value(get_transient_array_elem(*vector, 31)));
    ASSERT_EQ(1032, get_int64_value(get_transient_array_elem(*vector, 32)));
    ASSERT_EQ(1127, get_int64_value(get_transient_array_elem(*vector, 127)));
    ASSERT_EQ(1128, get_int64_value(get_transient_array_elem(*vector, 128)));
}

TEST_F(transient_array_test, should_change_a_transient_vector_into_a_persistent_one)
{
    Root p{create_array(nullptr, 0)};
    Root vector{transient_array(*p)};
    Root pv{transient_array_persistent(*vector)};
    EXPECT_EQ_VALS(*p, *pv);

    p = array(10, 12, 13);
    vector = transient_array(*p);
    pv = transient_array_persistent(*vector);
    EXPECT_EQ_VALS(*p, *pv);
}

}
}
