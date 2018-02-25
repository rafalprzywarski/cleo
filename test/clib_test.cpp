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
std::int64_t CLEO_CDECL ret13()
{
    return 13;
}

std::int64_t CLEO_CDECL inc7(std::int64_t x)
{
    return x + 7;
}

std::int64_t CLEO_CDECL add2(std::int64_t x, std::int64_t y)
{
    return x + y;
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
    auto name = create_symbol("gfn1");
    Root params{svec(clib::int64)};
    Root fn{create_c_fn((void *)inc7, name, clib::int64, *params)};
    Root call{list(*fn, 132)};
    Root val{eval(*call)};
    Root ex{i64(139)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_two_params)
{
    auto name = create_symbol("gfn2");
    Root params{svec(clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add2, name, clib::int64, *params)};
    Root call{list(*fn, 132, 71)};
    Root val{eval(*call)};
    Root ex{i64(203)};
    EXPECT_EQ_VALS(*ex, *val);
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
    Root params{svec(clib::int64)};
    Root fn{create_c_fn((void *)inc7, name, clib::int64, *params)};
    Root call{list(*fn, 3, 7)};
    expect_call_error(*call, "Wrong number of args (2) passed to: gfn1");
}

TEST_F(clib_test, two_param_function_should_check_arity)
{
    auto name = create_symbol("gfn2");
    Root params{svec(clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add2, name, clib::int64, *params)};
    Root call{list(*fn, 3)};
    expect_call_error(*call, "Wrong number of args (1) passed to: gfn2");
}

TEST_F(clib_test, one_param_function_should_fail_on_invalid_type)
{
    auto name = create_symbol("gfn1");
    Root params{svec(clib::int64)};
    Root fn{create_c_fn((void *)inc7, name, clib::int64, *params)};
    Root bad{create_string("bad")};
    Root call{list(*fn, *bad)};
    expect_call_error(*call, "Wrong arg 0 type: cleo.core/String");
}

TEST_F(clib_test, two_param_function_should_fail_on_invalid_type)
{
    auto name = create_symbol("gfn2");
    Root params{svec(clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add2, name, clib::int64, *params)};
    Root bad{create_string("bad")};
    Root call{list(*fn, *bad, 7)};
    expect_call_error(*call, "Wrong arg 0 type: cleo.core/String");
    call = list(*fn, 7, *bad);
    expect_call_error(*call, "Wrong arg 1 type: cleo.core/String");
}

}
}
