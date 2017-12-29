#include <cleo/macro.hpp>
#include <cleo/eval.hpp>
#include <cleo/error.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct macro_test : Test {};

TEST_F(macro_test, macroexpand1_should_return_the_given_form_if_its_not_a_list_with_a_macro)
{
    Root val, exp;
    val = i64(1234);
    exp = macroexpand1(*val);
    EXPECT_EQ_REFS(*val, *exp);
    val = list();
    exp = macroexpand1(*val);
    EXPECT_EQ_REFS(*val, *exp);
    val = list(SEQ);
    exp = macroexpand1(*val);
    EXPECT_EQ_REFS(*val, *exp);
}

TEST_F(macro_test, macroexpand1_should_eval_the_body)
{
    Root v{svec(5, 6, 7)};
    Root seq{list(SEQ, *v)};
    Root first{list(FIRST, *seq)};
    Root body{list(QUOTE, *first)};
    Root params{svec()};
    Root m{create_macro(nil, *params, *body)};
    Root call{list(*m)};
    Root val{macroexpand1(*call)};
    EXPECT_EQ_VALS(*first, *val);
}

TEST_F(macro_test, macroexpand1_should_pass_the_arguments_unevaluated)
{
    auto a = create_symbol("a");
    auto b = create_symbol("b");
    auto c = create_symbol("c");
    Root body{svec(b, c, a)};
    Root params{svec(a, b, c)};
    Root m{create_macro(nil, *params, *body)};
    Root v{svec(5, 6, 7)};
    Root call{list(*m, SEQ, FIRST, *v)};
    Root val{macroexpand1(*call)};
    Root ex{svec(FIRST, *v, SEQ)};
    EXPECT_EQ_VALS(type::SMALL_VECTOR, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, macroexpand1_should_fail_on_wrong_number_of_args)
{
    auto a = create_symbol("a");
    auto b = create_symbol("b");
    auto c = create_symbol("c");
    Root params{svec(a, b, c)};
    Root m{create_macro(nil, *params, nil)};
    Root call{list(*m, SEQ, FIRST)};
    try
    {
        macroexpand1(*call);
        FAIL() << "expected an exception";
    }
    catch (Exception const& )
    {
        cleo::Root e{cleo::catch_exception()};
        EXPECT_TRUE(type::CallError == get_value_type(*e));
    }
}

}
}
