#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include <cleo/list.hpp>
#include <cleo/equality.hpp>
#include <gtest/gtest.h>
#include <array>

namespace cleo
{
namespace test
{

TEST(eval_test, should_eval_simple_values_to_themselves)
{
    auto fn = create_native_function([](const Value *, std::uint8_t) { return nil; });
    auto kw = create_keyword("org.xyz", "eqkw");
    auto i = create_int64(7);
    auto flt = create_float64(3.5);
    auto s = create_string("abcd");

    ASSERT_TRUE(nil == eval(nil));
    ASSERT_TRUE(fn == eval(fn));
    ASSERT_TRUE(kw == eval(kw));
    ASSERT_TRUE(i == eval(i));
    ASSERT_TRUE(flt = eval(flt));
    ASSERT_TRUE(s = eval(s));
}

TEST(eval_test, should_eval_objects_to_themselves)
{
    auto sym = create_symbol("cleo.eval.test", "obj");
    auto o = create_object(sym, nullptr, 0);
    ASSERT_TRUE(o == eval(o));
}

TEST(eval_test, should_eval_symbols_to_var_values)
{
    auto sym = create_symbol("cleo.eval.test", "seven");
    auto val = create_int64(7);
    define(sym, val);
    ASSERT_TRUE(val == eval(sym));
}

TEST(eval_test, should_fail_when_a_symbol_cannot_be_resolved)
{
    auto sym = create_symbol("cleo.eval.test", "missing");
    try
    {
        eval(sym);
        FAIL() << "expected an exception";
    }
    catch (const symbol_not_found& e)
    {
    }
}

template <typename... T>
Value list(T... elems)
{
    std::array<Value, sizeof...(T)> a{{elems...}};
    return create_list(a.data(), a.size());
}

TEST(eval_test, should_eval_lists_as_function_calls)
{
    auto fun_name = create_symbol("cleo.eval.test", "fun1");
    define(fun_name, create_native_function([](const Value *args, std::uint8_t num_args) { return num_args ? create_list(args, num_args) : nil; }));

    ASSERT_TRUE(nil == eval(list(fun_name)));

    auto args = eval(list(fun_name, create_int64(101), create_int64(102)));

    ASSERT_TRUE(type::LIST == get_value_type(args));
    ASSERT_EQ(2, get_int64_value(get_list_size(args)));
    ASSERT_EQ(101, get_int64_value(get_list_first(args)));
    ASSERT_EQ(102, get_int64_value(get_list_first(get_list_next(args))));
}

TEST(eval_test, should_fail_when_trying_to_call_a_non_function)
{
    try
    {
        eval(list(create_int64(10)));
        FAIL() << "expected an exception";
    }
    catch (const call_error& e)
    {
    }
}

TEST(eval_test, should_eval_function_arguments)
{
    auto fun_name = create_symbol("cleo.eval.test", "fun2");
    auto x1 = create_symbol("cleo.eval.test", "x1");
    auto x2 = create_symbol("cleo.eval.test", "x2");
    define(fun_name, create_native_function([](const Value *args, std::uint8_t num_args) { return create_list(args, num_args); }));
    define(x1, create_int64(101));
    define(x2, create_int64(102));

    auto args = eval(list(fun_name, x1, x2));

    ASSERT_TRUE(type::LIST == get_value_type(args));
    ASSERT_EQ(2, get_int64_value(get_list_size(args)));
    ASSERT_EQ(101, get_int64_value(get_list_first(args)));
    ASSERT_EQ(102, get_int64_value(get_list_first(get_list_next(args))));

}

}
}
