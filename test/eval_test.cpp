#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include <cleo/list.hpp>
#include <cleo/equality.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include <cleo/fn.hpp>
#include <cleo/macro.hpp>
#include <cleo/reader.hpp>
#include <cleo/small_map.hpp>
#include <cleo/memory.hpp>
#include <cleo/reader.hpp>
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
};

TEST_F(eval_test, should_eval_simple_values_to_themselves)
{
    Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); })};
    auto kw = create_keyword("org.xyz", "eqkw");
    Root i{create_int64(7)};
    Root flt{create_float64(3.5)};
    Root s{create_string("abcd")};

    ASSERT_TRUE(nil == *Root(eval(nil)));
    ASSERT_TRUE(*fn == *Root(eval(*fn)));
    ASSERT_TRUE(kw == *Root(eval(kw)));
    ASSERT_TRUE(*i == *Root(eval(*i)));
    ASSERT_TRUE(*flt == *Root(eval(*flt)));
    ASSERT_TRUE(*s == *Root(eval(*s)));
}

TEST_F(eval_test, should_eval_objects_to_themselves)
{
    auto sym = create_symbol("cleo.eval.test", "obj");
    Root o{create_object0(sym)};
    ASSERT_TRUE(*o == *Root(eval(*o)));
}

TEST_F(eval_test, should_eval_symbols_to_var_values)
{
    auto sym = create_symbol("cleo.eval.test", "seven");
    Root val;
    val = create_int64(7);
    define(sym, *val);
    ASSERT_TRUE(*val == *Root(eval(sym)));
}

TEST_F(eval_test, should_eval_symbols_in_the_current_ns)
{
    auto sym = create_symbol("cleo.eval.test", "eight");
    Root val;
    val = create_int64(8);
    define(sym, *val);
    in_ns(create_symbol("cleo.eval.test"));
    ASSERT_TRUE(*val == *Root(eval(create_symbol("eight"))));
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
        ASSERT_EQ(type::SymbolNotFound, get_value_type(*e));
    }
}

TEST_F(eval_test, should_eval_lists_as_function_calls)
{
    Root fn{create_native_function([](const Value *args, std::uint8_t num_args) { return num_args ? create_list(args, num_args) : force(nil); })};

    Root l{list(*fn)};
    ASSERT_TRUE(nil == *Root(eval(*l)));

    Root e0, e1, val;
    e0 = create_int64(101);
    e1 = create_int64(102);
    val = list(*fn, *e0, *e1);
    e0 = nil;
    e1 = nil;
    val = eval(*val);

    ASSERT_TRUE(type::LIST == get_value_type(*val));
    ASSERT_EQ(2, get_int64_value(get_list_size(*val)));
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
        ASSERT_EQ(type::CallError, get_value_type(*e));
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

    ASSERT_TRUE(type::LIST == get_value_type(*val));
    ASSERT_EQ(2, get_int64_value(get_list_size(*val)));
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
    ASSERT_EQ(*ex, *val);
}

TEST_F(eval_test, fn_should_return_a_new_function)
{
    auto s = create_symbol("s");
    auto x = create_symbol("x");
    Root body{list(s, x)};
    Root params{svec(s, x)};
    Root call{list(FN, *params, *body)};
    Root val{eval(*call)};
    ASSERT_TRUE(type::FN == get_value_type(*val));
    ASSERT_TRUE(nil == get_fn_name(*val));
    ASSERT_EQ(1u, get_fn_size(*val));
    ASSERT_TRUE(*params == get_fn_params(*val, 0));
    ASSERT_TRUE(*body == get_fn_body(*val, 0));
}

TEST_F(eval_test, fn_should_return_a_new_function_with_a_name)
{
    auto s = create_symbol("s");
    auto x = create_symbol("x");
    auto name = create_symbol("fname");
    Root body{list(s, x)};
    Root params{svec(s, x)};
    Root call{list(FN, name, *params, *body)};
    Root val{eval(*call)};
    ASSERT_TRUE(type::FN == get_value_type(*val));
    ASSERT_TRUE(name == get_fn_name(*val));
    ASSERT_EQ(1u, get_fn_size(*val));
    ASSERT_TRUE(*params == get_fn_params(*val, 0));
    ASSERT_TRUE(*body == get_fn_body(*val, 0));
}

TEST_F(eval_test, fn_should_return_a_new_function_with_multiple_arities)
{
    auto x = create_symbol("x");
    auto y = create_symbol("y");
    Root params1{svec()};
    Root params2{svec(x)};
    Root params3{svec(x, y)};
    Root call{create_string("(fn xyz ([] :a) ([x] :b) ([x y] :c))")};
    call = read(*call);
    Root val{eval(*call)};
    ASSERT_EQ_VALS(type::FN, get_value_type(*val));
    EXPECT_EQ_VALS(create_symbol("xyz"), get_fn_name(*val));
    ASSERT_EQ(3u, get_fn_size(*val));
    EXPECT_EQ_VALS(*params1, get_fn_params(*val, 0));
    EXPECT_EQ_VALS(*params2, get_fn_params(*val, 1));
    EXPECT_EQ_VALS(*params3, get_fn_params(*val, 2));
    EXPECT_EQ_VALS(create_keyword("a"), get_fn_body(*val, 0));
    EXPECT_EQ_VALS(create_keyword("b"), get_fn_body(*val, 1));
    EXPECT_EQ_VALS(create_keyword("c"), get_fn_body(*val, 2));
}

TEST_F(eval_test, macro_should_return_a_new_macro)
{
    auto s = create_symbol("s");
    auto x = create_symbol("x");
    Root body{list(s, x)};
    Root params{svec(s, x)};
    Root call{list(MACRO, *params, *body)};
    Root val{eval(*call)};
    ASSERT_TRUE(type::Macro == get_value_type(*val));
    ASSERT_TRUE(nil == get_macro_name(*val));
    ASSERT_TRUE(*params == get_macro_params(*val));
    ASSERT_TRUE(*body == get_macro_body(*val));
}

TEST_F(eval_test, macro_should_return_a_new_macro_with_a_name)
{
    auto s = create_symbol("s");
    auto x = create_symbol("x");
    auto name = create_symbol("fname");
    Root body{list(s, x)};
    Root params{svec(s, x)};
    Root call{list(MACRO, name, *params, *body)};
    Root val{eval(*call)};
    ASSERT_TRUE(type::Macro == get_value_type(*val));
    ASSERT_TRUE(name == get_macro_name(*val));
    ASSERT_TRUE(*params == get_macro_params(*val));
    ASSERT_TRUE(*body == get_macro_body(*val));
}

TEST_F(eval_test, should_store_the_environment_in_created_fns)
{
    Root ex{svec(3, 4, 5)};
    Root val{read_str("(((fn [x y z] (fn [] [x y z])) 3 4 5))")};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("((((fn [x] (fn [y] (fn [z] [x y z]))) 3) 4) 5)");
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, should_eval_an_empty_list_as_an_empty_list)
{
    Root ex{list()};
    Root val{eval(*ex)};
    ASSERT_TRUE(*ex == *val);
}

TEST_F(eval_test, should_eval_vectors)
{
    Root x{create_int64(55)};
    auto xs = create_symbol("x");
    Root ex{svec(lookup_var(SEQ), lookup_var(FIRST), *x)};
    Root val{svec(SEQ, FIRST, xs)};
    Root env{smap(xs, *x)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);

    ex = svec();
    val = eval(*ex);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, should_eval_set)
{
    Root x{create_int64(55)};
    auto xs = create_symbol("x");
    Root ex{sset(lookup_var(SEQ), lookup_var(FIRST), *x)};
    Root val{sset(SEQ, FIRST, xs)};
    Root env{smap(xs, *x)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);

    ex = sset();
    val = eval(*ex);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, should_define_vars_in_the_current_ns)
{
    Root ex{create_int64(55)};
    auto x = create_symbol("x");
    Root env{smap(x, *ex)};
    in_ns(create_symbol("clue.eval.test"));
    Root val{read_str("(def var1 ((fn [] x)))")};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(*ex, lookup(create_symbol("clue.eval.test", "var1")));
}

TEST_F(eval_test, def_should_fail_when_ns_is_specified)
{
    auto x = create_symbol("x");
    Root env{smap(x, 55)};
    in_ns(create_symbol("clue.eval.test"));
    Root val{read_str("(def clue.eval.test.other/var2 ((fn [] x)))")};
    try
    {
        eval(*val, *env);
        FAIL() << "expected IllegalArgument";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_VALS(type::IllegalArgument, get_value_type(*e));
    }
    ASSERT_THROW(lookup(create_symbol("clue.eval.test.other", "var2")), Exception);
}

TEST_F(eval_test, def_should_fail_when_current_ns_is_nil)
{
    in_ns(nil);
    Root val{read_str("(def var4 10)")};
    ASSERT_THROW(eval(*val), Exception);
}

TEST_F(eval_test, def_should_not_fail_when_the_specified_ns_is_the_same_as_the_current_one)
{
    Root ex{create_int64(55)};
    auto x = create_symbol("x");
    Root env{smap(x, *ex)};
    in_ns(create_symbol("clue.eval.test"));
    Root val{read_str("(def clue.eval.test/var3 ((fn [] x)))")};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(*ex, lookup(create_symbol("clue.eval.test", "var3")));
}

TEST_F(eval_test, should_eval_maps)
{
    Root x{create_int64(55)};
    Root y{create_int64(77)};
    auto xs = create_symbol("x");
    auto ys = create_symbol("y");
    Root ex{smap(lookup_var(SEQ), lookup_var(FIRST), *x, *y)};
    Root val{smap(SEQ, FIRST, xs, ys)};
    Root env{smap(xs, *x, ys, *y)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);

    ex = smap();
    val = eval(*ex);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, should_eval_let)
{
    Root val{read_str("(let [a ((fn [] 55))] a)")};
    Root ex{create_int64(55)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("(let [x 10 y 20] {:a x, :b y, :c z})");
    ex = smap(create_keyword("a"), 10, create_keyword("b"), 20, create_keyword("c"), 30);
    Root env{smap(create_symbol("x"), -1, create_symbol("y"), -1, create_symbol("z"), 30)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, let_should_allow_rebinding)
{
    Root val{read_str("(let [a 10 a [a]] a)")};
    Root ex{read_str("[10]")};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, should_eval_loop)
{
    Root val{read_str("(loop [a ((fn [] 55))] a)")};
    Root ex{create_int64(55)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("(loop [x 10 y 20] {:a x, :b y, :c z})");
    ex = smap(create_keyword("a"), 10, create_keyword("b"), 20, create_keyword("c"), 30);
    Root env{smap(create_symbol("x"), -1, create_symbol("y"), -1, create_symbol("z"), 30)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, loop_should_allow_rebinding)
{
    Root val{read_str("(loop [a 10 a [a]] a)")};
    Root ex{read_str("[10]")};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, recur_should_rebind_the_bindings_of_loop_and_reevaluate_it)
{
    Root val{read_str("(loop [n 5 r 1] (if (cleo.core/= n 0) r (recur (cleo.core/- n 1) (cleo.core/* r n))))")};
    Root ex{create_int64(5 * 4 * 3 * 2 * 1)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, recur_should_rebind_the_bindings_of_fn_and_reevaluate_it)
{
    Root val{read_str("((fn [n r] (if (cleo.core/= n 0) r (recur (cleo.core/- n 1) (cleo.core/* r n)))) 5 1)")};
    Root ex{create_int64(5 * 4 * 3 * 2 * 1)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, recur_should_rebind_the_bindings_of_fn_with_varargs_and_reevaluate_it)
{
    Root val{read_str("((fn [cond & xs] (if cond xs (recur :true 1 2 3))) nil)")};
    Root ex{list(1, 2, 3)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("((fn [& xs] (if xs xs (recur 1 2 3))))");
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("((fn [cond & xs] (if cond xs (recur :true))) nil)");
    val = eval(*val);
    EXPECT_EQ_VALS(nil, *val);
}

TEST_F(eval_test, should_eval_if)
{
    Root bad{create_native_function([](const Value *, std::uint8_t) -> Force { throw std::runtime_error("should not have been evaluated"); })};
    Root val{read_str("(if c a (bad))")};
    Root ex{create_int64(55)};
    Root env{smap(create_symbol("c"), 11111, create_symbol("a"), 55, create_symbol("bad"), *bad)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);

    val = read_str("(if c (bad) a)");
    env = smap(create_symbol("c"), nil, create_symbol("a"), 55, create_symbol("bad"), *bad);
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(eval_test, if_should_return_nil_when_the_condition_is_false_and_there_is_no_else_value)
{
    Root val{read_str("(if nil 10)")};
    val = eval(*val);
    EXPECT_EQ_VALS(nil, *val);
}

TEST_F(eval_test, should_eval_throw)
{
    Root val{read_str("(throw x)")};
    Root env{smap(create_symbol("x"), 107)};
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
    Root val{read_str("(try (throw x) (catch cleo.core/Int64 e (cleo.core/+ e a)))")};
    Root env{smap(create_symbol("x"), 107, create_symbol("a"), 2)};
    Root ex{create_int64(109)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(nil, *Root(catch_exception()));

    val = read_str("(try (throw [1 2]) (catch cleo.core/Seqable x x))");
    ex = svec(1, 2);
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(nil, *Root(catch_exception()));

    val = read_str("(try (throw [1 2]) (catch cleo.core/Int64 x x))");
    EXPECT_THROW(eval(*val), Exception);
    EXPECT_EQ_VALS(*ex, *Root(catch_exception()));

    val = read_str("(try 777 (catch cleo.core/Int64 x x))");
    ex = create_int64(777);
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(nil, *Root(catch_exception()));
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
    Root env{smap(create_symbol("call-me"), *call_me, create_symbol("call-me-before"), *call_me_before)};

    Root val{read_str("(try (throw [1 2]) (finally (call-me nil)))")};
    Root ex{svec(1, 2)};
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
    val = read_str("(try [1 2] (finally (call-me nil)))");
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_TRUE(finally_called);

    finally_called = false;
    finally_called_too_early = false;
    val = read_str("(try (call-me-before nil) (finally (call-me nil)))");
    eval(*val, *env);
    EXPECT_FALSE(finally_called_too_early);
}

}
}
