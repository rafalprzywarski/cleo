#include <cleo/lazy_seq.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct lazy_seq_test : Test
{
    static bool called;
    lazy_seq_test() : Test("cleo.lazy-seq.test") { }
};

bool lazy_seq_test::called = false;

TEST_F(lazy_seq_test, seq_should_eval_the_fn_once_and_return_seq_of_the_result)
{
    Root fn, ls, ex, val;
    fn = create_native_function([](const Value *, std::uint8_t) -> Force { called = true; return list(5, 6, 7); });
    ls = create_lazy_seq(*fn);
    ex = list(5, 6, 7);
    called = false;
    val = lazy_seq_seq(*ls);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_TRUE(called);
    called = false;
    val = lazy_seq_seq(*ls);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_FALSE(called);

    fn = create_native_function([](const Value *, std::uint8_t) -> Force { called = true; return list(); });
    ls = create_lazy_seq(*fn);
    called = false;
    val = lazy_seq_seq(*ls);
    EXPECT_EQ_VALS(nil, *val);
    EXPECT_TRUE(called);
    called = false;
    val = lazy_seq_seq(*ls);
    EXPECT_EQ_VALS(nil, *val);
    EXPECT_FALSE(called);
}

TEST_F(lazy_seq_test, first_should_return_first_of_seq)
{
    Root fn{create_native_function([](const Value *, std::uint8_t) -> Force { return list(5, 6, 7); })};
    Root ls{create_lazy_seq(*fn)};
    Root ex{i64(5)};
    Root val{lazy_seq_first(*ls)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(lazy_seq_test, first_should_return_next_of_seq)
{
    Root fn{create_native_function([](const Value *, std::uint8_t) -> Force { return list(5, 6, 7); })};
    Root ls{create_lazy_seq(*fn)};
    Root ex{list(6, 7)};
    Root val{lazy_seq_next(*ls)};
    EXPECT_EQ_VALS(*ex, *val);
}


}
}
