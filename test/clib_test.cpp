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

std::int64_t CLEO_CDECL add3(
    std::int64_t a0, std::int64_t a1, std::int64_t a2)
{
    return a0 + a1 + a2;
}

std::int64_t CLEO_CDECL add4(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3)
{
    return a0 + a1 + a2 + a3;
}

std::int64_t CLEO_CDECL add5(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4)
{
    return a0 + a1 + a2 + a3 + a4;
}

std::int64_t CLEO_CDECL add6(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5)
{
    return a0 + a1 + a2 + a3 + a4 + a5;
}

std::int64_t CLEO_CDECL add7(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6;
}

std::int64_t CLEO_CDECL add8(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7;
}

std::int64_t CLEO_CDECL add9(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8;
}

std::int64_t CLEO_CDECL add10(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9;
}

std::int64_t CLEO_CDECL add11(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10;
}

std::int64_t CLEO_CDECL add12(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10, std::int64_t a11)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11;
}

std::int64_t CLEO_CDECL add13(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10, std::int64_t a11,
    std::int64_t a12)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11 + a12;
}

std::int64_t CLEO_CDECL add14(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10, std::int64_t a11,
    std::int64_t a12, std::int64_t a13)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11 + a12 + a13;
}

std::int64_t CLEO_CDECL add15(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10, std::int64_t a11,
    std::int64_t a12, std::int64_t a13, std::int64_t a14)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11 + a12 + a13 + a14;
}

std::int64_t CLEO_CDECL add16(
    std::int64_t a0, std::int64_t a1, std::int64_t a2, std::int64_t a3,
    std::int64_t a4, std::int64_t a5, std::int64_t a6, std::int64_t a7,
    std::int64_t a8, std::int64_t a9, std::int64_t a10, std::int64_t a11,
    std::int64_t a12, std::int64_t a13, std::int64_t a14, std::int64_t a15)
{
    return a0 + a1 + a2 + a3 + a4 + a5 + a6 + a7 + a8 + a9 + a10 + a11 + a12 + a13 + a14 + a15;
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

TEST_F(clib_test, should_create_c_function_with_3_params)
{
    auto name = create_symbol("gfn3");
    Root params{svec(
        clib::int64, clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add3, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7)};
    Root val{eval(*call)};
    Root ex{i64(15)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_4_params)
{
    auto name = create_symbol("gfn4");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add4, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11)};
    Root val{eval(*call)};
    Root ex{i64(26)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_5_params)
{
    auto name = create_symbol("gfn5");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64)};
    Root fn{create_c_fn((void *)add5, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13)};
    Root val{eval(*call)};
    Root ex{i64(39)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_6_params)
{
    auto name = create_symbol("gfn6");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add6, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17)};
    Root val{eval(*call)};
    Root ex{i64(56)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_7_params)
{
    auto name = create_symbol("gfn7");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add7, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19)};
    Root val{eval(*call)};
    Root ex{i64(75)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_8_params)
{
    auto name = create_symbol("gfn8");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add8, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19, 23)};
    Root val{eval(*call)};
    Root ex{i64(98)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_9_params)
{
    auto name = create_symbol("gfn9");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64)};
    Root fn{create_c_fn((void *)add9, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19, 23, 29)};
    Root val{eval(*call)};
    Root ex{i64(127)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_10_params)
{
    auto name = create_symbol("gfn10");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add10, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31)};
    Root val{eval(*call)};
    Root ex{i64(158)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_11_params)
{
    auto name = create_symbol("gfn11");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add11, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)};
    Root val{eval(*call)};
    Root ex{i64(195)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_12_params)
{
    auto name = create_symbol("gfn12");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add12, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41)};
    Root val{eval(*call)};
    Root ex{i64(236)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_13_params)
{
    auto name = create_symbol("gfn13");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64)};
    Root fn{create_c_fn((void *)add13, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43)};
    Root val{eval(*call)};
    Root ex{i64(279)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_14_params)
{
    auto name = create_symbol("gfn14");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add14, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47)};
    Root val{eval(*call)};
    Root ex{i64(326)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_15_params)
{
    auto name = create_symbol("gfn15");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add15, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53)};
    Root val{eval(*call)};
    Root ex{i64(379)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_create_c_function_with_16_params)
{
    auto name = create_symbol("gfn16");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64)};
    Root fn{create_c_fn((void *)add16, name, clib::int64, *params)};
    Root call{list(*fn, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59)};
    Root val{eval(*call)};
    Root ex{i64(438)};
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(clib_test, should_fail_to_create_c_functions_with_more_than_16_params)
{
    auto name = create_symbol("gfn16+");
    Root params{svec(
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64, clib::int64, clib::int64, clib::int64,
        clib::int64)};
    try
    {
        create_c_fn((void *)add16, name, clib::int64, *params);
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
