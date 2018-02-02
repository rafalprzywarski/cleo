#include <cleo/list.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct list_test : Test
{
    list_test() : Test("cleo.list.test") { }
};

TEST_F(list_test, should_create_an_empty_list)
{
    Root list{create_list(nullptr, 0)};
    ASSERT_EQ(0, get_int64_value(get_list_size(*list)));
    ASSERT_TRUE(get_list_first(*list).is_nil());
    ASSERT_TRUE(get_list_next(*list).is_nil());
}

TEST_F(list_test, should_create_a_list_from_elements)
{
    Root elem{create_int64(11)};
    auto elemVal = *elem;
    Root list{create_list(&elemVal, 1)};
    ASSERT_EQ(1, get_int64_value(get_list_size(*list)));
    ASSERT_TRUE(elem->is(get_list_first(*list)));
    ASSERT_TRUE(get_list_next(*list).is_nil());

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    list = create_list(elems.data(), elems.size());
    ASSERT_EQ(elems.size(), get_int64_value(get_list_size(*list)));
    ASSERT_TRUE(elems[0].is(get_list_first(*list)));

    Value next1 = get_list_next(*list);
    ASSERT_EQ(2u, get_int64_value(get_list_size(next1)));
    ASSERT_TRUE(elems[1].is(get_list_first(next1)));

    Value next2 = get_list_next(next1);
    ASSERT_EQ(1u, get_int64_value(get_list_size(next2)));
    ASSERT_TRUE(elems[2].is(get_list_first(next2)));
    ASSERT_TRUE(get_list_next(next2).is_nil());
}

TEST_F(list_test, should_conj_elements_in_front_of_the_list)
{
    Root list0{create_list(nullptr, 0)};
    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    Root list1{list_conj(*list0, *elem0)};
    Root list2{list_conj(*list1, *elem1)};
    Root list3{list_conj(*list2, *elem2)};

    ASSERT_EQ(1u, get_int64_value(get_list_size(*list1)));
    ASSERT_TRUE(elem0->is(get_list_first(*list1)));
    ASSERT_TRUE(get_list_next(*list1).is_nil());

    ASSERT_EQ(2u, get_int64_value(get_list_size(*list2)));
    ASSERT_TRUE(elem1->is(get_list_first(*list2)));
    Value next = get_list_next(*list2);
    ASSERT_TRUE(elem0->is(get_list_first(next)));
    ASSERT_TRUE(Root(get_list_next(next))->is_nil());

    ASSERT_EQ(3u, get_int64_value(get_list_size(*list3)));
    ASSERT_TRUE(elem2->is(get_list_first(*list3)));
    next = get_list_next(*list3);
    ASSERT_TRUE(elem1->is(get_list_first(next)));
    next = get_list_next(next);
    ASSERT_TRUE(elem0->is(get_list_first(next)));
    ASSERT_TRUE(get_list_next(next).is_nil());
}

TEST_F(list_test, seq_should_return_the_list_or_nil_when_empty)
{
    Root val;
    val = list();
    EXPECT_TRUE(list_seq(*val).is_nil());
    val = i64(5);
    val = list(nil, *val);
    EXPECT_TRUE(val->is(list_seq(*val)));
}

}
}
