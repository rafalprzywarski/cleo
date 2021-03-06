#include <cleo/array_set.hpp>
#include <cleo/global.hpp>
#include <cleo/eval.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct array_set_test : Test
{
    array_set_test() : Test("cleo.array-set.test") { }
};

TEST_F(array_set_test, should_create_an_empty_set)
{
    Root set{create_array_set()};
    ASSERT_EQ(0u, get_array_set_size(*set));
    ASSERT_EQ_VALS(nil, get_array_set_elem(*set, 0));
}

TEST_F(array_set_test, should_conj_elements)
{
    Root set0{create_array_set()};
    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    Root set1{array_set_conj(*set0, *elem0)};
    Root set2{array_set_conj(*set1, *elem1)};
    Root set3{array_set_conj(*set2, *elem2)};

    ASSERT_EQ(1u, get_array_set_size(*set1));
    EXPECT_EQ_REFS(*elem0, get_array_set_elem(*set1, 0));

    ASSERT_EQ(2u, get_array_set_size(*set2));
    EXPECT_EQ_REFS(*elem0, get_array_set_elem(*set2, 0));
    EXPECT_EQ_REFS(*elem1, get_array_set_elem(*set2, 1));

    ASSERT_EQ(3u, get_array_set_size(*set3));
    EXPECT_EQ_REFS(*elem0, get_array_set_elem(*set3, 0));
    EXPECT_EQ_REFS(*elem1, get_array_set_elem(*set3, 1));
    EXPECT_EQ_REFS(*elem2, get_array_set_elem(*set3, 2));
}

TEST_F(array_set_test, contains_should_tell_if_a_set_contains_a_key)
{
    Root s, elem0, elem1;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    s = create_array_set();
    ASSERT_EQ_VALS(nil, array_set_contains(*s, *elem0));
    ASSERT_EQ_VALS(nil, array_set_get(*s, *elem0));

    s = array_set_conj(*s, *elem0);
    ASSERT_EQ_VALS(TRUE, array_set_contains(*s, *elem0));
    ASSERT_EQ_VALS(*elem0, array_set_get(*s, *elem0));
    ASSERT_EQ_VALS(nil, array_set_contains(*s, *elem1));
    ASSERT_EQ_VALS(nil, array_set_get(*s, *elem1));

    s = array_set_conj(*s, *elem1);
    ASSERT_EQ_VALS(TRUE, array_set_contains(*s, *elem0));
    ASSERT_EQ_VALS(*elem0, array_set_get(*s, *elem0));
    ASSERT_EQ_VALS(TRUE, array_set_contains(*s, *elem1));
    ASSERT_EQ_VALS(*elem1, array_set_get(*s, *elem1));
}

TEST_F(array_set_test, should_not_conj_duplicates)
{
    Root s, elem0a, elem0b, elem1a, elem1b;
    elem0a = create_int64(101);
    elem0b = create_int64(101);
    elem1a = create_int64(102);
    elem1b = create_int64(102);
    s = create_array_set();
    s = array_set_conj(*s, *elem0a);
    s = array_set_conj(*s, *elem0b);

    ASSERT_EQ(1u, get_array_set_size(*s));
    EXPECT_EQ_REFS(*elem0a, get_array_set_elem(*s, 0));

    s = array_set_conj(*s, *elem1a);
    s = array_set_conj(*s, *elem1b);

    ASSERT_EQ(2u, get_array_set_size(*s));
    EXPECT_EQ_REFS(*elem0a, get_array_set_elem(*s, 0));
    EXPECT_EQ_REFS(*elem1a, get_array_set_elem(*s, 1));
}

TEST_F(array_set_test, seq_should_return_nil_for_an_empty)
{
    Root s{create_array_set()};
    EXPECT_EQ_REFS(nil, *Root(array_set_seq(*s)));
}

TEST_F(array_set_test, seq_should_return_a_sequence_of_the_set_elements)
{
    Root elem{create_int64(11)};
    Root s{create_array_set()};
    s = array_set_conj(*s, *elem);
    Root seq{array_set_seq(*s)};
    ASSERT_EQ_REFS(*elem, get_array_set_seq_first(*seq));
    ASSERT_EQ_REFS(nil, *Root(get_array_set_seq_next(*seq)));

    Root elem0, elem1, elem2;
    elem0 = create_int64(101);
    elem1 = create_int64(102);
    elem2 = create_int64(103);
    s = create_array_set();
    s = array_set_conj(*s, *elem0);
    s = array_set_conj(*s, *elem1);
    s = array_set_conj(*s, *elem2);
    seq = array_set_seq(*s);
    ASSERT_EQ_REFS(*elem0, get_array_set_seq_first(*seq));

    seq = get_array_set_seq_next(*seq);
    ASSERT_EQ_REFS(*elem1, get_array_set_seq_first(*seq));

    seq = get_array_set_seq_next(*seq);
    ASSERT_EQ_REFS(*elem2, get_array_set_seq_first(*seq));
    ASSERT_EQ_REFS(nil, *Root(get_array_set_seq_next(*seq)));
}

TEST_F(array_set_test, should_delegate_to_get_when_called)
{
    Root s{create_array_set()};
    Root e0{create_int64(101)};
    Root e1{create_int64(102)};
    s = array_set_conj(*s, *e0);
    Root call{list(*s, *e0)};
    Root val{eval(*call)};
    EXPECT_EQ_VALS(*e0, *val);

    call = list(*s, *e1);
    val = eval(*call);
    EXPECT_EQ_VALS(nil, *val);
}

}
}
