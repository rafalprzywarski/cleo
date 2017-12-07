#include <cleo/memory.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct memory_test : testing::Test
{
    Override<decltype(gc_frequency)> ovf{gc_frequency, 10000};
    Override<decltype(gc_counter)> ovc{gc_counter, 10000};

    memory_test()
    {
        gc();
    }
};

TEST_F(memory_test, should_add_allocations)
{
    auto num_allocations = allocations.size();

    create_int64(20);
    ASSERT_EQ(num_allocations + 1, allocations.size());

    create_int64(20);
    create_int64(20);
    ASSERT_EQ(num_allocations + 3, allocations.size());

    gc();

    ASSERT_EQ(num_allocations, allocations.size());
}

TEST_F(memory_test, Root_should_prevent_garbage_collection)
{
    auto num_allocations = allocations.size();

    Root root1;
    *root1 = create_int64(20);
    ASSERT_EQ(num_allocations + 1, allocations.size());

    gc();
    ASSERT_EQ(num_allocations + 1, allocations.size());

    {
        Root root2, root3;
        *root2 = create_int64(20);
        *root3 = create_int64(20);
        ASSERT_EQ(num_allocations + 3, allocations.size());

        gc();
        ASSERT_EQ(num_allocations + 3, allocations.size());
    }

    gc();
    ASSERT_EQ(num_allocations + 1, allocations.size());

    *root1 = nil;
    gc();
    ASSERT_EQ(num_allocations, allocations.size());
}

TEST_F(memory_test, should_collect_symbols)
{
    auto num_allocations_before = allocations.size();

    Root root;
    *root = create_symbol("cleo.memory.test", "to_collect");

    auto num_allocations_after = allocations.size();

    gc();
    ASSERT_EQ(num_allocations_after, allocations.size());

    *root = nil;
    gc();
    ASSERT_EQ(num_allocations_before, allocations.size());
}

TEST_F(memory_test, should_collect_keywords)
{
    auto num_allocations_before = allocations.size();

    Root root;
    *root = create_keyword("cleo.memory.test", "to_collect");

    auto num_allocations_after = allocations.size();

    gc();
    ASSERT_EQ(num_allocations_after, allocations.size());

    *root = nil;
    gc();
    ASSERT_EQ(num_allocations_before, allocations.size());
}

TEST_F(memory_test, should_collect_objects)
{
    auto num_allocations_before = allocations.size();

    Root root1, root2, root3;
    *root1 = create_object0(create_symbol("cleo.memory.text", "obj1"));
    *root2 = create_object0(create_symbol("cleo.memory.text", "obj2"));
    *root3 = create_object2(create_symbol("cleo.memory.text", "obj3"), *root1, *root2);

    auto num_allocations_after = allocations.size();

    *root1 = nil;
    *root2 = nil;
    gc();
    ASSERT_EQ(num_allocations_after, allocations.size());

    *root3 = nil;
    gc();
    ASSERT_EQ(num_allocations_before, allocations.size());
}

TEST_F(memory_test, alloc_should_periodically_call_gc)
{
    auto num_allocations = allocations.size();

    gc_counter = 4;
    create_int64(20);

    ASSERT_EQ(3u, gc_counter);
    ASSERT_EQ(num_allocations + 1, allocations.size());

    create_int64(20);
    create_int64(20);
    create_int64(20);

    ASSERT_EQ(0u, gc_counter);
    ASSERT_EQ(num_allocations + 4, allocations.size());

    gc_frequency = 16;
    create_int64(20);

    ASSERT_EQ(15u, gc_counter);
    ASSERT_EQ(num_allocations + 1, allocations.size());
}

}
}
