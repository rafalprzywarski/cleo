#include <cleo/bytecode_fn.hpp>
#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include <cleo/error.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct bytecode_fn_test : Test
{
    bytecode_fn_test() : Test("cleo.bytecode-fn.test") { }
};

TEST_F(bytecode_fn_test, should_eval_the_body)
{
    Root consts{array(2)};
    Root vars{array(get_var(PLUS))};
    std::array<vm::Byte, 11> bc{{
        vm::LDV, 0, 0,
        vm::LDC, 0, 0,
        vm::LDC, 0, 0,
        vm::CALL, 2}};
    Root body{create_bytecode_fn_body(*consts, *vars, 0, bc.data(), bc.size())};
    std::array<Value, 1> bodies{{*body}};
    std::array<Int64, 1> arities{{0}};
    Root fn{create_bytecode_fn(nil, arities.data(), bodies.data(), bodies.size())};
    Root call{fn_call(*fn)};
    Root ex{i64(4)};

    auto old_stack = stack;
    Root val{eval(*call)};

    EXPECT_EQ_VALS(*ex, *val);
    ASSERT_TRUE(old_stack == stack);
}

TEST_F(bytecode_fn_test, should_pass_the_arguments)
{
    std::array<vm::Byte, 11> bc{{
        vm::LDL, vm::Byte(-3), vm::Byte(-1),
        vm::LDL, vm::Byte(-2), vm::Byte(-1),
        vm::LDL, vm::Byte(-1), vm::Byte(-1),
        vm::CALL, 2}};
    Root body{create_bytecode_fn_body(nil, nil, 0, bc.data(), bc.size())};
    std::array<Value, 1> bodies{{*body}};
    std::array<Int64, 1> arities{{3}};
    Root fn{create_bytecode_fn(nil, arities.data(), bodies.data(), bodies.size())};
    Root call{fn_call(*fn, get_var_value(get_var(MINUS)), 5, 7)};
    Root ex{i64(-2)};

    auto old_stack = stack;
    Root val{eval(*call)};

    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ(old_stack.size(), stack.size());
    EXPECT_TRUE(old_stack == stack);
}

TEST_F(bytecode_fn_test, should_reserve_stack_space_for_local_variables)
{
    Root consts{array(10, 4, get_var_value(get_var(MINUS)))};
    std::array<vm::Byte, 29> bc{{
        vm::LDC, 2, 0,
        vm::STL, 0, 0,
        vm::LDC, 1, 0,
        vm::STL, 1, 0,
        vm::LDC, 0, 0,
        vm::STL, 2, 0,
        vm::LDL, 0, 0,
        vm::LDL, 1, 0,
        vm::LDL, 2, 0,
        vm::CALL, 2}};
    Root body{create_bytecode_fn_body(*consts, nil, 3, bc.data(), bc.size())};
    std::array<Value, 1> bodies{{*body}};
    std::array<Int64, 1> arities{{0}};
    Root fn{create_bytecode_fn(nil, arities.data(), bodies.data(), bodies.size())};
    Root call{fn_call(*fn)};
    Root ex{i64(-6)};

    auto old_stack = stack;
    Root val{eval(*call)};

    EXPECT_EQ_VALS(*ex, *val);
    ASSERT_TRUE(old_stack == stack);
}

TEST_F(bytecode_fn_test, should_fail_when_arity_cannot_be_matched)
{
    Root consts{array(10)};
    std::array<vm::Byte, 3> bc{{vm::LDC, 0, 0}};
    Root body{create_bytecode_fn_body(*consts, nil, 0, bc.data(), bc.size())};
    std::array<Value, 3> bodies{{*body, *body, *body}};
    std::array<Int64, 3> arities{{0, 1, 2}};
    auto name = create_symbol("fn012");
    Root fn{create_bytecode_fn(name, arities.data(), bodies.data(), bodies.size())};
    Root call{fn_call(*fn, nil, nil, nil)};

    try
    {
        eval(*call);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root e{catch_exception()};
        ASSERT_EQ_REFS(*type::CallError, get_value_type(*e));
    }

    fn = create_bytecode_fn(nil, nullptr, nullptr, 0);
    call = fn_call(*fn);
    ASSERT_THROW(eval(*call), Exception);
}

TEST_F(bytecode_fn_test, should_dispatch_to_the_right_arity)
{
    Root consts0{array(10)};
    Root consts1{array(nil, 11)};
    Root consts2{array(nil, nil, 12)};
    std::array<vm::Byte, 3> bc0{{vm::LDC, 0, 0}};
    std::array<vm::Byte, 3> bc1{{vm::LDC, 1, 0}};
    std::array<vm::Byte, 3> bc2{{vm::LDC, 2, 0}};
    Root body0{create_bytecode_fn_body(*consts0, nil, 0, bc0.data(), bc0.size())};
    Root body1{create_bytecode_fn_body(*consts1, nil, 1, bc1.data(), bc1.size())};
    Root body2{create_bytecode_fn_body(*consts2, nil, 2, bc2.data(), bc2.size())};
    std::array<Value, 3> bodies{{*body0, *body1, *body2}};
    std::array<Int64, 3> arities{{0, 1, 2}};
    Root fn{create_bytecode_fn(nil, arities.data(), bodies.data(), bodies.size())};
    Root call, ex, val;

    call = fn_call(*fn);
    ex = i64(10);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = fn_call(*fn, nil);
    ex = i64(11);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = fn_call(*fn, nil, nil);
    ex = i64(12);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(bytecode_fn_test, should_dispatch_to_vararg)
{
    Root consts0{array(10)};
    Root consts1{array(nil, 11)};
    Root consts2{array(nil, nil, 12)};
    std::array<vm::Byte, 3> bc0{{vm::LDC, 0, 0}};
    std::array<vm::Byte, 3> bc1{{vm::LDC, 1, 0}};
    std::array<vm::Byte, 3> bc2{{vm::LDC, 2, 0}};
    Root body0{create_bytecode_fn_body(*consts0, nil, 0, bc0.data(), bc0.size())};
    Root body1{create_bytecode_fn_body(*consts1, nil, 1, bc1.data(), bc1.size())};
    Root body2{create_bytecode_fn_body(*consts2, nil, 2, bc2.data(), bc2.size())};
    std::array<Value, 3> bodies{{*body0, *body1, *body2}};
    std::array<Int64, 3> arities{{0, 1, ~Int64(1)}};
    Root fn{create_bytecode_fn(nil, arities.data(), bodies.data(), bodies.size())};
    Root call, ex, val;

    call = fn_call(*fn);
    ex = i64(10);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = fn_call(*fn, nil);
    ex = i64(11);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = fn_call(*fn, nil, nil);
    ex = i64(12);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    arities[2] = ~Int64(3); // test passing the params!
    fn = create_bytecode_fn(nil, arities.data(), bodies.data(), bodies.size());

    call = fn_call(*fn, nil, nil, nil);
    ex = i64(12);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(bytecode_fn_test, should_pass_the_varargs_as_a_sequence_or_nil)
{
    auto create_fn2va = [](std::vector<vm::Byte> bc)
        {
            Root body{create_bytecode_fn_body(nil, nil, 0, bc.data(), bc.size())};
            std::array<Value, 1> bodies{{*body}};
            std::array<Int64, 1> arities{{~Int64(2)}};
            return create_bytecode_fn(nil, arities.data(), bodies.data(), bodies.size());
        };
    Root fn_a{create_fn2va({vm::LDL, vm::Byte(-3), vm::Byte(-1)})};
    Root fn_b{create_fn2va({vm::LDL, vm::Byte(-2), vm::Byte(-1)})};
    Root fn_c{create_fn2va({vm::LDL, vm::Byte(-1), vm::Byte(-1)})};
    Root call, ex, val;

    call = fn_call(*fn_a, 11, 12);
    ex = i64(11);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = fn_call(*fn_a, 11, 12, 13, 14);
    ex = i64(11);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = fn_call(*fn_b, 11, 12);
    ex = i64(12);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = fn_call(*fn_b, 11, 12, 13, 14);
    ex = i64(12);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = fn_call(*fn_c, 11, 12);
    val = eval(*call);
    EXPECT_EQ_VALS(nil, *val);

    call = fn_call(*fn_c, 11, 12, 13);
    ex = array(13);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = fn_call(*fn_c, 11, 12, 13, 14);
    ex = array(13, 14);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);
}

}
}