#include <cleo/clib.hpp>
#include <cleo/eval.hpp>
#include <cleo/error.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

namespace
{

template <typename T>
std::int64_t deref(T val)
{
    return val;
}

template <>
std::int64_t deref(const char *val)
{
    return *val;
}

template <typename T>
T neutral()
{
    return T();
}

template <>
const char *neutral()
{
    return "";
}

template <typename T>
T example();

template <>
Int64 example()
{
    return 0xf819283493ab6ef2ll;
}

template <>
const char *example()
{
    return "a";
}

template <typename T>
Value id();

template <>
Value id<Int64>()
{
    return clib::int64;
}

template <>
Value id<const char *>()
{
    return clib::string;
}

template <typename First>
std::int64_t add(First first)
{
    return deref(first);
}

template <typename First, typename Second, typename... Rest>
std::int64_t add(First first, Second second, Rest... rest)
{
    return deref(first) + add(second, rest...);
}

template <typename T>
Force example_list(int i)
{
    Root val{to_value(i == 0 ? example<T>() : neutral<T>())};
    return list_conj(*EMPTY_LIST, *val);
}

template <typename First, typename Second, typename... Rest>
Force example_list(int i)
{
    Root l{example_list<Second, Rest...>(i - 1)};
    Root val{to_value(i == 0 ? example<First>() : neutral<First>())};
    return list_conj(*l, *val);
}

template <typename T>
Force expected_value(int i)
{
    return to_value(deref(example<T>()));
}

template <typename First, typename Second, typename... Rest>
Force expected_value(int i)
{
    return i == 0 ? to_value(deref(example<First>())) : expected_value<Second, Rest...>(i - 1);
}

template <typename... Types>
void test_fn()
{
    auto name = create_symbol("gfn");
    Root params{array(id<Types>()...)};
    Root fn{create_c_fn((void *)add<Types...>, name, clib::int64, *params)};
    for (int i = 0; i < sizeof...(Types); ++i)
    {
        Root examples{example_list<Types...>(i)};
        Root call{list_conj(*examples, *fn)};
        Root val{eval(*call)};
        Root ex{expected_value<Types...>(i)};
        EXPECT_EQ_VALS(*ex, *val) << " param: " << i << " params: " << to_string(*params);
    }
}

std::int64_t CLEO_CDECL ret13()
{
    return 13;
}

std::int64_t CLEO_CDECL big17(
    std::int64_t, std::int64_t, std::int64_t, std::int64_t,
    std::int64_t, std::int64_t, std::int64_t, std::int64_t,
    std::int64_t, std::int64_t, std::int64_t, std::int64_t,
    std::int64_t, std::int64_t, std::int64_t, std::int64_t,
    std::int64_t)
{
    return 0;
}

}

struct clib_test : Test
{
    clib_test() : Test("cleo.clib.test") { }

    void expect_call_error(Value call, std::string msg)
    {
        try
        {
            Root val{eval(call)};
            FAIL() << "expected an exception, got " << to_string(*val);
        }
        catch (const Exception& )
        {
            Root e{catch_exception()};
            ASSERT_EQ_REFS(*type::CallError, get_value_type(*e));
            Root emsg{call_error_message(*e)};
            Root expected{create_string(msg)};
            ASSERT_EQ_VALS(*expected, *emsg);
        }
    }
};

TEST_F(clib_test, should_create_c_function_with_no_params)
{
    auto name = create_symbol("gfn0");
    Root fn{create_c_fn((void *)ret13, name, clib::int64, *EMPTY_VECTOR)};
    Root call{list(*fn)};
    Root val{eval(*call)};
    Root ex{i64(13)};
    EXPECT_EQ_VALS(*ex, *val);
    call = list(*fn, 99);
}

TEST_F(clib_test, should_create_c_function_with_one_param)
{
    test_fn<Int64>();
    test_fn<const char *>();
}

TEST_F(clib_test, should_create_c_function_with_two_params)
{
    test_fn<Int64, Int64>();
    test_fn<Int64, const char *>();
    test_fn<const char *, Int64>();
    test_fn<const char *, const char *>();
}

TEST_F(clib_test, should_create_c_function_with_3_params)
{
    test_fn<Int64, Int64, Int64>();
    test_fn<Int64, Int64, const char *>();
    test_fn<Int64, const char *, Int64>();
    test_fn<Int64, const char *, const char *>();
    test_fn<const char *, Int64, Int64>();
    test_fn<const char *, Int64, const char *>();
    test_fn<const char *, const char *, Int64>();
    test_fn<const char *, const char *, const char *>();
}

TEST_F(clib_test, should_create_c_function_with_4_params)
{
    test_fn<Int64, Int64, Int64, Int64>();
    test_fn<Int64, Int64, Int64, const char *>();
    test_fn<Int64, Int64, const char *, Int64>();
    test_fn<Int64, Int64, const char *, const char *>();
    test_fn<Int64, const char *, Int64, Int64>();
    test_fn<Int64, const char *, Int64, const char *>();
    test_fn<Int64, const char *, const char *, Int64>();
    test_fn<Int64, const char *, const char *, const char *>();
    test_fn<const char *, Int64, Int64, Int64>();
    test_fn<const char *, Int64, Int64, const char *>();
    test_fn<const char *, Int64, const char *, Int64>();
    test_fn<const char *, Int64, const char *, const char *>();
    test_fn<const char *, const char *, Int64, Int64>();
    test_fn<const char *, const char *, Int64, const char *>();
    test_fn<const char *, const char *, const char *, Int64>();
    test_fn<const char *, const char *, const char *, const char *>();
}

TEST_F(clib_test, should_create_c_function_with_5_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64>();
    test_fn<const char *, Int64, Int64, Int64, Int64>();
    test_fn<Int64, const char *, Int64, Int64, Int64>();
    test_fn<Int64, Int64, const char *, Int64, Int64>();
    test_fn<Int64, Int64, Int64, const char *, Int64>();
    test_fn<Int64, Int64, Int64, Int64, const char *>();
    test_fn<const char *, const char *, Int64, Int64, Int64>();
    test_fn<const char *, Int64, const char *, Int64, Int64>();
    test_fn<const char *, Int64, Int64, const char *, Int64>();
    test_fn<const char *, Int64, Int64, Int64, const char *>();
    test_fn<Int64, const char *, const char *, Int64, Int64>();
    test_fn<Int64, const char *, Int64, const char *, Int64>();
    test_fn<Int64, const char *, Int64, Int64, const char *>();
    test_fn<Int64, Int64, const char *, const char *, Int64>();
    test_fn<Int64, Int64, const char *, Int64, const char *>();
    test_fn<Int64, Int64, Int64, const char *, const char *>();

    test_fn<const char *, const char *, const char *, const char *, const char *>();
    test_fn<Int64, const char *, const char *, const char *, const char *>();
    test_fn<const char *, Int64, const char *, const char *, const char *>();
    test_fn<const char *, const char *, Int64, const char *, const char *>();
    test_fn<const char *, const char *, const char *, Int64, const char *>();
    test_fn<const char *, const char *, const char *, const char *, Int64>();
    test_fn<Int64, Int64, const char *, const char *, const char *>();
    test_fn<Int64, const char *, Int64, const char *, const char *>();
    test_fn<Int64, const char *, const char *, Int64, const char *>();
    test_fn<Int64, const char *, const char *, const char *, Int64>();
    test_fn<const char *, Int64, Int64, const char *, const char *>();
    test_fn<const char *, Int64, const char *, Int64, const char *>();
    test_fn<const char *, Int64, const char *, const char *, Int64>();
    test_fn<const char *, const char *, Int64, Int64, const char *>();
    test_fn<const char *, const char *, Int64, const char *, Int64>();
    test_fn<const char *, const char *, const char *, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_6_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_7_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_8_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_9_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_10_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_11_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_12_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_13_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_14_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_15_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
}

TEST_F(clib_test, should_create_c_function_with_16_params)
{
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
    test_fn<const char *, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64>();
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, const char *>();
    test_fn<Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, Int64, const char *, const char *>();
    test_fn<const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *>();
    test_fn<Int64, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *>();
    test_fn<const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, Int64>();
    test_fn<const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, const char *, Int64, Int64>();
}

TEST_F(clib_test, should_fail_to_create_c_functions_with_more_than_16_params)
{
    auto name = create_symbol("gfn16+");
    Root params{array(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64)};
    try
    {
        create_c_fn((void *)big17, name, clib::int64, *params);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::IllegalArgument, get_value_type(*e));
    }
}

TEST_F(clib_test, no_param_function_should_check_arity)
{
    auto name = create_symbol("gfn0");
    Root fn{create_c_fn((void *)ret13, name, clib::int64, *EMPTY_VECTOR)};
    Root call{list(*fn, 3)};
    expect_call_error(*call, "Wrong number of args (1) passed to: gfn0");
}

TEST_F(clib_test, one_param_function_should_check_arity)
{
    auto name = create_symbol("gfn1");
    Root params{array(clib::int64)};
    Root fn{create_c_fn((void *)add<Int64>, name, clib::int64, *params)};
    Root call{list(*fn, 3, 7)};
    expect_call_error(*call, "Wrong number of args (2) passed to: gfn1");
}

TEST_F(clib_test, two_param_function_should_check_arity)
{
    auto name = create_symbol("gfn2");
    Root params{array(clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add<Int64, Int64>, name, clib::int64, *params)};
    Root call{list(*fn, 3)};
    expect_call_error(*call, "Wrong number of args (1) passed to: gfn2");
}

TEST_F(clib_test, should_fail_on_invalid_declared_type)
{
    auto name = create_symbol("gfn1");
    Root params{array(create_keyword("bad"))};
    try
    {
        create_c_fn((void *)add<Int64>, name, clib::int64, *params);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::IllegalArgument, get_value_type(*e));
        Root emsg{illegal_argument_message(*e)};
        Root expected{create_string("Invalid parameter types: [:bad]")};
        ASSERT_EQ_VALS(*expected, *emsg);
    }
}

TEST_F(clib_test, one_int64_param_function_should_fail_on_invalid_type)
{
    auto name = create_symbol("gfn1");
    Root params{array(clib::int64)};
    Root fn{create_c_fn((void *)add<Int64>, name, clib::int64, *params)};
    Root bad{create_string("bad")};
    Root call{list(*fn, *bad)};
    expect_call_error(*call, "Wrong arg 0 type: cleo.core/String");
}

TEST_F(clib_test, one_string_param_function_should_fail_on_invalid_type)
{
    auto name = create_symbol("gfn1");
    Root params{array(clib::string)};
    Root fn{create_c_fn((void *)add<const char *>, name, clib::string, *params)};
    Root bad{create_keyword("bad")};
    Root call{list(*fn, *bad)};
    expect_call_error(*call, "Wrong arg 0 type: cleo.core/Keyword");
}

TEST_F(clib_test, two_param_function_should_fail_on_invalid_type)
{
    auto name = create_symbol("gfn2");
    Root params{array(clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add<Int64, Int64>, name, clib::int64, *params)};
    Root bad{create_string("bad")};
    Root call{list(*fn, *bad, 7)};
    expect_call_error(*call, "Wrong arg 0 type: cleo.core/String");
    call = list(*fn, 7, *bad);
    expect_call_error(*call, "Wrong arg 1 type: cleo.core/String");
}

}
}
