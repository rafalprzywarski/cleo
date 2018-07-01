#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include <cleo/list.hpp>
#include <cleo/equality.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include <cleo/fn.hpp>
#include <cleo/reader.hpp>
#include <cleo/array_map.hpp>
#include <cleo/memory.hpp>
#include <cleo/reader.hpp>
#include <cleo/util.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct eval_test : Test
{
    eval_test() : Test("cleo.eval.test") { }

    Force read_str(const std::string& s)
    {
        Root ss{create_string(s)};
        return read(*ss);
    }

    void expect_symbol_not_found(std::string source)
    {
        Root fn{create_string(source)};
        fn = read(*fn);
        try
        {
            eval(*fn);
            FAIL() << "expected SymbolNotFound for: " << source;
        }
        catch (const Exception& )
        {
            Root e{catch_exception()};
            ASSERT_EQ_REFS(*type::SymbolNotFound, get_value_type(*e));
        }
    }

    void expect_resolve_illegal_state(std::string source)
    {
        Root body{create_string(source)};
        body = read(*body);
        try
        {
            resolve_value(*body, *EMPTY_MAP);
            FAIL() << "expected IllegalState for: " << source;
        }
        catch (const Exception& )
        {
            Root e{catch_exception()};
            ASSERT_EQ_REFS(*type::IllegalState, get_value_type(*e)) << "exception: " << to_string(*e);
        }
    }

    void expect_symbol_resolved(std::string expected_source, std::string body_source, std::string env_source)
    {
        Root ex{create_string(expected_source)};
        ex = read(*ex);
        expect_symbol_resolved(*ex, body_source, env_source);
    }

    void expect_symbol_resolved(Force expected, std::string body_source, std::string env_source)
    {
        Root ex{expected};
        expect_symbol_resolved(*ex, body_source, env_source);
    }

    void expect_symbol_resolved(Value expected, std::string body_source, std::string env_source)
    {
        Root env{create_string(env_source)};
        env = read(*env);
        Root body{create_string(body_source)};
        body = read(*body);

        try
        {
            body = resolve_value(*body, *env);
        }
        catch (const Exception& )
        {
            Root e{catch_exception()};
            FAIL() << "exception: " << to_string(*e) << " body: " << body_source << " env: " << env_source;
        }
        ASSERT_EQ_VALS(expected, *body) << "body: " << body_source << " env: " << env_source;
    }
};

TEST_F(eval_test, should_eval_simple_values_to_themselves)
{
    Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); })};
    auto kw = create_keyword("org.xyz", "eqkw");
    Root i{create_int64(7)};
    Root flt{create_float64(3.5)};
    Root s{create_string("abcd")};

    ASSERT_TRUE(nil.is(*Root(eval(nil))));
    ASSERT_TRUE(fn->is(*Root(eval(*fn))));
    ASSERT_TRUE(kw.is(*Root(eval(kw))));
    ASSERT_TRUE(i->is(*Root(eval(*i))));
    ASSERT_TRUE(flt->is(*Root(eval(*flt))));
    ASSERT_TRUE(s->is(*Root(eval(*s))));
}

TEST_F(eval_test, should_eval_objects_to_themselves)
{
    auto sym = create_symbol("cleo.eval.test", "obj");
    Root o{create_object0(sym)};
    ASSERT_TRUE(o->is(*Root(eval(*o))));
}

TEST_F(eval_test, should_eval_symbols_to_var_values)
{
    auto sym = create_symbol("cleo.eval.test", "seven");
    Root val;
    val = create_int64(7);
    define(sym, *val);
    ASSERT_TRUE(val->is(*Root(eval(sym))));
}

TEST_F(eval_test, should_eval_symbols_in_the_current_ns)
{
    auto sym = create_symbol("cleo.eval.test", "eight");
    Root val;
    val = create_int64(8);
    define(sym, *val);
    in_ns(create_symbol("cleo.eval.test"));
    ASSERT_TRUE(val->is(*Root(eval(create_symbol("eight")))));
}

TEST_F(eval_test, should_fail_when_a_symbol_cannot_be_resolved)
{
    auto sym = create_symbol("cleo.eval.test", "missing");
    try
    {
        eval(sym);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::SymbolNotFound, get_value_type(*e));
    }
}

TEST_F(eval_test, should_eval_lists_as_function_calls)
{
    Root fn{create_native_function([](const Value *args, std::uint8_t num_args) { return num_args ? create_list(args, num_args) : force(nil); })};

    Root l{list(*fn)};
    ASSERT_TRUE(Root(eval(*l))->is_nil());

    Root e0, e1, val;
    e0 = create_int64(101);
    e1 = create_int64(102);
    val = list(*fn, *e0, *e1);
    e0 = nil;
    e1 = nil;
    val = eval(*val);

    ASSERT_TRUE(type::List->is(get_value_type(*val)));
    ASSERT_EQ(2, get_list_size(*val));
    ASSERT_EQ(101, get_int64_value(get_list_first(*val)));
    Root next;
    next = get_list_next(*val);
    ASSERT_EQ(102, get_int64_value(get_list_first(*next)));
}

TEST_F(eval_test, should_fail_when_trying_to_call_a_non_function)
{
    Root val;
    val = create_int64(10);
    val = list(*val);
    try
    {
        eval(*val);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::CallError, get_value_type(*e));
    }
}

TEST_F(eval_test, should_eval_function_arguments)
{
    auto fn_name = create_symbol("cleo.eval.test", "fn2");
    auto x1 = create_symbol("cleo.eval.test", "x1");
    auto x2 = create_symbol("cleo.eval.test", "x2");
    Root val;
    val = create_native_function([](const Value *args, std::uint8_t num_args) { return create_list(args, num_args); });
    define(fn_name, *val);
    val = create_int64(101);
    define(x1, *val);
    val = create_int64(102);
    define(x2, *val);

    val = list(fn_name, x1, x2);
    val = eval(*val);

    ASSERT_TRUE(type::List->is(get_value_type(*val)));
    ASSERT_EQ(2, get_list_size(*val));
    ASSERT_EQ(101, get_int64_value(get_list_first(*val)));
    Root next;
    next = get_list_next(*val);
    ASSERT_EQ(102, get_int64_value(get_list_first(*next)));
}

TEST_F(eval_test, quote_should_return_its_second_argument_unevaluated)
{
    Root ex, val;
    ex = create_symbol("cleo.eval.test", "some-symbol");
    val = list(QUOTE, *ex);
    val = eval(*val);
    ASSERT_EQ_REFS(*ex, *val);
}

TEST_F(eval_test, quote_should_fail_when_not_given_one_argument)
{
    Root val;
    val = list(QUOTE, 10, 20);
    ASSERT_ANY_THROW(eval(*val));
    val = list(QUOTE);
    ASSERT_ANY_THROW(eval(*val));
}

TEST_F(eval_test, fn_should_return_a_new_function)
{
    auto s = create_symbol("s");
    auto x = create_symbol("x");
    Root body{list(s, x)};
    Root resolved_body{fn_call(s, x)};
    Root params{array(s, x)};
    Root call{list(FN, *params, *body)};
    Root val{eval(*call)};
    ASSERT_TRUE(type::Fn->is(get_value_type(*val)));
    ASSERT_TRUE(!get_fn_name(*val));
    ASSERT_EQ(1u, get_fn_size(*val));
    ASSERT_TRUE(params->is(get_fn_params(*val, 0)));
    ASSERT_TRUE(*resolved_body == get_fn_body(*val, 0));
}

TEST_F(eval_test, fn_should_return_a_new_function_with_a_name)
{
    auto s = create_symbol("s");
    auto x = create_symbol("x");
    auto name = create_symbol("fname");
    Root body{list(s, x)};
    Root resolved_body{fn_call(s, x)};
    Root params{array(s, x)};
    Root call{list(FN, name, *params, *body)};
    Root val{eval(*call)};
    ASSERT_TRUE(type::Fn->is(get_value_type(*val)));
    ASSERT_TRUE(name.is(get_fn_name(*val)));
    ASSERT_EQ(1u, get_fn_size(*val));
    ASSERT_TRUE(params->is(get_fn_params(*val, 0)));
    ASSERT_TRUE(*resolved_body == get_fn_body(*val, 0));
}

TEST_F(eval_test, fn_should_return_a_new_function_with_multiple_arities)
{
    auto x = create_symbol("x");
    auto y = create_symbol("y");
    Root params1{array()};
    Root params2{array(x)};
    Root params3{array(x, y)};
    Root call{read_str("(fn* xyz ([] :a) ([x] :b) ([x y] :c))")};
    Root val{eval(*call)};
    ASSERT_EQ_VALS(*type::Fn, get_value_type(*val));
    EXPECT_EQ_VALS(create_symbol("xyz"), get_fn_name(*val));
    ASSERT_EQ(3u, get_fn_size(*val));
    EXPECT_EQ_VALS(*params1, get_fn_params(*val, 0));
    EXPECT_EQ_VALS(*params2, get_fn_params(*val, 1));
    EXPECT_EQ_VALS(*params3, get_fn_params(*val, 2));
    EXPECT_EQ_VALS(create_keyword("a"), get_fn_body(*val, 0));
    EXPECT_EQ_VALS(create_keyword("b"), get_fn_body(*val, 1));
    EXPECT_EQ_VALS(create_keyword("c"), get_fn_body(*val, 2));
}

TEST_F(eval_test, fn_should_return_a_new_function_with_no_arities)
{
    Root call{read_str("(fn*)")};
    Root val{eval(*call)};
    ASSERT_EQ_VALS(*type::Fn, get_value_type(*val));
    ASSERT_EQ_VALS(nil, get_fn_name(*val));
    ASSERT_EQ(0u, get_fn_size(*val));

    call = read_str("(fn* some)");
    val = eval(*call);
    ASSERT_EQ_VALS(*type::Fn, get_value_type(*val));
    ASSERT_EQ_VALS(create_symbol("some"), get_fn_name(*val));
    ASSERT_EQ(0u, get_fn_size(*val));
}

TEST_F(eval_test, fn_should_return_a_new_function_with_no_body)
{
    Root params{array()};
    Root call{read_str("(fn* [])")};
    Root val{eval(*call)};
    ASSERT_EQ_VALS(*type::Fn, get_value_type(*val));
    ASSERT_EQ_VALS(nil, get_fn_name(*val));
    ASSERT_EQ(1u, get_fn_size(*val));
    EXPECT_EQ_VALS(*params, get_fn_params(*val, 0));
    EXPECT_EQ_VALS(nil, get_fn_body(*val, 0));

    call = read_str("(fn* some [])");
    val = eval(*call);
    ASSERT_EQ_VALS(*type::Fn, get_value_type(*val));
    ASSERT_EQ_VALS(create_symbol("some"), get_fn_name(*val));
    ASSERT_EQ(1u, get_fn_size(*val));
    EXPECT_EQ_VALS(*params, get_fn_params(*val, 0));
    EXPECT_EQ_VALS(nil, get_fn_body(*val, 0));
}

TEST_F(eval_test, fn_should_fail_when_param_list_is_not_a_vector)
{
    Root call{read_str("(fn* 10 nil)")};
    EXPECT_ANY_THROW(eval(*call));

    call = read_str("(fn* some 10 nil)");
    EXPECT_ANY_THROW(eval(*call));

    call = read_str("(fn* (10 nil))");
    EXPECT_ANY_THROW(eval(*call));

    call = read_str("(fn* some (10 nil))");
    EXPECT_ANY_THROW(eval(*call));
}

TEST_F(eval_test, fn_should_fail_when_param_list_is_not_a_vector_of_unqualified_symbols)
{
    Root call{read_str("(fn* [10] nil)")};
    EXPECT_ANY_THROW(eval(*call));

    call = read_str("(fn* [x y 20] nil)");
    EXPECT_ANY_THROW(eval(*call));

    call = read_str("(fn* [a/x y] nil)");
    EXPECT_ANY_THROW(eval(*call));

    call = read_str("(fn* [x b/y] nil)");
    EXPECT_ANY_THROW(eval(*call));
}

TEST_F(eval_test, fn_should_fail_when_given_too_many_expressions)
{
    Root call{read_str("(fn* [] 10 20)")};
    EXPECT_ANY_THROW(eval(*call));
}

TEST_F(eval_test, should_store_the_environment_in_created_fns)
{
    Root ex{array(3, 4, 5)};
    Root val{read_str("(((fn* [x y z] (fn* [] [x y z])) 3 4 5))")};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("((((fn* [x] (fn* [y] (fn* [z] [x y z]))) 3) 4) 5)");
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, should_store_the_fn_name_in_the_environment)
{
    Root ex{i64(17)};
    Root val{read_str("((fn* abc ([] 10) ([x] (cleo.core/+ x (abc)))) 7)")};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    ex = array(100, 50, 4, 3);
    val = read_str("((((fn* cn ([] 100) ([x] (fn* [y] (fn* [z] [(cn) x y z])))) 50) 4) 3)");
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    ex = i64(13);
    val = read_str("((fn* cn [x] (if (cleo.core/= x 0) 10 (cleo.core/+ x (cn 0)))) 3)");
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, fn_params_should_hide_fn_names)
{
    Root ex{i64(10)};
    Root val{read_str("((fn* x [x] x) 10)")};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, fn_should_fail_when_a_var_does_not_exist)
{
    in_ns(create_symbol("cleo.fn.missing.test"));

    expect_symbol_not_found("(fn* [] bad)");

    expect_symbol_not_found("(fn* [] [bad])");
    expect_symbol_not_found("(fn* [] [nil bad])");
    expect_symbol_not_found("(fn* [] [bad 10 20])");

    expect_symbol_not_found("(fn* [] #{bad})");
    expect_symbol_not_found("(fn* [] #{nil bad})");
    expect_symbol_not_found("(fn* [] #{bad 10 20})");

    expect_symbol_not_found("(fn* [] {bad 10})");
    expect_symbol_not_found("(fn* [] {nil bad})");
    expect_symbol_not_found("(fn* [] {bad 10 20 30})");

    expect_symbol_not_found("(fn* [] [{#{bad} 10}])");

    expect_symbol_not_found("(fn* [] (let* [x a] a))");
    expect_symbol_not_found("(fn* [] (let* [x 10] a))");
}

TEST_F(eval_test, should_resolve_variables)
{
    in_ns(create_symbol("cleo.fn.resolved2.test"));
    auto yvar = define(create_symbol("cleo.fn.resolved2.test", "y"), nil);
    in_ns(create_symbol("cleo.fn.resolved.test"));
    auto xvar = define(create_symbol("cleo.fn.resolved.test", "x"), nil);
    refer(create_symbol("cleo.fn.resolved2.test"));
    Root xref{create_var_value_ref(xvar)};
    Root yref{create_var_value_ref(yvar)};
    auto xsym = create_symbol("x");
    auto ysym = create_symbol("y");
    auto asym = create_symbol("a");
    auto bsym = create_symbol("b");
    auto ffsym = create_symbol("ff");

    expect_symbol_resolved(*xref, "x", "{}");
    expect_symbol_resolved(*yref, "y", "{x nil}");
    expect_symbol_resolved("x", "x", "{x nil}");
    expect_symbol_resolved("y", "y", "{y nil}");

    expect_symbol_resolved("[]", "[]", "{}");
    expect_symbol_resolved(array(*xref, *yref), "[x y]", "{}");
    expect_symbol_resolved(array(xsym, *yref), "[x y]", "{x nil}");

    expect_symbol_resolved("#{}", "#{}", "{}");
    expect_symbol_resolved(aset(*xref, *yref), "#{x y}", "{}");
    expect_symbol_resolved(aset(xsym, *yref), "#{x y}", "{x nil}");

    expect_symbol_resolved("{}", "{}", "{}");
    expect_symbol_resolved(phmap(*xref, 10, 20, *yref), "{x 10 20 y}", "{}");
    expect_symbol_resolved(phmap(xsym, 10, 20, *yref), "{x 10 20 y}", "{x nil}");
    expect_symbol_resolved(phmap(*xref, 10, 20, ysym), "{x 10 20 y}", "{y nil}");

    expect_symbol_resolved("()", "()", "{}");
    expect_symbol_resolved(fn_call(*xref, *yref), "(x y)", "{}");
    expect_symbol_resolved(fn_call(xsym, *yref), "(x y)", "{x nil}");

    expect_symbol_resolved("(quote a x y)", "(quote a x y)", "{}");

    expect_symbol_resolved(list(DEF, xsym, arrayv(xsym, *yref)), "(def x [x y])", "{}");
    expect_symbol_resolved(list(DEF, xsym, arrayv(xsym, ysym)), "(def x [x y])", "{y nil}");
    expect_symbol_resolved(list(DEF, phmapv(create_keyword("some"), 10), xsym, arrayv(xsym, *yref)), "(def {:some 10} x [x y])", "{}");

    expect_symbol_resolved(list(DO, *xref, *yref), "(do x y)", "{}");
    expect_symbol_resolved(list(DO, *xref, ysym), "(do x y)", "{y nil}");

    expect_symbol_resolved(list(IF, *xref, *yref), "(if x y)", "{}");
    expect_symbol_resolved(list(IF, *xref, ysym), "(if x y)", "{y nil}");

    expect_symbol_resolved(fn_call(*recur, *xref, *yref), "(recur x y)", "{}");
    expect_symbol_resolved(fn_call(*recur, *xref, ysym), "(recur x y)", "{y nil}");

    expect_symbol_resolved(list(THROW, *xref, *yref), "(throw x y)", "{}");
    expect_symbol_resolved(list(THROW, *xref, ysym), "(throw x y)", "{y nil}");

    expect_symbol_resolved(list(TRY, *xref, *yref), "(try* x y)", "{}");
    expect_symbol_resolved(list(TRY, *xref, ysym), "(try* x y)", "{y nil}");

    expect_symbol_resolved(list(FINALLY, *xref, *yref), "(finally* x y)", "{}");
    expect_symbol_resolved(list(FINALLY, *xref, ysym), "(finally* x y)", "{y nil}");

    expect_symbol_resolved(list(LET, arrayv(), *xref, *yref), "(let* [] x y)", "{}");
    expect_symbol_resolved(list(LET, arrayv(asym, *xref, bsym, *yref), asym, bsym), "(let* [a x b y] a b)", "{}");
    expect_symbol_resolved(list(LET, arrayv(xsym, *xref, xsym, xsym), xsym), "(let* [x x x x] x)", "{}");
    expect_symbol_resolved(list(LET, arrayv(xsym, 10), xsym, *yref), "(let* [x 10] x y)", "{}");
    expect_symbol_resolved(list(LET, arrayv(ysym, 10), *xref, ysym), "(let* [y 10] x y)", "{}");
    expect_symbol_resolved(list(LET, arrayv(xsym, 10), xsym, ysym), "(let* [x 10] x y)", "{y nil}");

    expect_symbol_resolved(list(LOOP, arrayv(), *xref, *yref), "(loop* [] x y)", "{}");
    expect_symbol_resolved(list(LOOP, arrayv(asym, *xref, bsym, *yref), asym, bsym), "(loop* [a x b y] a b)", "{}");
    expect_symbol_resolved(list(LOOP, arrayv(xsym, *xref, xsym, xsym), xsym), "(loop* [x x x x] x)", "{}");
    expect_symbol_resolved(list(LOOP, arrayv(xsym, 10), xsym, *yref), "(loop* [x 10] x y)", "{}");
    expect_symbol_resolved(list(LOOP, arrayv(ysym, 10), *xref, ysym), "(loop* [y 10] x y)", "{}");
    expect_symbol_resolved(list(LOOP, arrayv(xsym, 10), xsym, ysym), "(loop* [x 10] x y)", "{y nil}");

    expect_symbol_resolved(list(CATCH, *xref, asym, asym, *yref), "(catch* x a a y)", "{}");
    expect_symbol_resolved(list(CATCH, *xref, ysym, ysym), "(catch* x y y)", "{}");
    expect_symbol_resolved(list(CATCH, xsym, ysym, ysym), "(catch* x y y)", "{x nil}");
    expect_symbol_resolved(list(CATCH, *xref, asym, ysym), "(catch* x a y)", "{y nil}");

    expect_symbol_resolved(list(FN, arrayv()), "(fn* [])", "{}");
    expect_symbol_resolved(list(FN, ffsym, arrayv(), ffsym), "(fn* ff [] ff)", "{}");
    expect_symbol_resolved(list(FN, ffsym, listv(arrayv(), ffsym), listv(arrayv(xsym), ffsym)), "(fn* ff ([] ff) ([x] ff))", "{}");
    expect_symbol_resolved(list(FN, arrayv(), *xref, *yref), "(fn* [] x y)", "{}");
    expect_symbol_resolved(list(FN, arrayv(xsym), xsym, *yref), "(fn* [x] x y)", "{}");
    expect_symbol_resolved(list(FN, arrayv(xsym, ysym), arrayv(xsym, ysym)), "(fn* [x y] [x y])", "{}");
    expect_symbol_resolved(list(FN, arrayv(xsym), xsym, ysym), "(fn* [x] x y)", "{y nil}");
    expect_symbol_resolved(list(FN, listv(arrayv())), "(fn* ([]))", "{}");
    expect_symbol_resolved(list(FN, listv(arrayv(xsym), xsym, *yref), listv(arrayv(ysym), *xref, ysym), listv(arrayv(), *xref, *yref), listv(arrayv(xsym, ysym), xsym, ysym)), "(fn* ([x] x y) ([y] x y) ([] x y) ([x y] x y))", "{}");
    expect_symbol_resolved(list(FN, listv(arrayv(xsym), xsym, ysym), listv(arrayv(ysym), *xref, ysym), listv(arrayv(), *xref, ysym), listv(arrayv(xsym, ysym), xsym, ysym)), "(fn* ([x] x y) ([y] x y) ([] x y) ([x y] x y))", "{y nil}");
}

TEST_F(eval_test, resolving_should_expand_macros)
{
    in_ns(create_symbol("cleo.fn.resolving-macros.test"));
    auto avar = define(create_symbol("cleo.fn.resolving-macros.test", "a"), nil);
    auto bvar = define(create_symbol("cleo.fn.resolving-macros.test", "b"), nil);
    Root aref{create_var_value_ref(avar)};
    Root bref{create_var_value_ref(bvar)};
    Root plusref{create_var_value_ref(get_var(PLUS))};
    Root add{create_string("(fn* [&form &env x y] `(cleo.core/+ ~x ~y))")};
    add = read(*add);
    add = eval(*add);
    Root meta{amap(MACRO_KEY, TRUE)};
    define(create_symbol("cleo.fn.resolving-macros.test", "add"), *add, *meta);

    auto b = create_symbol("b");
    auto c = create_symbol("c");
    auto d = create_symbol("d");
    auto addsym = create_symbol("add");

    expect_symbol_resolved(list(FN, arrayv(c, d), fn_callv(*plusref, fn_callv(*plusref, *aref, *bref), fn_callv(*plusref, c, d))), "(fn* [c d] (add (add a b) (add c d)))", "{}");
    expect_symbol_resolved(list(FN, arrayv(addsym), fn_callv(addsym, *aref, *bref)), "(fn* [add] (add a b))", "{}");
    expect_symbol_resolved(list(FN, arrayv(addsym), fn_callv(addsym, *aref, b)), "(fn* [add] (add a b))", "{b nil}");
}

TEST_F(eval_test, resolving_should_fail_when_passing_macros_as_arguments)
{
    in_ns(create_symbol("cleo.fn.failing-macro-args.test"));
    Root add{create_string("(fn* [&form &env x y] `(cleo.core/+ ~x ~y))")};
    add = read(*add);
    add = eval(*add);
    Root meta{amap(MACRO_KEY, TRUE)};
    define(create_symbol("cleo.fn.failing-macro-args.test", "add"), *add, *meta);

    expect_resolve_illegal_state("(if 10 add nil)");
    expect_resolve_illegal_state("(let* [x add] nil)");
}

TEST_F(eval_test, should_expand_all_macros)
{
    in_ns(create_symbol("cleo.fn.macros.test"));
    Root add{create_string("(fn* [&form &env x y] `(cleo.core/+ ~x ~y))")};
    add = read(*add);
    add = eval(*add);
    Root meta{amap(MACRO_KEY, TRUE)};
    define(create_symbol("cleo.fn.macros.test", "add"), *add, *meta);
    Root fn{create_string("(fn* [] (add (add 1 2) (add 3 4)))")};
    fn = read(*fn);
    fn = eval(*fn);
    define(create_symbol("cleo.fn.macros.test", "add"), nil);
    Root val{list(*fn)}, ex;
    val = eval(*val);
    ex = create_int64(10);
    EXPECT_EQ_VALS(*ex, *val);
}


TEST_F(eval_test, should_eval_an_empty_list_as_an_empty_list)
{
    Root ex{list()};
    Root val{eval(*ex)};
    ASSERT_TRUE(ex->is(*val));
}

TEST_F(eval_test, should_eval_vectors)
{
    Root x{create_int64(55)};
    auto xs = create_symbol("x");
    Root ex{array(*rt::seq, *rt::first, *x)};
    Root val{array(SEQ, FIRST, xs)};
    Root env{amap(xs, *x)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);

    ex = array();
    val = eval(*ex);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, should_eval_set)
{
    Root x{create_int64(55)};
    auto xs = create_symbol("x");
    Root ex{aset(*rt::seq, *rt::first, *x)};
    Root val{aset(SEQ, FIRST, xs)};
    Root env{amap(xs, *x)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);

    ex = aset();
    val = eval(*ex);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, should_define_vars_in_the_current_ns)
{
    Root ex{create_int64(55)};
    auto x = create_symbol("x");
    Root env{amap(x, *ex)};
    in_ns(create_symbol("clue.eval.test"));
    Root val{read_str("(def var1 ((fn* [] x)))")};
    val = eval(*val, *env);
    auto name = create_symbol("clue.eval.test", "var1");
    EXPECT_EQ_VALS(get_var(name), *val);
    EXPECT_FALSE(bool(is_var_macro(*val)));
    EXPECT_EQ_VALS(*ex, lookup(name));
}

TEST_F(eval_test, def_should_fail_when_ns_is_specified)
{
    auto x = create_symbol("x");
    Root env{amap(x, 55)};
    in_ns(create_symbol("clue.eval.test"));
    Root val{read_str("(def clue.eval.test.other/var2 ((fn* [] x)))")};
    try
    {
        eval(*val, *env);
        FAIL() << "expected IllegalArgument";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_VALS(*type::IllegalArgument, get_value_type(*e));
    }
    ASSERT_THROW(lookup(create_symbol("clue.eval.test.other", "var2")), Exception);
}

TEST_F(eval_test, def_should_fail_when_current_ns_is_nil)
{
    rt::current_ns = nil;
    Root val{read_str("(def var4 10)")};
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, def_should_fail_when_the_name_is_not_provided)
{
    Root val{read_str("(def)")};
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, def_should_fail_when_name_is_not_a_symbol)
{
    Root val{read_str("(def 20 10)")};
    ASSERT_THROW(eval(*val), Exception);

    val = read_str("(def {})");
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, def_should_fail_when_given_too_many_arguments)
{
    Root val{read_str("(def x 10 20 30)")};
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, def_should_define_a_var_with_nil_value_when_not_given_a_value)
{
    in_ns(create_symbol("clue.eval.var.default.test"));
    Root val{read_str("(def var1)")};
    val = eval(*val);
    auto name = create_symbol("clue.eval.var.default.test", "var1");
    EXPECT_EQ_VALS(get_var(name), *val);
    EXPECT_EQ_VALS(nil, lookup(name));
}

TEST_F(eval_test, def_should_define_a_var_with_meta)
{
    in_ns(create_symbol("clue.eval.var.meta.test"));
    Root val, ex;
    val = read_str("(def {:macro :true} var1)");
    val = eval(*val);
    auto name = create_symbol("clue.eval.var.meta.test", "var1");
    EXPECT_EQ_VALS(get_var(name), *val);
    EXPECT_TRUE(bool(is_var_macro(*val)));
    EXPECT_EQ_VALS(nil, lookup(name));

    val = read_str("(def {:macro :true} var2 30)");
    val = eval(*val);
    ex = i64(30);
    name = create_symbol("clue.eval.var.meta.test", "var2");
    EXPECT_EQ_VALS(get_var(name), *val);
    EXPECT_TRUE(bool(is_var_macro(*val)));
    EXPECT_EQ_VALS(*ex, lookup(name));
}

TEST_F(eval_test, def_should_fail_when_meta_is_not_a_map)
{
    in_ns(create_symbol("clue.eval.var.meta.test"));
    Root val{read_str("(def 10 var1 7)")};
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, def_should_not_fail_when_the_specified_ns_is_the_same_as_the_current_one)
{
    Root ex{create_int64(55)};
    auto x = create_symbol("x");
    Root env{amap(x, *ex)};
    in_ns(create_symbol("clue.eval.test"));
    Root val{read_str("(def clue.eval.test/var3 ((fn* [] x)))")};
    val = eval(*val, *env);
    auto name = create_symbol("clue.eval.test", "var3");
    EXPECT_EQ_VALS(get_var(name), *val);
    EXPECT_EQ_VALS(*ex, lookup(create_symbol("clue.eval.test", "var3")));
}

TEST_F(eval_test, should_eval_maps)
{
    Root x{create_int64(55)};
    Root y{create_int64(77)};
    auto xs = create_symbol("x");
    auto ys = create_symbol("y");
    Root ex{amap(*rt::seq, *rt::first, *x, *y)};
    Root val{amap(SEQ, FIRST, xs, ys)};
    Root env{amap(xs, *x, ys, *y)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);

    ex = amap();
    val = eval(*ex);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, should_eval_let)
{
    Root val{read_str("(let* [a ((fn* [] 55))] a)")};
    Root ex{create_int64(55)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("(let* [x 10 y 20] {:a x, :b y, :c z})");
    ex = amap(create_keyword("a"), 10, create_keyword("b"), 20, create_keyword("c"), 30);
    Root env{amap(create_symbol("x"), -1, create_symbol("y"), -1, create_symbol("z"), 30)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, let_should_allow_rebinding)
{
    Root val{read_str("(let* [a 10 a [a]] a)")};
    Root ex{read_str("[10]")};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, let_should_fail_when_not_given_bindings_and_body)
{
    Root val{read_str("(let*)")};
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("(let* [a 10])");
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, let_should_fail_when_given_too_many_expressions)
{
    Root val{read_str("(let* [a 10] a 10)")};
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, let_should_fail_when_bindings_are_not_a_vector)
{
    Root val{read_str("(let* {a 10} a)")};
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, let_should_fail_when_the_last_binding_is_missing_a_value)
{
    Root val{read_str("(let* [a] a)")};
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("(let* [a 10 b] a)");
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, let_should_fail_when_the_binding_names_are_not_unqualified_symbols)
{
    Root val{read_str("(let* [10 20] 10)")};
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("(let* [a/x 10] a/x)");
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, should_eval_loop)
{
    Root val{read_str("(loop* [a ((fn* [] 55))] a)")};
    Root ex{create_int64(55)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("(loop* [x 10 y 20] {:a x, :b y, :c z})");
    ex = amap(create_keyword("a"), 10, create_keyword("b"), 20, create_keyword("c"), 30);
    Root env{amap(create_symbol("x"), -1, create_symbol("y"), -1, create_symbol("z"), 30)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, loop_should_allow_rebinding)
{
    Root val{read_str("(loop* [a 10 a [a]] a)")};
    Root ex{read_str("[10]")};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, loop_should_fail_when_not_given_bindings_and_body)
{
    Root val{read_str("(loop*)")};
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("(loop* [a 10])");
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, loop_should_fail_when_given_too_many_expressions)
{
    Root val{read_str("(loop* [a 10] a 10)")};
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, loop_should_fail_when_bindings_are_not_a_vector)
{
    Root val{read_str("(loop* {a 10} a)")};
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, loop_should_fail_when_the_last_binding_is_missing_a_value)
{
    Root val{read_str("(loop* [a] a)")};
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("(loop* [a 10 b] a)");
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, loop_should_fail_when_the_binding_names_are_not_unqualified_symbols)
{
    Root val{read_str("(loop* [10 20] 10)")};
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("(loop* [a/x 10] a/x)");
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, recur_should_rebind_the_bindings_of_loop_and_reevaluate_it)
{
    Root val{read_str("(loop* [n 5 r 1] (if (cleo.core/= n 0) r (recur (cleo.core/- n 1) (cleo.core/* r n))))")};
    Root ex{create_int64(5 * 4 * 3 * 2 * 1)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, recur_should_fail_when_the_number_of_bindings_does_not_match_a_loop)
{
    Root val{read_str("(loop* [n 5] (recur))")};
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("(loop* [n 5] (recur 1 2))");
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, recur_should_rebind_the_bindings_of_fn_and_reevaluate_it)
{
    Root val{read_str("((fn* [n r] (if (cleo.core/= n 0) r (recur (cleo.core/- n 1) (cleo.core/* r n)))) 5 1)")};
    Root ex{create_int64(5 * 4 * 3 * 2 * 1)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, recur_should_rebind_the_bindings_of_fn_with_varargs_and_reevaluate_it)
{
    Root val{read_str("((fn* [cond & xs] (if cond xs (recur :true xs))) nil 1 2 3)")};
    Root ex{list(1, 2, 3)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("((fn* [& xs] (if xs xs (recur [1 2 3]))))");
    val = eval(*val);
    ex = array(1, 2, 3);
    EXPECT_EQ_VALS(*type::Array, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, recur_should_fail_when_the_number_of_bindings_does_not_match_a_fn)
{
    Root val{read_str("((fn* [n 5] (recur)) 7)")};
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("((fn* [n 5] (recur 1 2)) 7)");
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("((fn* [n & ns] (recur 1 2 3)) 7)");
    EXPECT_THROW(eval(*val), Exception);

    val = read_str("((fn* [n & ns] (recur 1)) 7)");
    EXPECT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, should_eval_if)
{
    Root bad{create_native_function([](const Value *, std::uint8_t) -> Force { throw std::runtime_error("should not have been evaluated"); })};
    Root val{read_str("(if c a (bad))")};
    Root ex{create_int64(55)};
    Root env{amap(create_symbol("c"), 11111, create_symbol("a"), 55, create_symbol("bad"), *bad)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("(if c (bad) a)");
    env = amap(create_symbol("c"), nil, create_symbol("a"), 55, create_symbol("bad"), *bad);
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, if_should_return_nil_when_the_condition_is_false_and_there_is_no_else_value)
{
    Root val{read_str("(if nil 10)")};
    val = eval(*val);
    EXPECT_EQ_VALS(nil, *val);
}

TEST_F(eval_test, if_should_fail_when_given_too_few_arguments)
{
    Root val{read_str("(if)")};
    ASSERT_THROW(eval(*val), Exception);

    val = read_str("(if 1)");
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, if_should_fail_when_given_too_many_arguments)
{
    Root val{read_str("(if 3 4 5 6)")};
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, should_eval_throw)
{
    Root val{read_str("(throw x)")};
    Root env{amap(create_symbol("x"), 107)};
    Root ex{create_int64(107)};
    try
    {
        eval(*val, *env);
        FAIL() << "expected an exception";
    }
    catch (const Exception& e)
    {
        env = nil;
        val = nil;
        gc();
        EXPECT_EQ_VALS(*ex, *Root(catch_exception()));
    }
}

TEST_F(eval_test, should_eval_try_catch)
{
    Root val{read_str("(try* (throw x) (catch* cleo.core/Int64 e (cleo.core/+ e a)))")};
    Root env{amap(create_symbol("x"), 107, create_symbol("a"), 2)};
    Root ex{create_int64(109)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(nil, *Root(catch_exception()));

    val = read_str("(try* (throw [1 2]) (catch* cleo.core/Seqable x x))");
    ex = array(1, 2);
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(nil, *Root(catch_exception()));

    val = read_str("(try* (throw [1 2]) (catch* cleo.core/Int64 x x))");
    EXPECT_THROW(eval(*val), Exception);
    EXPECT_EQ_VALS(*ex, *Root(catch_exception()));

    val = read_str("(try* 777 (catch* cleo.core/Int64 x x))");
    ex = create_int64(777);
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(nil, *Root(catch_exception()));
}

TEST_F(eval_test, should_eval_catch_with_a_var_ref_as_type)
{
    Root val{read_str("(try* (throw x) (catch* cleo.core/Int64 e (cleo.core/+ e a)))")};
    Root env{amap(create_symbol("x"), 107, create_symbol("a"), 2)};
    Root ex{create_int64(109)};
    val = resolve_value(*val, *env);
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(nil, *Root(catch_exception()));
}

TEST_F(eval_test, should_eval_do)
{
    Root env{amap(create_symbol("a"), create_keyword("z"))};
    Root val, ex;
    val = read_str("(do)");
    val = eval(*val);
    EXPECT_EQ_VALS(nil, *val);

    val = read_str("(do a)");
    val = eval(*val, *env);
    EXPECT_EQ_VALS(create_keyword("z"), *val);

    val = read_str("(do (cleo.core/in-ns 'cleo.eval.do.test) a)");
    val = eval(*val, *env);
    EXPECT_EQ_VALS(create_keyword("z"), *val);
    EXPECT_EQ_VALS(create_symbol("cleo.eval.do.test"), ns_name(*rt::current_ns));
}

namespace
{
bool finally_called_too_early = false;
bool finally_called = false;

Force on_before_finally(Value)
{
    finally_called_too_early = finally_called;
    return nil;
}

Force on_finally(Value)
{
    finally_called = true;
    return nil;
}
}

TEST_F(eval_test, should_eval_try_finally)
{
    Root call_me{create_native_function1<on_finally>()};
    Root call_me_before{create_native_function1<on_before_finally>()};
    Root env{amap(create_symbol("call-me"), *call_me, create_symbol("call-me-before"), *call_me_before)};

    Root val{read_str("(try* (throw [1 2]) (finally* (call-me nil)))")};
    Root ex{array(1, 2)};
    finally_called = false;
    try
    {
        val = eval(*val, *env);
    }
    catch (const Exception& )
    {
        EXPECT_EQ_VALS(*ex, *Root(catch_exception()));
    }
    EXPECT_TRUE(finally_called);

    finally_called = false;
    val = read_str("(try* [1 2] (finally* (call-me nil)))");
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_TRUE(finally_called);

    finally_called = false;
    finally_called_too_early = false;
    val = read_str("(try* (call-me-before nil) (finally* (call-me nil)))");
    eval(*val, *env);
    EXPECT_FALSE(finally_called_too_early);
}

TEST_F(eval_test, try_should_fail_when_given_too_few_or_too_many_expressions)
{
    Root val{read_str("(try*)")};
    ASSERT_THROW(eval(*val), Exception);

    val = read_str("(try* 10)");
    ASSERT_THROW(eval(*val), Exception);

    val = read_str("(try* 10 (catch* cleo.core/Int64 e e) (finally* nil))");
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, try_should_fail_when_the_second_for_is_neither_catch_nor_finally)
{
    Root val{read_str("(try* 10 (bad 20))")};
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, try_should_fail_when_finally_has_too_few_or_too_many_expressions)
{
    Root val{read_str("(try* 10 (finally*))")};
    ASSERT_THROW(eval(*val), Exception);

    val = read_str("(try* 10 (finally* 20 30))");
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, try_should_fail_when_catch_is_malformed)
{
    Root val{read_str("(try* 10 (catch*))")};
    ASSERT_THROW(eval(*val), Exception);

    val = read_str("(try* 10 (catch* some))");
    ASSERT_THROW(eval(*val), Exception);

    val = read_str("(try* 10 (catch* some binding))");
    ASSERT_THROW(eval(*val), Exception);

    val = read_str("(try* 10 (catch* some :not-binding 20))");
    ASSERT_THROW(eval(*val), Exception);

    val = read_str("(try* 10 (catch* some bad/binding 20))");
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, load_should_read_and_eval_all_forms_in_the_source_code)
{
    in_ns(create_symbol("cleo.eval.load.test"));
    Root source{create_string("(def y :xyz) (cleo.core/in-ns 'cleo.eval.load2.test) (def x :abc) x")};
    Root val{load(*source)};
    auto ex = create_keyword("abc");
    EXPECT_EQ_VALS(ex, *val);
    EXPECT_EQ_VALS(ex, get_var_value(get_var(create_symbol("cleo.eval.load2.test", "x"))));
    EXPECT_EQ_VALS(create_keyword("xyz"), get_var_value(get_var(create_symbol("cleo.eval.load.test", "y"))));
    EXPECT_EQ_VALS(create_symbol("cleo.eval.load.test"), ns_name(*rt::current_ns));
}

TEST_F(eval_test, apply_should_call_functions)
{
    Root fn{create_native_function([](const Value *args, std::uint8_t num_args) { return create_list(args, num_args); })};
    Root args{array(4, 3, 2, 1)};
    Root val{apply(*fn, *args)};
    Root ex{list(4, 3, 2, 1)};
    ASSERT_EQ_VALS(*ex, *val);

    args = list(4);
    val = apply(*fn, *args);
    ex = list(4);
    ASSERT_EQ_VALS(*ex, *val);

    args = array();
    val = apply(*fn, *args);
    ex = list();
    ASSERT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, apply_should_not_reevaluate_params)
{
    Root fn{create_native_function([](const Value *args, std::uint8_t num_args) { return create_list(args, num_args); })};
    Root l{list(3, 2)};
    Root args{array(*l, 3, 2, 1)};
    Root val{apply(*fn, *args)};
    Root ex{list(*l, 3, 2, 1)};
    ASSERT_EQ_VALS(*ex, *val);
}


}
}
