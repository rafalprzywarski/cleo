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
    auto var1 = define_var(sym1, *val1);
    ASSERT_FALSE(bool(is_var_macro(var1)));
    ASSERT_EQ_REFS(var1, get_var(sym1));
    ASSERT_EQ_REFS(*val1, get_var_root_value(var1));
    ASSERT_EQ_REFS(*val1, get_var_value(var1));
    auto var2 = define_var(sym2, *val2);
    ASSERT_EQ_REFS(var1, get_var(sym1));
    ASSERT_EQ_REFS(*val1, get_var_root_value(var1));
    ASSERT_EQ_REFS(*val1, get_var_value(var1));
    ASSERT_FALSE(bool(is_var_macro(var2)));
    ASSERT_EQ_REFS(var2, get_var(sym2));
    ASSERT_EQ_REFS(*val2, get_var_root_value(var2));
    ASSERT_EQ_REFS(*val2, get_var_value(var2));
}

TEST_F(var_test, should_define_macro_vars)
{
    auto sym1 = create_symbol("cleo.var.test", "mabc");
    auto sym2 = create_symbol("cleo.var.test", "mxyz");
    Root val1{Force(create_int64(10))};
    Root val2{Force(create_int64(20))};
    Root meta{amap(MACRO_KEY, TRUE)};
    auto var1 = define_var(sym1, *val1, *meta);
    ASSERT_TRUE(bool(is_var_macro(var1)));
    ASSERT_EQ_REFS(var1, get_var(sym1));
    ASSERT_EQ_REFS(*val1, get_var_root_value(var1));
    ASSERT_EQ_REFS(*val1, get_var_value(var1));
    auto var2 = define_var(sym2, *val2, *meta);
    ASSERT_EQ_REFS(var1, get_var(sym1));
    ASSERT_EQ_REFS(*val1, get_var_root_value(var1));
    ASSERT_EQ_REFS(*val1, get_var_value(var1));
    ASSERT_TRUE(bool(is_var_macro(var2)));
    ASSERT_EQ_REFS(var2, get_var(sym2));
    ASSERT_EQ_REFS(*val2, get_var_root_value(var2));
    ASSERT_EQ_REFS(*val2, get_var_value(var2));
}

TEST_F(var_test, should_redefine_vars)
{
    auto sym = create_symbol("cleo.var.test", "bcd");
    Root val1{Force(create_int64(10))};
    Root val2{Force(create_int64(20))};
    auto var = define_var(sym, *val1);
    ASSERT_EQ_REFS(*val1, get_var_value(var));
    ASSERT_EQ_REFS(*val1, get_var_root_value(var));
    ASSERT_EQ_REFS(var, define_var(sym, *val2));
    ASSERT_EQ_REFS(*val2, get_var_value(var));
    ASSERT_EQ_REFS(*val2, get_var_root_value(var));
}

TEST_F(var_test, should_redefine_macro_vars)
{
    auto sym = create_symbol("cleo.var.test", "mbcd");
    Root val1{Force(create_int64(10))};
    Root val2{Force(create_int64(20))};
    Root meta{amap(MACRO_KEY, TRUE)};
    auto var = define_var(sym, *val1, *meta);
    ASSERT_EQ_REFS(*val1, get_var_value(var));
    ASSERT_EQ_REFS(*val1, get_var_root_value(var));
    ASSERT_EQ_REFS(var, define_var(sym, *val2, nil));
    ASSERT_FALSE(bool(is_var_macro(var)));
    ASSERT_EQ_REFS(*val2, get_var_value(var));
    ASSERT_EQ_REFS(*val2, get_var_root_value(var));
    ASSERT_EQ_REFS(var, define_var(sym, *val2, *meta));
    ASSERT_TRUE(bool(is_var_macro(var)));
}

TEST_F(var_test, lookup_should_fail_when_a_var_is_not_found)
{
    auto sym = create_symbol("cleo.var.test", "missing");
    try
    {
        get_var(sym);
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::SymbolNotFound, get_value_type(*e));
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
        Root bindings1{amap(a, ka2)};
        PushBindingsGuard bind1{*bindings1};
        EXPECT_EQ_VALS(ka, get_var_root_value(get_var(a)));
        EXPECT_EQ_VALS(ka2, get_var_value(get_var(a)));
        EXPECT_EQ_VALS(kb, get_var_root_value(get_var(b)));
        EXPECT_EQ_VALS(kb, get_var_value(get_var(b)));
        {
            Root bindings2{amap(a, ka3, b, kb2)};
            PushBindingsGuard bind1{*bindings2};
            EXPECT_EQ_VALS(ka, get_var_root_value(get_var(a)));
            EXPECT_EQ_VALS(ka3, get_var_value(get_var(a)));
            EXPECT_EQ_VALS(kb, get_var_root_value(get_var(b)));
            EXPECT_EQ_VALS(kb2, get_var_value(get_var(b)));
            {
                Root bindings3{amap()};
                PushBindingsGuard bind1{*bindings3};
                EXPECT_EQ_VALS(ka, get_var_root_value(get_var(a)));
                EXPECT_EQ_VALS(ka3, get_var_value(get_var(a)));
                EXPECT_EQ_VALS(kb, get_var_root_value(get_var(b)));
                EXPECT_EQ_VALS(kb2, get_var_value(get_var(b)));
            }
        }
        EXPECT_EQ_VALS(ka, get_var_root_value(get_var(a)));
        EXPECT_EQ_VALS(ka2, get_var_value(get_var(a)));
        EXPECT_EQ_VALS(kb, get_var_root_value(get_var(b)));
        EXPECT_EQ_VALS(kb, get_var_value(get_var(b)));
    }
    EXPECT_EQ_VALS(ka, get_var_root_value(get_var(a)));
    EXPECT_EQ_VALS(ka, get_var_value(get_var(a)));
    EXPECT_EQ_VALS(kb, get_var_root_value(get_var(b)));
    EXPECT_EQ_VALS(kb, get_var_value(get_var(b)));
}

TEST_F(var_test, set_var_should_fail_if_there_are_no_bindings_for_it)
{
    auto s = create_symbol("cleo.var.test/nob");
    auto var = define_var(s, create_keyword("xxx"));
    try
    {
        set_var(s, create_keyword("yyy"));
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_VALS(*type::IllegalState, get_value_type(*e));
    }

    PushBindingsGuard bind1{*EMPTY_MAP};

    try
    {
        set_var(s, create_keyword("yyy"));
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_VALS(*type::IllegalState, get_value_type(*e));
    }

    ASSERT_EQ_VALS(create_keyword("xxx"), get_var_value(var));
}

TEST_F(var_test, set_var_change_the_value_of_a_var_in_the_latest_bindings)
{
    auto a = create_symbol("cleo.var.test/ba");
    auto b = create_symbol("cleo.var.test/bb");
    auto ka = create_keyword("a");
    auto kb = create_keyword("b");
    auto ka2 = create_keyword("a2");
    auto kb2 = create_keyword("b2");
    auto ka3 = create_keyword("a3");
    auto kax = create_keyword("ax");
    auto kbx = create_keyword("bx");

    auto va = define_var(a, ka);
    auto vb = define_var(b, kb);
    {
        Root bindings1{amap(a, ka2)};
        PushBindingsGuard bind1{*bindings1};

        set_var(a, kax);
        EXPECT_EQ_VALS(ka, get_var_root_value(va));
        EXPECT_EQ_VALS(kax, get_var_value(va));

        {
            Root bindings2{amap(a, ka3, b, kb2)};
            PushBindingsGuard bind1{*bindings2};
            EXPECT_EQ_VALS(ka3, get_var_value(va));
            EXPECT_EQ_VALS(kb, get_var_root_value(vb));
            EXPECT_EQ_VALS(kb2, get_var_value(vb));

            set_var(a, kax);
            EXPECT_EQ_VALS(kax, get_var_value(va));
            set_var(b, kbx);
            EXPECT_EQ_VALS(kbx, get_var_value(vb));

            set_var(a, ka3);
            EXPECT_EQ_VALS(ka3, get_var_value(va));
            set_var(b, kb2);
            EXPECT_EQ_VALS(kb2, get_var_value(vb));
        }
        EXPECT_EQ_VALS(kax, get_var_value(va));
        EXPECT_EQ_VALS(kb, get_var_value(vb));
    }
    EXPECT_EQ_VALS(ka, get_var_value(va));
    EXPECT_EQ_VALS(kb, get_var_value(vb));
}

}
}
