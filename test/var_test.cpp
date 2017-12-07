#include <cleo/var.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct var_test : Test {};

TEST_F(var_test, should_define_vars)
{
    auto sym1 = create_symbol("cleo.var.test", "abc");
    auto sym2 = create_symbol("cleo.var.test", "xyz");
    Root val1{Force(create_int64(10))};
    Root val2{Force(create_int64(20))};
    define(sym1, *val1);
    ASSERT_TRUE(*val1 == lookup(sym1));
    define(sym2, *val2);
    ASSERT_TRUE(*val1 == lookup(sym1));
    ASSERT_TRUE(*val2 == lookup(sym2));
}

TEST_F(var_test, should_redefine_vars)
{
    auto sym = create_symbol("cleo.var.test", "bcd");
    Root val1{Force(create_int64(10))};
    Root val2{Force(create_int64(20))};
    define(sym, *val1);
    ASSERT_TRUE(*val1 == lookup(sym));
    define(sym, *val2);
    ASSERT_TRUE(*val2 == lookup(sym));
}

TEST_F(var_test, lookup_should_return_nil_when_a_var_is_not_found)
{
    auto sym = create_symbol("cleo.var.test", "missing");
    ASSERT_TRUE(nil == lookup(sym));
}

}
}
