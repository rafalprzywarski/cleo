#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include <cleo/list.hpp>
#include <cleo/equality.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include <cleo/fn.hpp>
#include <cleo/reader.hpp>
#include <cleo/small_map.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct eval_test : Test
{
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

TEST_F(eval_test, should_fail_when_a_symbol_cannot_be_resolved)
{
    auto sym = create_symbol("cleo.eval.test", "missing");
    try
    {
        eval(sym);
        FAIL() << "expected an exception";
    }
    catch (const SymbolNotFound& e)
    {
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
    catch (const CallError& e)
    {
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
    ASSERT_TRUE(*params == get_fn_params(*val));
    ASSERT_TRUE(*body == get_fn_body(*val));
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
    ASSERT_TRUE(*params == get_fn_params(*val));
    ASSERT_TRUE(*body == get_fn_body(*val));
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
    Root ex{svec(lookup(SEQ), lookup(FIRST), *x)};
    Root val{svec(SEQ, FIRST, xs)};
    Root env{smap(xs, *x)};
    val = eval(*val, *env);
    EXPECT_EQ_VALS(*ex, *val);

    ex = svec();
    val = eval(*ex);
    EXPECT_EQ_VALS(*ex, *val);
}

}
}
