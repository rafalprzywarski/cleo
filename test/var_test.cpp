#include <cleo/var.hpp>
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

TEST(var_test, should_define_vars)
{
    auto sym1 = create_symbol("cleo.var.test", "abc");
    auto sym2 = create_symbol("cleo.var.test", "xyz");
    auto val1 = create_int64(10);
    auto val2 = create_int64(20);
    define(sym1, val1);
    ASSERT_TRUE(val1 == lookup(sym1));
    define(sym2, val2);
    ASSERT_TRUE(val1 == lookup(sym1));
    ASSERT_TRUE(val2 == lookup(sym2));
}

TEST(var_test, should_redefine_vars)
{
    auto sym = create_symbol("cleo.var.test", "bcd");
    auto val1 = create_int64(10);
    auto val2 = create_int64(20);
    define(sym, val1);
    ASSERT_TRUE(val1 == lookup(sym));
    define(sym, val2);
    ASSERT_TRUE(val2 == lookup(sym));
}

TEST(var_test, lookup_should_return_nil_when_a_var_is_not_found)
{
    auto sym = create_symbol("cleo.var.test", "missing");
    ASSERT_TRUE(nil == lookup(sym));
}

}
}
