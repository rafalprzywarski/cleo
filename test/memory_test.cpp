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
        EXPECT_TRUE(stack.empty());
    }

    ~memory_test()
    {
        stack.clear();
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
    root1 = create_int64(20);
    ASSERT_EQ(num_allocations + 1, allocations.size());

    gc();
    ASSERT_EQ(num_allocations + 1, allocations.size());

    {
        Root root2, root3;
        root2 = create_int64(20);
        root3 = create_int64(20);
        ASSERT_EQ(num_allocations + 3, allocations.size());

        gc();
        ASSERT_EQ(num_allocations + 3, allocations.size());
    }

    gc();
    ASSERT_EQ(num_allocations + 1, allocations.size());

    root1 = nil;
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
    Root type1{create_object_type("cleo.memory.test", "obj1")};
    Root type2{create_object_type("cleo.memory.test", "obj2")};
    Root type3{create_object_type("cleo.memory.test", "obj3")};
    auto num_allocations_before = allocations.size();

    Root root1, root2, root3;
    root1 = create_object0(*type1);
    root2 = create_object0(*type2);
    root3 = create_object2(*type3, *root1, *root2);

    auto num_allocations_after = allocations.size();

    root1 = nil;
    root2 = nil;
    gc();
    ASSERT_EQ(num_allocations_after, allocations.size());

    root3 = nil;
    gc();
    ASSERT_EQ(num_allocations_before, allocations.size());
}

TEST_F(memory_test, should_handle_cycles)
{
    Root type1{create_object_type("cleo.memory.test", "obj1")};
    Root type2{create_object_type("cleo.memory.test", "obj2")};
    Root type3{create_object_type("cleo.memory.test", "obj3")};
    auto num_allocations_before = allocations.size();

    Root root1, root2, root3;
    root1 = create_object1(*type1, nil);
    root2 = create_object0(*type2);
    root3 = create_object2(*type3, *root1, *root2);
    set_object_element(*root1, 0, *root3);
    set_object_type(*root2, *root3);

    auto num_allocations_after = allocations.size();

    root1 = nil;
    root2 = nil;
    gc();
    ASSERT_EQ(num_allocations_after, allocations.size());

    root3 = nil;
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

    Root val1{create_int64(20)}, val2{create_int64(30)};
    define_var(name1, *val1);
    define_var(name2, *val2);
    val1 = nil;
    val2 = nil;
    auto num_allocations_after = allocations.size();

    gc();
    ASSERT_EQ(num_allocations_after, allocations.size());

    undefine_var(name1);
    undefine_var(name2);

    gc();
    ASSERT_EQ(num_allocations_before, allocations.size());
}

TEST_F(memory_test, should_trace_multimethods)
{
    Root name{create_symbol("cleo.memory.test", "mm1")};
    Root fn{create_native_function([](const Value *, std::uint8_t){ return force(nil); })};
    Root fn1{create_native_function([](const Value *, std::uint8_t){ return force(nil); })};
    Root def_val{create_string("cleo.memory.test/mm_def_val")};
    Root val1{create_string("cleo.memory.test/mm_val1")};

    define(*name, nil);
    gc();

    auto before_ns = allocations.size();
    define(*name, nil);
    auto after_ns = allocations.size();
    gc();

    auto mm = define_multimethod(*name, *fn, *def_val);
    define_method(*name, *val1, *fn1);
    get_method(mm, *val1);
    name = nil;
    fn = nil;
    fn1 = nil;
    def_val = nil;
    val1 = nil;

    auto num_allocations = allocations.size();

    gc();
    ASSERT_EQ(num_allocations - after_ns + before_ns, allocations.size());
}

TEST_F(memory_test, should_trace_global_hierarchy)
{
    Root s1{create_string("cleo.memory.test/string1")};
    Root s2{create_string("cleo.memory.test/string2")};

    auto num_allocations = allocations.size();

    derive(*s1, *s2);
    s1 = nil;
    s2 = nil;

    gc();
    ASSERT_EQ(num_allocations, allocations.size());
}

TEST_F(memory_test, should_trace_global_stack)
{
    auto num_allocations = allocations.size();

    stack_push(create_int64(20));
    ASSERT_EQ(num_allocations + 1, allocations.size());

    gc();
    ASSERT_EQ(num_allocations + 1, allocations.size());

    stack_push(create_int64(20));
    stack_push(create_int64(20));
    ASSERT_EQ(num_allocations + 3, allocations.size());

    gc();
    ASSERT_EQ(num_allocations + 3, allocations.size());

    stack_pop();
    stack_pop();
    gc();
    ASSERT_EQ(num_allocations + 1, allocations.size());

    stack.back() = nil;
    gc();
    ASSERT_EQ(num_allocations, allocations.size());
}

}
}
