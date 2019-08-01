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
    bytecode_fn_test() : Test("cleo.bytecode-fn.test")
    {
        stack_push(FN);
        stack_push(CONJ);
    }
};

TEST_F(bytecode_fn_test, should_eval_the_body)
{
    Root x{new_index_out_of_bounds()};
    Root consts{array(2, *x)};
    Root vars{array(get_var(INTERNAL_ADD_2))};
    std::array<Int64, 4> entries{{0, 4, 4, 0}};
    std::array<Value, 1> types{{*type::IndexOutOfBounds}};
    Root et{create_bytecode_fn_exception_table(entries.data(), types.data(), types.size())};
    std::array<vm::Byte, 16> bc{{
        vm::LDC, 1, 0,
        vm::THROW,
        vm::POP,
        vm::LDV, 0, 0,
        vm::LDC, 0, 0,
        vm::LDC, 0, 0,
        vm::CALL, 2}};
    Root body{create_bytecode_fn_body(0, *consts, *vars, *et, 0, bc.data(), bc.size())};
    std::array<Value, 1> bodies{{*body}};
    Root fn{create_bytecode_fn(nil, bodies.data(), bodies.size())};
    Root call{list(*fn)};
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
    Root body{create_bytecode_fn_body(3, nil, nil, nil, 0, bc.data(), bc.size())};
    std::array<Value, 1> bodies{{*body}};
    Root fn{create_bytecode_fn(nil, bodies.data(), bodies.size())};
    Root call{list(*fn, get_var_value(get_var(MINUS)), 5, 7)};
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
    Root body{create_bytecode_fn_body(0, *consts, nil, nil, 3, bc.data(), bc.size())};
    std::array<Value, 1> bodies{{*body}};
    Root fn{create_bytecode_fn(nil, bodies.data(), bodies.size())};
    Root call{list(*fn)};
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
    Root body0{create_bytecode_fn_body(0, *consts, nil, nil, 0, bc.data(), bc.size())};
    Root body1{create_bytecode_fn_body(1, *consts, nil, nil, 0, bc.data(), bc.size())};
    Root body2{create_bytecode_fn_body(2, *consts, nil, nil, 0, bc.data(), bc.size())};
    std::array<Value, 3> bodies{{*body0, *body1, *body2}};
    auto name = create_symbol("fn012");
    Root fn{create_bytecode_fn(name, bodies.data(), bodies.size())};
    Root call{list(*fn, nil, nil, nil)};

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

    fn = create_bytecode_fn(nil, nullptr, 0);
    call = list(*fn);
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
    Root body0{create_bytecode_fn_body(0, *consts0, nil, nil, 0, bc0.data(), bc0.size())};
    Root body1{create_bytecode_fn_body(1, *consts1, nil, nil, 1, bc1.data(), bc1.size())};
    Root body2{create_bytecode_fn_body(2, *consts2, nil, nil, 2, bc2.data(), bc2.size())};
    std::array<Value, 3> bodies{{*body0, *body1, *body2}};
    Root fn{create_bytecode_fn(nil, bodies.data(), bodies.size())};
    Root call, ex, val;

    call = list(*fn);
    ex = i64(10);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn, nil);
    ex = i64(11);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn, nil, nil);
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
    Root body0{create_bytecode_fn_body(0, *consts0, nil, nil, 0, bc0.data(), bc0.size())};
    Root body1{create_bytecode_fn_body(1, *consts1, nil, nil, 1, bc1.data(), bc1.size())};
    Root body2{create_bytecode_fn_body(~Int64(1), *consts2, nil, nil, 2, bc2.data(), bc2.size())};
    std::array<Value, 3> bodies{{*body0, *body1, *body2}};
    Root fn{create_bytecode_fn(nil, bodies.data(), bodies.size())};
    Root call, ex, val;

    call = list(*fn);
    ex = i64(10);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn, nil);
    ex = i64(11);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn, nil, nil);
    ex = i64(12);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    body2 = create_bytecode_fn_body(~Int64(3), *consts2, nil, nil, 2, bc2.data(), bc2.size()); // test passing the params!
    bodies[2] = *body2;
    fn = create_bytecode_fn(nil, bodies.data(), bodies.size());

    call = list(*fn, nil, nil, nil);
    ex = i64(12);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(bytecode_fn_test, should_pass_the_varargs_as_a_sequence_or_nil)
{
    auto create_fn2va = [](std::vector<vm::Byte> bc)
        {
            Root body{create_bytecode_fn_body(~Int64(2), nil, nil, nil, 0, bc.data(), bc.size())};
            std::array<Value, 1> bodies{{*body}};
            return create_bytecode_fn(nil, bodies.data(), bodies.size());
        };
    Root fn_a{create_fn2va({vm::LDL, vm::Byte(-3), vm::Byte(-1)})};
    Root fn_b{create_fn2va({vm::LDL, vm::Byte(-2), vm::Byte(-1)})};
    Root fn_c{create_fn2va({vm::LDL, vm::Byte(-1), vm::Byte(-1)})};
    Root call, ex, val;

    call = list(*fn_a, 11, 12);
    ex = i64(11);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn_a, 11, 12, 13, 14);
    ex = i64(11);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn_b, 11, 12);
    ex = i64(12);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn_b, 11, 12, 13, 14);
    ex = i64(12);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn_c, 11, 12);
    val = eval(*call);
    EXPECT_EQ_VALS(nil, *val);

    call = list(*fn_c, 11, 12, 13);
    ex = array(13);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);

    call = list(*fn_c, 11, 12, 13, 14);
    ex = array(13, 14);
    val = eval(*call);
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(bytecode_fn_test, should_restore_stack_when_an_exception_is_thrown)
{
    Root ex{new_index_out_of_bounds()};
    Root consts{array(*ex)};
    std::array<vm::Byte, 4> bc{{vm::LDC, 0, 0, vm::THROW}};
    Root body{create_bytecode_fn_body(0, *consts, nil, nil, 10, bc.data(), bc.size())};
    std::array<Value, 1> bodies{{*body}};
    Root fn{create_bytecode_fn(nil, bodies.data(), bodies.size())};
    Root call{list(*fn)};

    auto old_stack = stack;
    try
    {
        eval(*call);
        FAIL() << "expected an exception";
    }
    catch (Exception const & )
    {
        Root actual{catch_exception()};
        EXPECT_EQ_REFS(*ex, *actual);
        EXPECT_TRUE(old_stack == stack);
    }
}

TEST_F(bytecode_fn_test, should_replace_last_n_constants_in_all_bodies)
{
    auto name = create_symbol("abc");
    Root fn{create_bytecode_fn(name, nullptr, 0)};
    Root mfn{bytecode_fn_replace_consts(*fn, nullptr, 0)};
    EXPECT_EQ_REFS(*fn, *mfn);

    Root consts1{array(10, 20, 30, 40, 50, 60)};
    Root consts2{array(50, 60, 70, 80, 90, 100)};
    Root vars1{array()};
    Root vars2{array()};
    std::array<Int64, 4> et_entries{{1, 2, 7, 0}};
    std::array<Value, 1> et_types{{*type::IndexOutOfBounds}};
    Root et1{create_bytecode_fn_exception_table(et_entries.data(), et_types.data(), et_types.size())};
    Root et2{create_bytecode_fn_exception_table(et_entries.data(), et_types.data(), et_types.size())};
    Int64 locals_size1 = 7;
    Int64 locals_size2 = 9;
    std::array<vm::Byte, 1> bytes1{{vm::CNIL}};
    std::array<vm::Byte, 3> bytes2{{vm::CNIL, vm::CNIL, vm::POP}};
    Roots rbodies(2);
    rbodies.set(0, create_bytecode_fn_body(2, *consts1, *vars1, *et1, locals_size1, bytes1.data(), bytes1.size()));
    rbodies.set(1, create_bytecode_fn_body(3, *consts2, *vars2, *et2, locals_size2, bytes2.data(), bytes2.size()));
    std::array<Value, 2> bodies{{rbodies[0], rbodies[1]}};

    fn = create_bytecode_fn(name, bodies.data(), bodies.size());
    Root aconsts{array(17, 19)};
    std::array<Value, 2> nconsts{{get_array_elem(*aconsts, 0), get_array_elem(*aconsts, 1)}};
    mfn = bytecode_fn_replace_consts(*fn, nconsts.data(), nconsts.size());

    Root mconsts1{array(10, 20, 30, 40, 17, 19)};
    Root mconsts2{array(50, 60, 70, 80, 17, 19)};
    EXPECT_EQ_REFS(*type::BytecodeFn, get_value_type(*mfn));
    EXPECT_EQ(2, get_bytecode_fn_size(*mfn));
    EXPECT_EQ_VALS(name, get_bytecode_fn_name(*mfn));
    EXPECT_EQ(2, get_bytecode_fn_arity(*mfn, 0));
    EXPECT_EQ(3, get_bytecode_fn_arity(*mfn, 1));
    EXPECT_EQ(2, get_bytecode_fn_body_arity(get_bytecode_fn_body(*mfn, 0)));
    EXPECT_EQ(3, get_bytecode_fn_body_arity(get_bytecode_fn_body(*mfn, 1)));
    EXPECT_EQ_VALS(*mconsts1, get_bytecode_fn_body_consts(get_bytecode_fn_body(*mfn, 0)));
    EXPECT_EQ_VALS(*mconsts2, get_bytecode_fn_body_consts(get_bytecode_fn_body(*mfn, 1)));
    EXPECT_EQ_REFS(*vars1, get_bytecode_fn_body_vars(get_bytecode_fn_body(*mfn, 0)));
    EXPECT_EQ_REFS(*vars2, get_bytecode_fn_body_vars(get_bytecode_fn_body(*mfn, 1)));
    EXPECT_EQ_REFS(*et1, get_bytecode_fn_body_exception_table(get_bytecode_fn_body(*mfn, 0)));
    EXPECT_EQ_REFS(*et2, get_bytecode_fn_body_exception_table(get_bytecode_fn_body(*mfn, 1)));
    EXPECT_EQ(locals_size1, get_bytecode_fn_body_locals_size(get_bytecode_fn_body(*mfn, 0)));
    EXPECT_EQ(locals_size2, get_bytecode_fn_body_locals_size(get_bytecode_fn_body(*mfn, 1)));
    EXPECT_EQ(Int64(bytes1.size()), get_bytecode_fn_body_bytes_size(get_bytecode_fn_body(*mfn, 0)));
    EXPECT_EQ(Int64(bytes2.size()), get_bytecode_fn_body_bytes_size(get_bytecode_fn_body(*mfn, 1)));
    EXPECT_TRUE(std::equal(begin(bytes1), end(bytes1), get_bytecode_fn_body_bytes(get_bytecode_fn_body(*mfn, 0))));
    EXPECT_TRUE(std::equal(begin(bytes2), end(bytes2), get_bytecode_fn_body_bytes(get_bytecode_fn_body(*mfn, 1))));
}

TEST_F(bytecode_fn_test, should_find_exception_handlers_based_on_range_and_type_in_order_order_of_declaration)
{
    Root et{create_bytecode_fn_exception_table(nullptr, nullptr, 0)};
    EXPECT_EQ(-1, bytecode_fn_find_exception_handler(*et, 0, *type::IllegalArgument).offset);

    std::array<Int64, 4> entries1{{3, 5, 17, 7}};
    std::array<Value, 1> types1{{nil}};
    et = create_bytecode_fn_exception_table(entries1.data(), types1.data(), types1.size());
    EXPECT_EQ(-1, bytecode_fn_find_exception_handler(*et, 2, *type::IllegalArgument).offset);
    EXPECT_EQ(17, bytecode_fn_find_exception_handler(*et, 3, *type::IllegalArgument).offset);
    EXPECT_EQ(7, bytecode_fn_find_exception_handler(*et, 3, *type::IllegalArgument).stack_size);
    EXPECT_EQ(17, bytecode_fn_find_exception_handler(*et, 4, *type::IllegalArgument).offset);
    EXPECT_EQ(-1, bytecode_fn_find_exception_handler(*et, 5, *type::IllegalArgument).offset);
    EXPECT_EQ(-1, bytecode_fn_find_exception_handler(*et, 6, *type::IllegalArgument).offset);

    std::array<Int64, 8> entries2{{3, 6, 11, 7, 4, 8, 22, 9}};
    std::array<Value, 2> types2{{nil, nil}};
    et = create_bytecode_fn_exception_table(entries2.data(), types2.data(), types2.size());
    EXPECT_EQ(-1, bytecode_fn_find_exception_handler(*et, 2, *type::IllegalArgument).offset);
    EXPECT_EQ(11, bytecode_fn_find_exception_handler(*et, 3, *type::IllegalArgument).offset);
    EXPECT_EQ(7, bytecode_fn_find_exception_handler(*et, 3, *type::IllegalArgument).stack_size);
    EXPECT_EQ(11, bytecode_fn_find_exception_handler(*et, 4, *type::IllegalArgument).offset);
    EXPECT_EQ(11, bytecode_fn_find_exception_handler(*et, 5, *type::IllegalArgument).offset);
    EXPECT_EQ(22, bytecode_fn_find_exception_handler(*et, 6, *type::IllegalArgument).offset);
    EXPECT_EQ(9, bytecode_fn_find_exception_handler(*et, 6, *type::IllegalArgument).stack_size);
    EXPECT_EQ(22, bytecode_fn_find_exception_handler(*et, 7, *type::IllegalArgument).offset);
    EXPECT_EQ(-1, bytecode_fn_find_exception_handler(*et, 8, *type::IllegalArgument).offset);
    EXPECT_EQ(-1, bytecode_fn_find_exception_handler(*et, 9, *type::IllegalArgument).offset);

    std::array<Int64, 8> entries3{{5, 7, 11, 7, 3, 9, 22, 9}};
    std::array<Value, 2> types3{{*type::IndexOutOfBounds, *type::IllegalArgument}};
    et = create_bytecode_fn_exception_table(entries3.data(), types3.data(), types3.size());
    EXPECT_EQ(22, bytecode_fn_find_exception_handler(*et, 3, *type::IllegalArgument).offset);
    EXPECT_EQ(9, bytecode_fn_find_exception_handler(*et, 3, *type::IllegalArgument).stack_size);
    EXPECT_EQ(22, bytecode_fn_find_exception_handler(*et, 5, *type::IllegalArgument).offset);

    std::array<Int64, 8> entries4{{5, 7, 55, 33}};
    std::array<Value, 2> types4{{*type::Exception}};
    et = create_bytecode_fn_exception_table(entries4.data(), types4.data(), types4.size());
    EXPECT_EQ(55, bytecode_fn_find_exception_handler(*et, 5, *type::IllegalArgument).offset);
    EXPECT_EQ(33, bytecode_fn_find_exception_handler(*et, 5, *type::IllegalArgument).stack_size);
}

}
}
