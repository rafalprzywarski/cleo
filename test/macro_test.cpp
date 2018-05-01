#include <cleo/fn.hpp>
#include <cleo/eval.hpp>
#include <cleo/error.hpp>
#include <cleo/var.hpp>
#include <cleo/reader.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct macro_test : Test
{
    macro_test() : Test("cleo.macro.test") {}
};

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
    Root v{array(5, 6, 7)};
    Root seq{list(SEQ, *v)};
    Root first{list(FIRST, *seq)};
    Root body{list(QUOTE, *first)};
    Root params{array()};
    Root m{create_macro(nil, nil, *params, *body)};
    Root call{list(*m)};
    Root val{macroexpand1(*call)};
    EXPECT_EQ_VALS(*first, *val);
}

TEST_F(macro_test, macroexpand1_should_pass_the_arguments_unevaluated)
{
    auto a = create_symbol("a");
    auto b = create_symbol("b");
    auto c = create_symbol("c");
    Root body{array(b, c, a)};
    Root params{array(a, b, c)};
    Root m{create_macro(nil, nil, *params, *body)};
    Root v{array(5, 6, 7)};
    Root call{list(*m, SEQ, FIRST, *v)};
    Root val{macroexpand1(*call)};
    Root ex{array(FIRST, *v, SEQ)};
    EXPECT_EQ_VALS(*type::Array, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, macroexpand1_should_evaluate_the_first_argument_if_its_a_symbol)
{
    auto a = create_symbol("a");
    auto b = create_symbol("b");
    auto c = create_symbol("c");
    auto name = create_symbol("cleo.macro.test", "m2e");
    Root body{array(b, c, a)};
    Root params{array(a, b, c)};
    Root m{create_macro(nil, nil, *params, *body)};
    define(name, *m);
    Root v{array(5, 6, 7)};
    Root call{list(name, SEQ, FIRST, *v)};
    Root val{macroexpand1(*call)};
    Root ex{array(FIRST, *v, SEQ)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, macroexpand1_should_fail_on_wrong_number_of_args)
{
    auto a = create_symbol("a");
    auto b = create_symbol("b");
    auto c = create_symbol("c");
    Root params{array(a, b, c)};
    Root m{create_macro(nil, nil, *params, nil)};
    Root call{list(*m, SEQ, FIRST)};
    try
    {
        macroexpand1(*call);
        FAIL() << "expected an exception";
    }
    catch (Exception const& )
    {
        cleo::Root e{cleo::catch_exception()};
        EXPECT_EQ_REFS(*type::CallError, get_value_type(*e));
    }
}

TEST_F(macro_test, macroexpand_should_return_the_given_form_if_its_not_a_list_with_a_macro)
{
    Root val, exp;
    val = i64(1234);
    exp = macroexpand(*val);
    EXPECT_EQ_REFS(*val, *exp);
    val = list();
    exp = macroexpand(*val);
    EXPECT_EQ_REFS(*val, *exp);
    val = list(SEQ);
    exp = macroexpand(*val);
    EXPECT_EQ_REFS(*val, *exp);
}

TEST_F(macro_test, macroexpand_expand_until_the_first_element_is_not_a_macro)
{
    Root params{array()};
    auto x = create_keyword("x");
    Root m{create_macro(nil, nil, *params, x)};
    Root call{list(*m)};
    Root val{macroexpand(*call)};
    EXPECT_EQ_VALS(x, *val);

    Root q{list(QUOTE, *call)};
    m = create_macro(nil, nil, *params, *q);
    call = list(*m);
    val = macroexpand(*call);
    EXPECT_EQ_VALS(x, *val);

    q = list(QUOTE, *call);
    m = create_macro(nil, nil, *params, *q);
    call = list(*m);
    val = macroexpand(*call);
    EXPECT_EQ_VALS(x, *val);
}

TEST_F(macro_test, eval_should_expand_the_macro_and_eval_the_result)
{
    auto s = create_symbol("s");
    Root env{amap(s, *rt::seq)};
    Root v{array(5, 6, 7)};
    Root seq{list(s, *v)};
    Root body{list(FIRST, *seq)};
    body = list(QUOTE, *body);
    Root params{array()};
    Root m{create_macro(nil, nil, *params, *body)};
    Root call{list(*m)};
    Root val{eval(*call, *env)};
    EXPECT_EQ_REFS(get_array_elem(*v, 0), *val);

    auto a = create_symbol("a");
    auto x = create_keyword("x");
    params = array(a);
    body = list(MACRO, *params, a);
    call = list(*body, x);
    val = eval(*call, *env);
    EXPECT_EQ_REFS(x, *val);
}

TEST_F(macro_test, should_correctly_resolve_symbols_from_macros_namespace)
{
    in_ns(create_symbol("cleo.macro.resolve.test1"));
    define(create_symbol("cleo.macro.resolve.test1", "x"), create_keyword("ok"));
    Root fn{create_string("(macro* [] [x `'x `'y])")};
    fn = read(*fn);
    fn = eval(*fn);
    in_ns(create_symbol("cleo.macro.resolve.test2"));
    Root val{list(*fn)}, ex;
    val = eval(*val);
    ex = array(create_keyword("ok"), create_symbol("cleo.macro.resolve.test1", "x"), create_symbol("cleo.macro.resolve.test1", "y"));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, should_pass_the_form_as_a_hidden_parameter)
{
    in_ns(create_symbol("cleo.macro.form.test"));
    Root decl{create_string("(def ff (macro* [& args] `(quote ~&form)))")};
    decl = read(*decl);
    eval(*decl);
    Root form{create_string("(ff 1 (+ 2 3) 4)")};
    form = read(*form);
    Root val{eval(*form)};
    EXPECT_EQ_VALS(*form, *val);
}

TEST_F(macro_test, should_pass_the_env_as_a_hidden_parameter)
{
    in_ns(create_symbol("cleo.macro.env.test"));
    Root decl{create_string("(def ff (macro* [& args] `(quote ~&env)))")};
    decl = read(*decl);
    eval(*decl);
    Root form{create_string("(ff 1)")};
    form = read(*form);
    Root env{amap(create_symbol("x"), 7, create_symbol("y"), 8)};
    Root val{eval(*form, *env)};
    EXPECT_EQ_VALS(*env, *val);
}

}
}
