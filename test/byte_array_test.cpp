#include <cleo/byte_array.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct byte_array_test : Test
{
    byte_array_test() : Test("cleo.byte-array.test") { }
};

TEST_F(byte_array_test, should_create_an_empty_array)
{
    Root a{create_byte_array(nullptr, 0)};
    ASSERT_EQ(0, get_byte_array_size(*a));
    ASSERT_TRUE(get_byte_array_elem(*a, 0).is_nil());
    ASSERT_TRUE(get_byte_array_elem(*a, -1).is_nil());
}

TEST_F(byte_array_test, should_create_an_array_from_elements)
{
    Root elem{create_int64(11)};
    auto elemVal = *elem;
    Root vector{create_byte_array(&elemVal, 1)};
    ASSERT_EQ(1, get_byte_array_size(*vector));
    EXPECT_EQ_VALS(*elem, get_byte_array_elem(*vector, 0));
    EXPECT_TRUE(get_byte_array_elem(*vector, 1).is_nil());

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    vector = create_byte_array(elems.data(), elems.size());
    ASSERT_EQ(Int64(elems.size()), get_byte_array_size(*vector));
    EXPECT_TRUE(get_byte_array_elem(*vector, -1).is_nil());
    EXPECT_EQ_VALS(elems[0], get_byte_array_elem(*vector, 0));
    EXPECT_EQ_VALS(elems[1], get_byte_array_elem(*vector, 1));
    EXPECT_EQ_VALS(elems[2], get_byte_array_elem(*vector, 2));
    EXPECT_TRUE(get_byte_array_elem(*vector, 3).is_nil());
}

TEST_F(byte_array_test, seq_should_return_nil_for_an_empty_array)
{
    Root v{create_byte_array(nullptr, 0)};
    EXPECT_TRUE(Root(byte_array_seq(*v))->is_nil());
}

TEST_F(byte_array_test, seq_should_return_a_sequence_of_the_elements)
{
    Root elem{create_int64(11)};
    auto elemVal = *elem;
    Root v{create_byte_array(&elemVal, 1)};
    Root seq{byte_array_seq(*v)};
    EXPECT_EQ_VALS(elemVal, get_byte_array_seq_first(*seq));
    EXPECT_TRUE(Root(get_byte_array_seq_next(*seq))->is_nil());

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    v = create_byte_array(elems.data(), elems.size());
    seq = byte_array_seq(*v);
    EXPECT_EQ_VALS(*elem0, get_byte_array_seq_first(*seq));

    seq = get_byte_array_seq_next(*seq);
    EXPECT_EQ_VALS(*elem1, get_byte_array_seq_first(*seq));

    seq = get_byte_array_seq_next(*seq);
    EXPECT_EQ_VALS(*elem2, get_byte_array_seq_first(*seq));
    EXPECT_TRUE(Root(get_byte_array_seq_next(*seq))->is_nil());
}

TEST_F(byte_array_test, should_conj_elements_at_the_end_of_the_array)
{
    Root vec0{create_byte_array(nullptr, 0)};
    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    Root vec1{byte_array_conj(*vec0, *elem0)};
    Root vec2{byte_array_conj(*vec1, *elem1)};
    Root vec3{byte_array_conj(*vec2, *elem2)};

    ASSERT_EQ(1, get_byte_array_size(*vec1));
    EXPECT_EQ_VALS(*elem0, get_byte_array_elem(*vec1, 0));

    ASSERT_EQ(2, get_byte_array_size(*vec2));
    EXPECT_EQ_VALS(*elem0, get_byte_array_elem(*vec2, 0));
    EXPECT_EQ_VALS(*elem1, get_byte_array_elem(*vec2, 1));

    ASSERT_EQ(3, get_byte_array_size(*vec3));
    EXPECT_EQ_VALS(*elem0, get_byte_array_elem(*vec3, 0));
    EXPECT_EQ_VALS(*elem1, get_byte_array_elem(*vec3, 1));
    EXPECT_EQ_VALS(*elem2, get_byte_array_elem(*vec3, 2));
}

TEST_F(byte_array_test, should_pop_elements_from_the_end_of_the_vector)
{
    Root a{create_int64(137)};
    Root b{create_int64(147)};
    Root c{create_int64(157)};
    Root vec1{byte_array_conj(*EMPTY_BYTE_ARRAY, *a)};
    Root vec2{byte_array_conj(*vec1, *b)};
    Root vec3{byte_array_conj(*vec2, *c)};
    Root pvec2{byte_array_pop(*vec3)};
    Root pvec1{byte_array_pop(*pvec2)};
    Root pvec0{byte_array_pop(*pvec1)};

    EXPECT_EQ_VALS(*vec2, *pvec2);
    EXPECT_EQ_VALS(*vec1, *pvec1);
    EXPECT_EQ_REFS(*EMPTY_BYTE_ARRAY, *pvec0);

    try
    {
        Root empty{create_byte_array(nullptr, 0)};
        array_pop(*empty);
        FAIL() << "array_pop should fail for an empty array";
    }
    catch (Exception const& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::IllegalState, get_value_type(*e));
    }
}

struct transient_byte_array_test : Test
{
    transient_byte_array_test() : Test("cleo.transient-byte-array.test") { }
};

TEST_F(transient_byte_array_test, should_create_an_empty_array)
{
    Root p{create_byte_array(nullptr, 0)};
    Root vector{transient_byte_array(*p)};
    ASSERT_EQ(0, get_transient_byte_array_size(*vector));
    ASSERT_TRUE(get_transient_byte_array_elem(*vector, -1).is_nil());
    ASSERT_TRUE(get_transient_byte_array_elem(*vector, 0).is_nil());
}

TEST_F(transient_byte_array_test, should_create_a_persistent_array_from_a_transient_array)
{
    Root elem{create_int64(11)};
    auto elemVal = *elem;
    Root p{create_byte_array(&elemVal, 1)};
    Root vector{transient_byte_array(*p)};
    ASSERT_EQ(1, get_transient_byte_array_size(*vector));
    EXPECT_EQ_VALS(*elem, get_transient_byte_array_elem(*vector, 0));
    EXPECT_TRUE(get_transient_byte_array_elem(*vector, 1).is_nil());

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    p = create_byte_array(elems.data(), elems.size());
    vector = transient_byte_array(*p);
    ASSERT_EQ(Int64(elems.size()), get_transient_byte_array_size(*vector));
    EXPECT_EQ_VALS(elems[0], get_transient_byte_array_elem(*vector, 0));
    EXPECT_EQ_VALS(elems[1], get_transient_byte_array_elem(*vector, 1));
    EXPECT_EQ_VALS(elems[2], get_transient_byte_array_elem(*vector, 2));
    EXPECT_TRUE(get_transient_byte_array_elem(*vector, 3).is_nil());
}

TEST_F(transient_byte_array_test, should_conj_elements_at_the_end_of_the_vector)
{
    Root p{create_byte_array(nullptr, 0)};
    Root vec0{transient_byte_array(*p)};
    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    Root vec1{transient_byte_array_conj(*vec0, *elem0)};

    ASSERT_EQ(1, get_transient_byte_array_size(*vec1));
    EXPECT_EQ_VALS(*elem0, get_transient_byte_array_elem(*vec1, 0));

    Root vec2{transient_byte_array_conj(*vec1, *elem1)};

    ASSERT_EQ(2, get_transient_byte_array_size(*vec2));
    EXPECT_EQ_VALS(*elem0, get_transient_byte_array_elem(*vec2, 0));
    EXPECT_EQ_VALS(*elem1, get_transient_byte_array_elem(*vec2, 1));

    Root vec3{transient_byte_array_conj(*vec2, *elem2)};

    ASSERT_EQ(3, get_transient_byte_array_size(*vec3));
    EXPECT_EQ_VALS(*elem0, get_transient_byte_array_elem(*vec3, 0));
    EXPECT_EQ_VALS(*elem1, get_transient_byte_array_elem(*vec3, 1));
    EXPECT_EQ_VALS(*elem2, get_transient_byte_array_elem(*vec3, 2));
}

TEST_F(transient_byte_array_test, should_pop_elements_from_the_end_of_the_vector)
{
    Root a{create_int64(201)};
    Root b{create_int64(202)};
    Root c{create_int64(203)};
    Root vec1{byte_array_conj(*EMPTY_BYTE_ARRAY, *a)};
    Root vec2{byte_array_conj(*vec1, *b)};
    Root vec3{byte_array_conj(*vec2, *c)};
    Root pvec2{transient_byte_array(*vec3)};
    pvec2 = transient_byte_array_pop(*pvec2);

    ASSERT_EQ(2, get_transient_byte_array_size(*pvec2));
    ASSERT_TRUE(a->is(get_transient_byte_array_elem(*pvec2, 0)));
    ASSERT_TRUE(b->is(get_transient_byte_array_elem(*pvec2, 1)));

    Root pvec1{transient_byte_array_pop(*pvec2)};

    ASSERT_EQ(1, get_transient_byte_array_size(*pvec1));
    ASSERT_TRUE(a->is(get_transient_byte_array_elem(*pvec1, 0)));

    Root pvec0{transient_byte_array_pop(*pvec1)};

    ASSERT_EQ(0, get_transient_byte_array_size(*pvec0));

    try
    {
        Root empty{create_byte_array(nullptr, 0)};
        empty = transient_byte_array(*empty);
        transient_byte_array_pop(*empty);
        FAIL() << "array_pop should fail for an empty array";
    }
    catch (Exception const& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::IllegalState, get_value_type(*e));
    }
}

TEST_F(transient_byte_array_test, conj_should_reallocate_when_capacity_is_exceeded)
{
    Root p{create_byte_array(nullptr, 0)};
    Root vector{transient_byte_array(*p)};
    Root n;
    for (int i = 0; i < 129; ++i)
    {
        n = i64(i + 100);
        vector = transient_byte_array_conj(*vector, *n);
    }

    ASSERT_EQ(129, get_transient_byte_array_size(*vector));
    ASSERT_EQ(100, get_int64_value(get_transient_byte_array_elem(*vector, 0)));
    ASSERT_EQ(101, get_int64_value(get_transient_byte_array_elem(*vector, 1)));
    ASSERT_EQ(131, get_int64_value(get_transient_byte_array_elem(*vector, 31)));
    ASSERT_EQ(132, get_int64_value(get_transient_byte_array_elem(*vector, 32)));
    ASSERT_EQ(227, get_int64_value(get_transient_byte_array_elem(*vector, 127)));
    ASSERT_EQ(228, get_int64_value(get_transient_byte_array_elem(*vector, 128)));
}

TEST_F(transient_byte_array_test, should_change_a_transient_vector_into_a_persistent_one)
{
    Root p{create_byte_array(nullptr, 0)};
    Root vector{transient_byte_array(*p)};
    Root pv{transient_byte_array_persistent(*vector)};
    EXPECT_EQ_VALS(*p, *pv);

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    p = create_byte_array(elems.data(), elems.size());
    vector = transient_byte_array(*p);
    pv = transient_byte_array_persistent(*vector);
    EXPECT_EQ_VALS(*p, *pv);
}

}
}
