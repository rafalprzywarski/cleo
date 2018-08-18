#include <cleo/fn.hpp>
#include <cleo/eval.hpp>
#include <cleo/error.hpp>
#include <cleo/var.hpp>
#include <cleo/reader.hpp>
#include <cleo/compile.hpp>
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
    val = list(create_symbol("abc"));
    exp = macroexpand1(*val);
    EXPECT_EQ_REFS(*val, *exp);
}

TEST_F(macro_test, macroexpand1_should_eval_the_body)
{
    Root v{array(5, 6, 7)};
    Root seq{list(SEQ, *v)};
    Root first{list(FIRST, *seq)};
    Root body{list(QUOTE, *first)};
    Root params{array(FORM, ENV)};
    Root m{create_fn(nil, nil, *params, *body)};
    auto name = create_symbol("cleo.macro.test", "mex1");
    Root meta{amap(MACRO_KEY, TRUE)};
    define(name, *m, *meta);
    Root call{list(name)};
    Root val{macroexpand1(*call)};
    EXPECT_EQ_VALS(*first, *val);
}

TEST_F(macro_test, macroexpand1_should_pass_the_arguments_unevaluated)
{
    auto a = create_symbol("a");
    auto b = create_symbol("b");
    auto c = create_symbol("c");
    Root body{array(b, c, a)};
    Root params{array(FORM, ENV, a, b, c)};
    Root m{create_fn(nil, nil, *params, *body)};
    auto name = create_symbol("cleo.macro.test", "mex1");
    Root meta{amap(MACRO_KEY, TRUE)};
    define(name, *m, *meta);
    Root v{array(5, 6, 7)};
    Root call{list(name, SEQ, FIRST, *v)};
    Root val{macroexpand1(*call)};
    Root ex{array(FIRST, *v, SEQ)};
    EXPECT_EQ_VALS(*type::Array, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, macroexpand1_should_evaluate_bytecode_fns)
{
    Root m{create_string("(fn* [&form &env a b c] [b c a])")};
    m = read(*m);
    m = compile_fn(*m, nil);
    auto name = create_symbol("cleo.macro.test", "bcmex1");
    Root meta{amap(MACRO_KEY, TRUE)};
    define(name, *m, *meta);
    Root v{array(5, 6, 7)};
    Root call{list(name, SEQ, FIRST, *v)};
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
    auto name = create_symbol("cleo.macro.test", "ms2e");
    Root body{array(b, c, a)};
    Root params{array(FORM, ENV, a, b, c)};
    Root m{create_fn(nil, nil, *params, *body)};
    Root meta{amap(MACRO_KEY, TRUE)};
    define(name, *m, *meta);
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
    Root params{array(FORM, ENV, a, b, c)};
    Root m{create_fn(nil, nil, *params, nil)};
    Root meta{amap(MACRO_KEY, TRUE)};
    auto name = create_symbol("cleo.macro.test", "mex3");
    define(name, *m, *meta);
    Root call{list(name, SEQ, FIRST)};
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
    Root params{array(FORM, ENV)};
    Root meta{amap(MACRO_KEY, TRUE)};
    auto x = create_keyword("x");
    Root m{create_fn(nil, nil, *params, x)};
    auto name = create_symbol("cleo.macro.test", "mex3");
    define(name, *m, *meta);
    Root call{list(name)};
    Root val{macroexpand(*call)};
    EXPECT_EQ_VALS(x, *val);

    Root q{list(QUOTE, *call)};
    m = create_fn(nil, nil, *params, *q);
    name = create_symbol("cleo.macro.test", "mex4");
    define(name, *m, *meta);
    call = list(name);
    val = macroexpand(*call);
    EXPECT_EQ_VALS(x, *val);

    q = list(QUOTE, *call);
    m = create_fn(nil, nil, *params, *q);
    name = create_symbol("cleo.macro.test", "mex5");
    define(name, *m, *meta);
    call = list(name);
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
    Root params{array(FORM, ENV)};
    Root m{create_fn(nil, nil, *params, *body)};
    Root meta{amap(MACRO_KEY, TRUE)};
    auto name = create_symbol("cleo.macro.test", "mex6");
    define(name, *m, *meta);
    Root call{list(name)};
    Root val{eval(*call, *env)};
    EXPECT_EQ_REFS(get_array_elem(*v, 0), *val);
}

TEST_F(macro_test, eval_should_fail_when_evaluating_a_macro_symbol)
{
    Root params{array(FORM, ENV)};
    Root m{create_fn(nil, nil, *params, nil)};
    Root meta{amap(MACRO_KEY, TRUE)};
    auto name = create_symbol("cleo.macro.test", "mex7");
    define(name, *m, *meta);

    try
    {
        eval(name);
        FAIL() << "expected an exception";
    }
    catch (Exception const& )
    {
        cleo::Root e{cleo::catch_exception()};
        EXPECT_EQ_REFS(*type::CompilationError, get_value_type(*e));
    }

}

TEST_F(macro_test, should_correctly_resolve_symbols_from_macros_namespace)
{
    in_ns(create_symbol("cleo.macro.resolve.test1"));
    define(create_symbol("cleo.macro.resolve.test1", "x"), create_keyword("ok"));
    Root fn{create_string("(fn* [&form &env] [x `'x `'y])")};
    fn = read(*fn);
    fn = eval(*fn);
    Root meta{amap(MACRO_KEY, TRUE)};
    auto name = create_symbol("cleo.macro.resolve.test1", "m");
    define(name, *fn, *meta);
    in_ns(create_symbol("cleo.macro.resolve.test2"));
    Root val{list(name)}, ex;
    val = eval(*val);
    ex = array(create_keyword("ok"), create_symbol("cleo.macro.resolve.test1", "x"), create_symbol("cleo.macro.resolve.test1", "y"));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, should_pass_the_form_as_a_hidden_parameter)
{
    in_ns(create_symbol("cleo.macro.form.test"));
    Root decl{create_string("(def {:macro :true} ff (fn* [&form &env & args] `(quote ~&form)))")};
    decl = read(*decl);
    eval(*decl);
    Root form{create_string("(ff 1 (+ 2 3) 4)")};
    form = read(*form);
    Root val{eval(*form)};
    EXPECT_EQ_VALS(*form, *val);
}

}
}
