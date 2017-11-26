#include <cleo/list.hpp>
#include <gtest/gtest.h>
#include <array>

namespace cleo
{
namespace test
{

TEST(list_test, should_create_an_empty_list)
{
    Value list = create_list(nullptr, 0);
    ASSERT_EQ(0, get_int64_value(get_list_size(list)));
    ASSERT_TRUE(get_nil() == get_list_first(list));
    ASSERT_TRUE(get_nil() == get_list_next(list));
}

TEST(list_test, should_create_a_list_from_elements)
{
    Value elem = create_int64(11);
    Value list = create_list(&elem, 1);
    ASSERT_EQ(1, get_int64_value(get_list_size(list)));
    ASSERT_TRUE(elem == get_list_first(list));
    ASSERT_TRUE(get_nil() == get_list_next(list));

    std::array<Value, 3> elems{{
        create_int64(101),
        create_int64(102),
        create_int64(103)
    }};
    list = create_list(elems.data(), elems.size());
    ASSERT_EQ(elems.size(), get_int64_value(get_list_size(list)));
    ASSERT_TRUE(elems[0] == get_list_first(list));

    Value next1 = get_list_next(list);
    ASSERT_EQ(2u, get_int64_value(get_list_size(next1)));
    ASSERT_TRUE(elems[1] == get_list_first(next1));

    Value next2 = get_list_next(next1);
    ASSERT_EQ(1u, get_int64_value(get_list_size(next2)));
    ASSERT_TRUE(elems[2] == get_list_first(next2));
    ASSERT_TRUE(get_nil() == get_list_next(next2));
}

TEST(list_test, should_conj_elements_in_front_of_the_list)
{
    Value list0 = create_list(nullptr, 0);
    std::array<Value, 3> elems{{
        create_int64(101),
        create_int64(102),
        create_int64(103)
    }};
    Value list1 = list_conj(list0, elems[0]);
    Value list2 = list_conj(list1, elems[1]);
    Value list3 = list_conj(list2, elems[2]);

    ASSERT_EQ(1u, get_int64_value(get_list_size(list1)));
    ASSERT_TRUE(elems[0] == get_list_first(list1));
    ASSERT_TRUE(get_nil() == get_list_next(list1));

    ASSERT_EQ(2u, get_int64_value(get_list_size(list2)));
    ASSERT_TRUE(elems[1] == get_list_first(list2));
    ASSERT_TRUE(elems[0] == get_list_first(get_list_next(list2)));
    ASSERT_TRUE(get_nil() == get_list_next(get_list_next(list2)));

    ASSERT_EQ(3u, get_int64_value(get_list_size(list3)));
    ASSERT_TRUE(elems[2] == get_list_first(list3));
    ASSERT_TRUE(elems[1] == get_list_first(get_list_next(list3)));
    ASSERT_TRUE(elems[0] == get_list_first(get_list_next(get_list_next(list3))));
    ASSERT_TRUE(get_nil() == get_list_next(get_list_next(get_list_next(list3))));
}

}
}
