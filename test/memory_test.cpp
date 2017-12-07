#include <cleo/memory.hpp>
#include <cleo/global.hpp>
#include <cleo/var.hpp>
#include <cleo/multimethod.hpp>
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

TEST_F(memory_test, should_trace_symbols)
{
    create_symbol("cleo.memory.test", "not_collected1");
    create_symbol("cleo.memory.test", "not_collected2");
    create_symbol("cleo.memory.test2", "not_collected3");

    auto num_allocations = allocations.size();

    gc();
    ASSERT_EQ(num_allocations, allocations.size());
}

TEST_F(memory_test, should_trace_keywords)
{
    create_keyword("cleo.memory.test", "not_collected1");
    create_keyword("cleo.memory.test", "not_collected2");
    create_keyword("cleo.memory.test2", "not_collected3");

    auto num_allocations = allocations.size();

    gc();
    ASSERT_EQ(num_allocations, allocations.size());
}

TEST_F(memory_test, should_collect_objects)
{
    auto name1 = create_symbol("cleo.memory.test", "obj1");
    auto name2 = create_symbol("cleo.memory.test", "obj2");
    auto name3 = create_symbol("cleo.memory.test", "obj3");
    auto num_allocations_before = allocations.size();

    Root root1, root2, root3;
    *root1 = create_object0(name1);
    *root2 = create_object0(name2);
    *root3 = create_object2(name3, *root1, *root2);

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

TEST_F(memory_test, should_trace_vars)
{
    auto name1 = create_symbol("cleo.memory.test", "var1");
    auto name2 = create_symbol("cleo.memory.test", "var2");

    auto num_allocations_before = allocations.size();

    define(name1, create_int64(20));
    define(name2, create_int64(30));
    auto num_allocations_after = allocations.size();

    gc();
    ASSERT_EQ(num_allocations_after, allocations.size());

    define(name1, nil);
    define(name2, nil);

    gc();
    ASSERT_EQ(num_allocations_before, allocations.size());
}

TEST_F(memory_test, should_trace_multimethods)
{
    auto name = create_string("cleo.memory.test/mm1");
    auto fn = create_native_function([](const Value *, std::uint8_t){ return nil; });
    auto fn1 = create_native_function([](const Value *, std::uint8_t){ return nil; });
    auto def_val = create_string("cleo.memory.test/mm_def_val");
    auto val1 = create_string("cleo.memory.test/mm_val1");

    define_multimethod(name, fn, def_val);
    define_method(name, val1, fn1);

    auto num_allocations = allocations.size();

    gc();
    ASSERT_EQ(num_allocations, allocations.size());
}

TEST_F(memory_test, should_trace_global_hierarchy)
{
    auto s1 = create_string("cleo.memory.test/string1");
    auto s2 = create_string("cleo.memory.test/string2");

    auto num_allocations = allocations.size();

    derive(s1, s2);

    gc();
    ASSERT_EQ(num_allocations, allocations.size());
}

}
}
