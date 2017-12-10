#include <cleo/small_map.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct small_map_test : Test {};

TEST_F(small_map_test, should_create_an_empty_map)
{
    Root m{create_small_map()};
    ASSERT_EQ(0u, get_small_map_size(*m));
    ASSERT_TRUE(nil == small_map_get(*m, create_keyword("abc")));
}

TEST_F(small_map_test, assoc_should_create_a_new_map_with_an_added_or_replaced_element)
{
    Root k1a{create_int64(10)};
    Root k1b{create_int64(10)};
    Root k2{create_int64(20)};
    Root k3{create_int64(30)};
    Root v1{create_int64(101)};
    Root v2{nil};
    Root v3{create_int64(103)};
    Root m0{create_small_map()};
    Root m1{small_map_assoc(*m0, *k1a, *v1)};
    Root m2{small_map_assoc(*m1, *k2, *v2)};
    Root m3{small_map_assoc(*m2, *k3, *v3)};
    Root m4{small_map_assoc(*m3, *k1b, *v3)};

    EXPECT_EQ(0u, get_small_map_size(*m0));
    EXPECT_EQ_REFS(nil, small_map_get(*m0, *k1b));

    EXPECT_EQ(1u, get_small_map_size(*m1));
    EXPECT_EQ_REFS(*v1, small_map_get(*m1, *k1b));
    EXPECT_EQ_REFS(nil, small_map_get(*m1, *k2));

    EXPECT_EQ(2u, get_small_map_size(*m2));
    EXPECT_EQ_REFS(*v1, small_map_get(*m2, *k1b));
    EXPECT_EQ_REFS(*v2, small_map_get(*m2, *k2));
    EXPECT_EQ_REFS(nil, small_map_get(*m2, *k3));

    EXPECT_EQ(3u, get_small_map_size(*m3));
    EXPECT_EQ_REFS(*v1, small_map_get(*m3, *k1b));
    EXPECT_EQ_REFS(*v2, small_map_get(*m3, *k2));
    EXPECT_EQ_REFS(*v3, small_map_get(*m3, *k3));

    EXPECT_EQ(3u, get_small_map_size(*m4));
    EXPECT_EQ_REFS(*v3, small_map_get(*m4, *k1a));
    EXPECT_EQ_REFS(*v2, small_map_get(*m4, *k2));
    EXPECT_EQ_REFS(*v3, small_map_get(*m4, *k3));
}

TEST_F(small_map_test, contains_should_tell_if_a_map_contains_a_key)
{
    Root k1a{create_int64(10)};
    Root k1b{create_int64(10)};
    Root k2{create_int64(20)};
    Root v1{create_int64(101)};
    Root v2{nil};
    Root m0{create_small_map()};
    Root m1{small_map_assoc(*m0, *k1a, *v1)};
    Root m2{small_map_assoc(*m1, *k2, nil)};

    EXPECT_EQ_REFS(nil, small_map_contains(*m0, *k1b));
    EXPECT_EQ_REFS(TRUE, small_map_contains(*m1, *k1b));
    EXPECT_EQ_REFS(nil, small_map_contains(*m1, *k2));
    EXPECT_EQ_REFS(TRUE, small_map_contains(*m2, *k1b));
    EXPECT_EQ_REFS(TRUE, small_map_contains(*m2, *k2));
}

TEST_F(small_map_test, should_merge_maps_with_values_from_the_second_map_overriding_values_from_the_first)
{
    Root k1{create_int64(10)};
    Root k2{create_int64(20)};
    Root k3{create_int64(30)};
    Root v1{create_int64(101)};
    Root v2{create_int64(102)};
    Root v3{create_int64(103)};
    Root v4{create_int64(104)};
    Root l{create_small_map()};
    l = small_map_assoc(*l, *k1, *v1);
    l = small_map_assoc(*l, *k2, *v2);

    Root r{create_small_map()};
    r = small_map_assoc(*r, *k1, *v4);
    r = small_map_assoc(*r, *k3, *v3);

    Root empty{create_small_map()};
    Root m{small_map_merge(*l, *empty)};
    m = small_map_merge(*m, *r);

    EXPECT_EQ_REFS(*v4, small_map_get(*m, *k1));
    EXPECT_EQ_REFS(*v2, small_map_get(*m, *k2));
    EXPECT_EQ_REFS(*v3, small_map_get(*m, *k3));

    EXPECT_EQ_REFS(*v1, small_map_get(*l, *k1));
    EXPECT_EQ_REFS(*v2, small_map_get(*l, *k2));
    EXPECT_EQ_REFS(nil, small_map_get(*l, *k3));

    EXPECT_EQ_REFS(*v4, small_map_get(*r, *k1));
    EXPECT_EQ_REFS(nil, small_map_get(*r, *k2));
    EXPECT_EQ_REFS(*v3, small_map_get(*r, *k3));
}

}
}
