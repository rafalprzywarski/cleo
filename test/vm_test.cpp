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
    Stack stack;
    const std::size_t extra_roots_size;

    vm_test() : Test("cleo.vm.test"), extra_roots_size(extra_roots.size()) { }
    ~vm_test() { extra_roots.resize(extra_roots_size); }

    void push(Force val)
    {
        stack.push_back(val.value());
        extra_roots.push_back(val.value());
    }

    static Force create_constants(std::vector<std::pair<std::uint32_t, Int64>> vals)
    {
        std::uint32_t size = 65536;
        Roots rs{size};
        std::vector<Value> vs(size);
        for (auto& v : vals)
        {
            rs.set(v.first, i64(v.second));
            vs.push_back(rs[v.first]);
        }
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
    eval_bytecode(stack, *constants, nil, 0, bc1.data(), bc1.size());

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
    eval_bytecode(stack, *constants, nil, 0, bc2.data(), bc2.size());

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

TEST_F(vm_test, pop)
{
    push(*THREE);
    push(*ONE);
    push(*TWO);

    const std::array<Byte, 1> bc1{{POP}};
    eval_bytecode(stack, nil, nil, 0, bc1.data(), bc1.size());
    ASSERT_EQ(2u, stack.size());
    EXPECT_EQ_VALS(*ONE, stack[1]);
    EXPECT_EQ_VALS(*THREE, stack[0]);

    const std::array<Byte, 2> bc2{{POP, POP}};
    eval_bytecode(stack, nil, nil, 0, bc2.data(), bc2.size());
    ASSERT_TRUE(stack.empty());
}

}
}
}
