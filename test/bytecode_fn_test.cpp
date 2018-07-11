#include <cleo/bytecode_fn.hpp>
#include <cleo/eval.hpp>
#include <cleo/var.hpp>
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
    std::array<Int64, 1> arities{{2}};
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

TEST_F(bytecode_fn_test, DISABLED_should_fail_when_arity_cannot_be_matched)
{
}

TEST_F(bytecode_fn_test, DISABLED_should_dispatch_to_the_right_arity)
{
}

TEST_F(bytecode_fn_test, DISABLED_should_dispatch_to_vararg)
{
}

}
}
