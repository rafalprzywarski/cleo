#include <cleo/vm.hpp>
#include <cleo/bytecode_fn.hpp>
#include <cleo/error.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace vm
{
namespace test
{
using namespace cleo::test;

struct vm_test : Test
{
    vm_test() : Test("cleo.vm.test") { }

    template <std::size_t N>
    void eval_bytecode(Value constants, Value vars, std::uint32_t locals_size, const std::array<Byte, N>& bc)
    {
        vm::eval_bytecode(stack, constants, vars, locals_size, nil, bc.data(), bc.size());
    }

    template <std::size_t N, std::size_t K>
    void eval_bytecode(Value constants, Value vars, std::uint32_t locals_size, const std::array<Int64, K * 3>& et_entries, const std::array<Value, K>& et_types, const std::array<Byte, N>& bc)
    {
        Root etv{create_bytecode_fn_exception_table(et_entries.data(), et_types.data(), et_types.size())};
        vm::eval_bytecode(stack, constants, vars, locals_size, *etv, bc.data(), bc.size());
    }
    static Force create_constants(std::vector<std::pair<std::uint32_t, Int64>> vals)
    {
        std::uint32_t size = 65536;
        Roots rs{size};
        std::vector<Value> vs(size);
        for (auto& v : vals)
        {
            rs.set(v.first, i64(v.second));
            vs[v.first] = rs[v.first];
        }
        return create_array(vs.data(), vs.size());
    }

    static Force create_vars(std::vector<std::pair<std::uint32_t, Value>> vals)
    {
        std::uint32_t size = 65536;
        std::vector<Value> vs(size);
        for (auto& v : vals)
            vs[v.first] = v.second;
        return create_array(vs.data(), vs.size());
    }
};

TEST_F(vm_test, ldc)
{
    Root constants{create_constants({
      {0, 13},
      {1, 17},
      {2, 19},
      {128, 301},
      {255, 302},
      {256, 303},
      {32767, 304},
      {65535, 305}})};

    const std::array<Byte, 3> bc1{{LDC, Byte(-1), 0}};
    eval_bytecode(*constants, nil, 0, bc1);

    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 255), stack[0]);

    const std::array<Byte, 21> bc2{{
        LDC, 0, 0,
        LDC, 1, 0,
        LDC, 2, 0,
        LDC, Byte(-128), 0,
        LDC, 0, 1,
        LDC, Byte(-1), 127,
        LDC, Byte(-1), Byte(-1)}};
    eval_bytecode(*constants, nil, 0, bc2);

    ASSERT_EQ(8u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 65535), stack[7]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 32767), stack[6]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 256), stack[5]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 128), stack[4]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 2), stack[3]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 1), stack[2]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 0), stack[1]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 255), stack[0]);
}

TEST_F(vm_test, ldl)
{
    Root x{i64(17)};
    stack_push(*x);
    const std::array<Byte, 3> bc1{{LDL, 0, 0}};
    eval_bytecode(nil, nil, 1, bc1);

    ASSERT_EQ(2u, stack.size());
    EXPECT_EQ_VALS(*x, stack[1]);
    EXPECT_EQ_VALS(*x, stack[0]);

    Root y{i64(35)}, a{i64(7)}, b{i64(8)};
    stack.clear();
    stack_push(*b);
    stack_push(*a);
    stack.resize(stack.size() + 1024, *x);
    stack_push(*y);
    const std::array<Byte, 12> bc2{{
        LDL, Byte(-1), 3,
        LDL, 0, 4,
        LDL, Byte(-1), Byte(-1),
        LDL, Byte(-2), Byte(-1)}};
    eval_bytecode(nil, nil, 1025, bc2);

    ASSERT_EQ(1031u, stack.size());
    EXPECT_EQ_VALS(*b, stack[1030]);
    EXPECT_EQ_VALS(*a, stack[1029]);
    EXPECT_EQ_VALS(*y, stack[1028]);
    EXPECT_EQ_VALS(*x, stack[1027]);
    EXPECT_EQ_VALS(*y, stack[1026]);
    EXPECT_EQ_VALS(*x, stack[1025]);
    EXPECT_EQ_VALS(*x, stack[2]);
    EXPECT_EQ_VALS(*a, stack[1]);
    EXPECT_EQ_VALS(*b, stack[0]);
}

TEST_F(vm_test, stl)
{
    Root x{i64(17)};
    stack_push(*x);
    stack_push(nil);
    const std::array<Byte, 6> bc1{{LDL, Byte(-1), Byte(-1), STL, 0, 0}};
    eval_bytecode(nil, nil, 1, bc1);

    ASSERT_EQ(2u, stack.size());
    EXPECT_EQ_VALS(*x, stack[1]);
    EXPECT_EQ_VALS(*x, stack[0]);

    Root y{i64(35)}, a{i64(7)}, b{i64(8)};
    stack.clear();
    stack_push(*b);
    stack_push(*a);
    stack.resize(stack.size() + 1024, *x);
    stack_push(*y);
    const std::array<Byte, 30> bc2{{
        LDL, Byte(-1), Byte(-1), STL, 0, 0,
        LDL, Byte(-2), Byte(-1), STL, 1, 0,
        LDL, 0, 4, STL, Byte(-1), Byte(-1),
        LDL, Byte(-1), 3, STL, Byte(-2), Byte(-1),
        LDL, 0, 0, STL, 0, 4}};
    eval_bytecode(nil, nil, 1025, bc2);

    ASSERT_EQ(1027u, stack.size());
    EXPECT_EQ_VALS(*a, stack[1026]);
    EXPECT_EQ_VALS(*x, stack[1025]);
    EXPECT_EQ_VALS(*x, stack[4]);
    EXPECT_EQ_VALS(*b, stack[3]);
    EXPECT_EQ_VALS(*a, stack[2]);
    EXPECT_EQ_VALS(*y, stack[1]);
    EXPECT_EQ_VALS(*x, stack[0]);
}

TEST_F(vm_test, setv)
{
    in_ns(create_symbol("vm.setv.test"));
    auto var = define(create_symbol("vm.setv.test", "a"), *THREE);
    Root meta{phmap(10, 20)};
    stack_push(var);
    stack_push(*TWO);
    stack_push(*meta);
    const std::array<Byte, 10> bc1{{LDL, -3, -1, LDL, -2, -1, LDL, -1, -1, SETV}};
    eval_bytecode(nil, nil, 0, bc1);

    ASSERT_EQ(4u, stack.size());
    EXPECT_EQ_VALS(var, stack[3]);
    EXPECT_EQ_VALS(*meta, stack[2]);
    EXPECT_EQ_VALS(*TWO, stack[1]);
    EXPECT_EQ_VALS(var, stack[0]);
    EXPECT_EQ_VALS(*TWO, get_var_root_value(var));
    EXPECT_EQ_VALS(*meta, get_var_meta(var));
}

TEST_F(vm_test, ldv)
{
    in_ns(create_symbol("vm.ldv.test"));
    auto v1 = define(create_symbol("vm.ldv.test", "a"), *THREE);
    auto v2 = define(create_symbol("vm.ldv.test", "b"), *TWO);
    auto v3 = define(create_symbol("vm.ldv.test", "c"), *ZERO);
    auto v4 = define(create_symbol("vm.ldv.test", "d"), *ONE);
    Root vars{create_vars({{255, v1}, {0, v2}, {1, v3}, {65535, v4}})};

    const std::array<Byte, 3> bc1{{LDV, Byte(-1), 0}};
    eval_bytecode(nil, *vars, 0, bc1);

    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(get_var_value(v1), stack[0]);

    const std::array<Byte, 9> bc2{{
        LDV, 0, 0,
        LDV, 1, 0,
        LDV, Byte(-1), Byte(-1)}};
    eval_bytecode(nil, *vars, 0, bc2);

    ASSERT_EQ(4u, stack.size());
    EXPECT_EQ_VALS(get_var_value(v4), stack[3]);
    EXPECT_EQ_VALS(get_var_value(v3), stack[2]);
    EXPECT_EQ_VALS(get_var_value(v2), stack[1]);
    EXPECT_EQ_VALS(get_var_value(v1), stack[0]);
}

TEST_F(vm_test, br)
{
    Root constants{create_constants({{0, 10}, {1, 20}})};
    const std::array<Byte, 6> bc1{{BR, 0, 0, LDC, 0, 0}};

    eval_bytecode(*constants, nil, 0, bc1);
    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 0), stack[0]);
    stack.clear();

    const std::array<Byte, 9> bc2{{BR, 3, 0, LDC, 0, 0, LDC, 1, 0}};
    eval_bytecode(*constants, nil, 0, bc2);

    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 1), stack[0]);
    stack.clear();

    const std::array<Byte, 15> bc3{{BR, 3, 0, BR, 6, 0, LDC, 0, 0, BR, Byte(-9), Byte(-1), LDC, 1, 0}};
    eval_bytecode(*constants, nil, 0, bc3);

    ASSERT_EQ(2u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 1), stack[1]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 0), stack[0]);
    stack.clear();

    const std::array<Byte, 264> bc4{{
        BR, 2, 1,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 1, 0}};
    eval_bytecode(*constants, nil, 0, bc4);

    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 1), stack[0]);
}

TEST_F(vm_test, bnil)
{
    Root constants{create_constants({{0, 10}, {1, 20}})};
    const std::array<Byte, 9> bc1{{BNIL, 3, 0, LDC, 0, 0, LDC, 1, 0}};

    stack_push(nil);
    stack_push(i64(7));
    eval_bytecode(*constants, nil, 0, bc1);

    ASSERT_EQ(3u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 1), stack[2]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 0), stack[1]);
    EXPECT_EQ_VALS(nil, stack[0]);
    stack.clear();

    stack_push(nil);
    eval_bytecode(*constants, nil, 0, bc1);
    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 1), stack[0]);
}

TEST_F(vm_test, bnnil)
{
    Root constants{create_constants({{0, 10}, {1, 20}})};
    const std::array<Byte, 9> bc1{{BNNIL, 3, 0, LDC, 0, 0, LDC, 1, 0}};

    stack_push(i64(7));
    stack_push(nil);
    eval_bytecode(*constants, nil, 0, bc1);

    ASSERT_EQ(3u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 1), stack[2]);
    EXPECT_EQ_VALS(get_array_elem(*constants, 0), stack[1]);
    stack.clear();

    stack_push(i64(7));
    eval_bytecode(*constants, nil, 0, bc1);
    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(get_array_elem(*constants, 1), stack[0]);
}

TEST_F(vm_test, pop)
{
    stack_push(*THREE);
    stack_push(*ONE);
    stack_push(*TWO);

    const std::array<Byte, 1> bc1{{POP}};
    eval_bytecode(nil, nil, 0, bc1);
    ASSERT_EQ(2u, stack.size());
    EXPECT_EQ_VALS(*ONE, stack[1]);
    EXPECT_EQ_VALS(*THREE, stack[0]);

    const std::array<Byte, 2> bc2{{POP, POP}};
    eval_bytecode(nil, nil, 0, bc2);
    ASSERT_TRUE(stack.empty());
}

TEST_F(vm_test, call)
{
    Root x{i64(7)};
    Root constants{array(arrayv(*x, 8), *rt::first)};
    const std::array<Byte, 8> bc1{{LDC, 1, 0, LDC, 0, 0, CALL, 1}};
    eval_bytecode(*constants, nil, 0, bc1);

    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(*x, stack[0]);
    stack.clear();

    constants = array(phmapv(10, 20), 30, 40, *rt::assoc);
    const std::array<Byte, 14> bc2{{LDC, 3, 0, LDC, 0, 0, LDC, 1, 0, LDC, 2, 0, CALL, 3}};
    stack_push(*x);
    eval_bytecode(*constants, nil, 0, bc2);

    Root ex{phmap(10, 20, 30, 40)};
    ASSERT_EQ(2u, stack.size());
    EXPECT_EQ_VALS(*ex, stack[1]);
    EXPECT_EQ_VALS(*x, stack[0]);
    stack.clear();

    Root count_args{create_native_function([](const Value *, std::uint8_t n) { return i64(n); })};
    constants = array(*x, *count_args);
    const std::array<Byte, 392> bc3{{
        LDC, 1, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        CALL, Byte(-127),
    }};
    eval_bytecode(*constants, nil, 0, bc3);

    ex = i64(129);
    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(*ex, stack[0]);
}

TEST_F(vm_test, apply)
{
    Root x{i64(7)};
    Root constants{array(arrayv(*x, 8), *rt::first)};
    const std::array<Byte, 9> bc1{{LDC, 1, 0, LDC, 0, 0, CNIL, APPLY, 1}};
    eval_bytecode(*constants, nil, 0, bc1);

    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(*x, stack[0]);
    stack.clear();

    constants = array(phmapv(10, 20), 30, 40, *rt::assoc);
    const std::array<Byte, 15> bc2{{LDC, 3, 0, LDC, 0, 0, LDC, 1, 0, LDC, 2, 0, CNIL, APPLY, 3}};
    stack_push(*x);
    eval_bytecode(*constants, nil, 0, bc2);

    Root ex{phmap(10, 20, 30, 40)};
    ASSERT_EQ(2u, stack.size());
    EXPECT_EQ_VALS(*ex, stack[1]);
    EXPECT_EQ_VALS(*x, stack[0]);
    stack.clear();

    Root count_args{create_native_function([](const Value *, std::uint8_t n) { return i64(n); })};
    constants = array(*x, *count_args);
    const std::array<Byte, 393> bc3{{
        LDC, 1, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        CNIL,
        APPLY, Byte(-127),
    }};
    eval_bytecode(*constants, nil, 0, bc3);
    ex = i64(129);
    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(*ex, stack[0]);
    stack.clear();

    Root pick_args{create_native_function([](const Value *args, std::uint8_t n) { return array(n, args[0], args[128], args[129], args[128 + 50]); })};
    constants = array(*x,
                      arrayv(1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                             11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
                             21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
                             31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
                             41, 42, 43, 44, 45, 46, 47, 48, 49, 50),
                      *pick_args);
    const std::array<Byte, 395> bc4{{
        LDC, 2, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0, LDC, 0, 0,
        LDC, 1, 0,
        APPLY, Byte(-127),
    }};
    eval_bytecode(*constants, nil, 0, bc4);
    ex = array(129 + 50, *x, *x, 1, 50);
    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_VALS(*ex, stack[0]);
}

TEST_F(vm_test, cnil)
{
    stack_push(*THREE);

    const std::array<Byte, 1> bc1{{CNIL}};
    eval_bytecode(nil, nil, 0, bc1);
    ASSERT_EQ(2u, stack.size());
    EXPECT_EQ_VALS(nil, stack[1]);
    EXPECT_EQ_VALS(*THREE, stack[0]);

    const std::array<Byte, 2> bc2{{CNIL, CNIL}};
    eval_bytecode(nil, nil, 0, bc2);
    ASSERT_EQ(4u, stack.size());
    EXPECT_EQ_VALS(nil, stack[3]);
    EXPECT_EQ_VALS(nil, stack[2]);
    EXPECT_EQ_VALS(nil, stack[1]);
    EXPECT_EQ_VALS(*THREE, stack[0]);
}

TEST_F(vm_test, ifn)
{
    auto name = create_symbol("abc");
    Root fn{create_bytecode_fn(name, nullptr, nullptr, 0)};
    stack_push(*fn);
    const std::array<Byte, 2> bc1{{IFN, 0}};
    eval_bytecode(nil, nil, 0, bc1);

    ASSERT_EQ(1u, stack.size());
    EXPECT_EQ_REFS(*fn, stack[0]);
    stack.clear();

    Root consts1{array(10, 20, 30, 40, 50, 60)};
    Root consts2{array(50, 60, 70, 80, 90, 100)};
    std::array<vm::Byte, 1> bytes{{vm::CNIL}};
    Roots rbodies(2);
    rbodies.set(0, create_bytecode_fn_body(*consts1, nil, nil, 0, bytes.data(), bytes.size()));
    rbodies.set(1, create_bytecode_fn_body(*consts2, nil, nil, 0, bytes.data(), bytes.size()));
    std::array<Value, 2> bodies{{rbodies[0], rbodies[1]}};
    std::array<Int64, 2> arities{{2, 3}};

    fn = create_bytecode_fn(name, arities.data(), bodies.data(), bodies.size());
    stack_push(*fn);
    stack_push(i64(17));
    stack_push(i64(19));
    std::array<Byte, 2> bc2{{IFN, 2}};
    eval_bytecode(nil, nil, 0, bc2);

    ASSERT_EQ(1u, stack.size());
    auto mfn = stack[0];
    Root mconsts1{array(10, 20, 30, 40, 17, 19)};
    Root mconsts2{array(50, 60, 70, 80, 17, 19)};
    EXPECT_EQ_REFS(*type::BytecodeFn, get_value_type(mfn));
    EXPECT_EQ(2, get_bytecode_fn_size(mfn));
    EXPECT_EQ_VALS(name, get_bytecode_fn_name(mfn));
    EXPECT_EQ_VALS(*mconsts1, get_bytecode_fn_body_consts(get_bytecode_fn_body(mfn, 0)));
    EXPECT_EQ_VALS(*mconsts2, get_bytecode_fn_body_consts(get_bytecode_fn_body(mfn, 1)));
}

TEST_F(vm_test, throw_)
{
    Root ex{new_index_out_of_bounds()};
    stack_push(i64(10));
    stack_push(i64(12));
    stack_push(*ex);

    const std::array<Byte, 6> bc1{{CNIL, CNIL, LDL, 0, 0, THROW}};
    try
    {
        eval_bytecode(nil, nil, 1, bc1);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root actual{catch_exception()};
        EXPECT_EQ_REFS(*ex, *actual);
    }

    EXPECT_EQ(6u, stack.size());
    EXPECT_EQ_REFS(*ex, stack[5]);
    EXPECT_EQ_VALS(nil, stack[4]);
    EXPECT_EQ_VALS(nil, stack[3]);
    EXPECT_EQ_REFS(*ex, stack[2]);
}

TEST_F(vm_test, catching_exceptions_from_throw)
{
    Root ex{new_index_out_of_bounds()};

    const std::array<Byte, 9> bc1{{CNIL, CNIL, LDL, 0, 0, THROW, CNIL, CNIL, CNIL}};
    std::array<Int64, 3> et{{5, 6, 8}};
    std::array<Value, 1> types{{*type::IndexOutOfBounds}};
    stack_push(*THREE);
    stack_push(*TWO);
    stack_push(*ex);
    eval_bytecode(nil, nil, 1, et, types, bc1);

    ASSERT_EQ(5u, stack.size());
    EXPECT_EQ_VALS(nil, stack[4]);
    EXPECT_EQ_REFS(*ex, stack[3]);
    EXPECT_EQ_REFS(*ex, stack[2]);
    EXPECT_EQ_REFS(*TWO, stack[1]);
    EXPECT_EQ_REFS(*THREE, stack[0]);
    stack.clear();

    std::array<Value, 1> other_types{{*type::IllegalArgument}};
    stack_push(*ex);
    try
    {
        eval_bytecode(nil, nil, 1, et, other_types, bc1);
        FAIL() << "expected an exception";
    }
    catch (const Exception& )
    {
        Root actual{catch_exception()};
        EXPECT_EQ_REFS(*ex, *actual);
    }
}

TEST_F(vm_test, catching_exceptions_from_call)
{
    Root constants{array(8, *rt::first)};
    const std::array<Byte, 12> bc1{{CNIL, CNIL, LDC, 1, 0, LDC, 0, 0, CALL, 1, CNIL, CNIL}};
    const std::array<Int64, 3> et{{8, 9, 11}};
    const std::array<Value, 1> types{{*type::IllegalArgument}};
    stack_push(*TWO);
    stack_push(*THREE);
    eval_bytecode(*constants, nil, 1, et, types, bc1);

    ASSERT_EQ(4u, stack.size());
    EXPECT_EQ_VALS(nil, stack[3]);
    EXPECT_EQ_REFS(*type::IllegalArgument, get_value_type(stack[2]));
    EXPECT_EQ_VALS(*THREE, stack[1]);
    EXPECT_EQ_VALS(*TWO, stack[0]);
    stack.clear();

    const std::array<Value, 1> other_types{{*type::IndexOutOfBounds}};
    stack_push(*TWO);
    stack_push(*THREE);
    try
    {
        eval_bytecode(*constants, nil, 1, et, other_types, bc1);
    }
    catch (const Exception& )
    {
        Root actual{catch_exception()};
        EXPECT_EQ_REFS(*type::IllegalArgument, get_value_type(*actual));
        ASSERT_EQ(6u, stack.size());
        EXPECT_EQ_REFS(*rt::first, stack[4]);
        EXPECT_EQ_VALS(nil, stack[3]);
        EXPECT_EQ_VALS(nil, stack[2]);
        EXPECT_EQ_VALS(*THREE, stack[1]);
        EXPECT_EQ_VALS(*TWO, stack[0]);
    }
}

TEST_F(vm_test, catching_exceptions_from_apply)
{
    Root constants{array(8, *rt::first)};
    const std::array<Byte, 13> bc1{{CNIL, CNIL, LDC, 1, 0, LDC, 0, 0, CNIL, APPLY, 1, CNIL, CNIL}};
    const std::array<Int64, 3> et{{9, 10, 12}};
    const std::array<Value, 1> types{{*type::IllegalArgument}};
    stack_push(*TWO);
    stack_push(*THREE);
    eval_bytecode(*constants, nil, 1, et, types, bc1);

    ASSERT_EQ(4u, stack.size());
    EXPECT_EQ_VALS(nil, stack[3]);
    EXPECT_EQ_REFS(*type::IllegalArgument, get_value_type(stack[2]));
    EXPECT_EQ_VALS(*THREE, stack[1]);
    EXPECT_EQ_VALS(*TWO, stack[0]);
    stack.clear();

    const std::array<Value, 1> other_types{{*type::IndexOutOfBounds}};
    stack_push(*TWO);
    stack_push(*THREE);
    try
    {
        eval_bytecode(*constants, nil, 1, et, other_types, bc1);
    }
    catch (const Exception& )
    {
        Root actual{catch_exception()};
        EXPECT_EQ_REFS(*type::IllegalArgument, get_value_type(*actual));
        ASSERT_EQ(7u, stack.size());
        EXPECT_EQ_VALS(nil, stack[6]);
        EXPECT_EQ_REFS(*rt::first, stack[4]);
        EXPECT_EQ_VALS(nil, stack[3]);
        EXPECT_EQ_VALS(nil, stack[2]);
        EXPECT_EQ_VALS(*THREE, stack[1]);
        EXPECT_EQ_VALS(*TWO, stack[0]);
    }
}

}
}
}
