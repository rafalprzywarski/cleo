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

    Force eval_str(std::string source)
    {
        Root s{create_string(source)};
        s = read(*s);
        return eval(*s);
    }
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
    eval_str("(def {:macro :true} mex1 (fn* [&form &env] (quote (cleo.core/first (cleo.core/seq [5 6 7])))))");
    auto name = create_symbol("cleo.macro.test", "mex1");
    Root call{list(name)};
    Root val{macroexpand1(*call)};
    Root ex{list(FIRST, listv(SEQ, arrayv(5, 6, 7)))};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, macroexpand1_should_pass_the_arguments_unevaluated)
{
    eval_str("(def {:macro :true} mex1 (fn* [&form &env a b c] [b c a]))");
    auto name = create_symbol("cleo.macro.test", "mex1");
    Root v{array(5, 6, 7)};
    Root call{list(name, SEQ, FIRST, *v)};
    Root val{macroexpand1(*call)};
    Root ex{array(FIRST, *v, SEQ)};
    EXPECT_EQ_VALS(*type::Array, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, macroexpand1_should_expand_seqs)
{
    eval_str("(def {:macro :true} mex1 (fn* [&form &env a b c] [b c a]))");
    auto name = create_symbol("cleo.macro.test", "mex1");
    Root v{array(5, 6, 7)};
    Root call{array(name, SEQ, FIRST, *v)};
    call = seq(*call);
    Root val{macroexpand1(*call)};
    Root ex{array(FIRST, *v, SEQ)};
    EXPECT_EQ_VALS(*type::Array, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, macroexpand1_should_evaluate_the_first_argument_if_its_a_symbol)
{
    eval_str("(def {:macro :true} ms2e (fn* [&form &env a b c] [b c a]))");
    auto name = create_symbol("cleo.macro.test", "ms2e");
    Root v{array(5, 6, 7)};
    Root call{list(name, SEQ, FIRST, *v)};
    Root val{macroexpand1(*call)};
    Root ex{array(FIRST, *v, SEQ)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, macroexpand1_should_fail_on_wrong_number_of_args)
{
    eval_str("(def {:macro :true} mex3 (fn* [&form &env a b c] nil))");
    auto name = create_symbol("cleo.macro.test", "mex3");
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
    auto x = create_keyword("x");
    eval_str("(def {:macro :true} mex3 (fn* [&form &env] :x))");
    auto name = create_symbol("cleo.macro.test", "mex3");
    Root call{list(name)};
    Root val{macroexpand(*call)};
    EXPECT_EQ_VALS(x, *val);

    eval_str("(def {:macro :true} mex4 (fn* [&form &env] (quote (cleo.macro.test/mex3))))");
    name = create_symbol("cleo.macro.test", "mex4");
    call = list(name);
    val = macroexpand(*call);
    EXPECT_EQ_VALS(x, *val);

    eval_str("(def {:macro :true} mex5 (fn* [&form &env] (quote (cleo.macro.test/mex4))))");
    name = create_symbol("cleo.macro.test", "mex5");
    call = list(name);
    val = macroexpand(*call);
    EXPECT_EQ_VALS(x, *val);
}

TEST_F(macro_test, eval_should_expand_the_macro_and_eval_the_result)
{
    auto s = create_symbol("s");
    Root env{amap(s, *rt::seq)};
    eval_str("(def {:macro :true} mex6 (fn* [&form &env] (quote (cleo.core/first (s [5 6 7])))))");
    auto name = create_symbol("cleo.macro.test", "mex6");
    Root call{list(name)};
    Root val{eval(*call, *env)};
    Root ex{create_int64(5)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(macro_test, eval_should_fail_when_evaluating_a_macro_symbol)
{
    eval_str("(def {:macro :true} mex7 (fn* [&form &env] nil))");
    auto name = create_symbol("cleo.macro.test", "mex7");

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
