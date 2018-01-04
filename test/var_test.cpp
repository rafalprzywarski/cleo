#include <cleo/var.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct var_test : Test
{
    var_test() : Test("cleo.var.test") { }
};

TEST_F(var_test, should_define_vars)
{
    auto sym1 = create_symbol("cleo.var.test", "abc");
    auto sym2 = create_symbol("cleo.var.test", "xyz");
    Root val1{Force(create_int64(10))};
    Root val2{Force(create_int64(20))};
    define_var(sym1, *val1);
    ASSERT_TRUE(*val1 == lookup_var(sym1));
    define_var(sym2, *val2);
    ASSERT_TRUE(*val1 == lookup_var(sym1));
    ASSERT_TRUE(*val2 == lookup_var(sym2));
}

TEST_F(var_test, should_redefine_vars)
{
    auto sym = create_symbol("cleo.var.test", "bcd");
    Root val1{Force(create_int64(10))};
    Root val2{Force(create_int64(20))};
    define_var(sym, *val1);
    ASSERT_TRUE(*val1 == lookup_var(sym));
    define_var(sym, *val2);
    ASSERT_TRUE(*val2 == lookup_var(sym));
}

TEST_F(var_test, lookup_should_fail_when_a_var_is_not_found)
{
    auto sym = create_symbol("cleo.var.test", "missing");
    try
    {
        lookup_var(sym);
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ(type::SymbolNotFound, get_value_type(*e));
    }
}

TEST_F(var_test, binding_should_override)
{
    auto a = create_symbol("cleo.var.test/a");
    auto b = create_symbol("cleo.var.test/b");
    auto ka = create_keyword("a");
    auto kb = create_keyword("b");
    auto ka2 = create_keyword("a2");
    auto kb2 = create_keyword("b2");
    auto ka3 = create_keyword("a3");

    define_var(a, ka);
    define_var(b, kb);
    {
        Root bindings1{smap(a, ka2)};
        PushBindingsGuard bind1{*bindings1};
        EXPECT_EQ_VALS(ka2, lookup_var(a));
        EXPECT_EQ_VALS(kb, lookup_var(b));
        {
            Root bindings2{smap(a, ka3, b, kb2)};
            PushBindingsGuard bind1{*bindings2};
            EXPECT_EQ_VALS(ka3, lookup_var(a));
            EXPECT_EQ_VALS(kb2, lookup_var(b));
        }
        EXPECT_EQ_VALS(ka2, lookup_var(a));
        EXPECT_EQ_VALS(kb, lookup_var(b));
    }
    EXPECT_EQ_VALS(ka, lookup_var(a));
    EXPECT_EQ_VALS(kb, lookup_var(b));
}

}
}
