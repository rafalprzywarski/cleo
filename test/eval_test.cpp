#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include <cleo/list.hpp>
#include <cleo/equality.hpp>
#include <cleo/global.hpp>
#include <cleo/error.hpp>
#include <gtest/gtest.h>
#include <array>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct eval_test : Test {};

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
    auto fn_name = create_symbol("cleo.eval.test", "fn1");
    Root fn{create_native_function([](const Value *args, std::uint8_t num_args) { return num_args ? create_list(args, num_args) : force(nil); })};
    define(fn_name, *fn);

    Root l{list(fn_name)};
    ASSERT_TRUE(nil == *Root(eval(*l)));

    Root e0, e1, val;
    e0 = create_int64(101);
    e1 = create_int64(102);
    val = list(fn_name, *e0, *e1);
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

}
}
