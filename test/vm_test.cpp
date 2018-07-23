#include <cleo/vm.hpp>
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
        vm::eval_bytecode(stack, constants, vars, locals_size, bc.data(), bc.size());
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

}
}
}
