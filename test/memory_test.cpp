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
    const Int64 LARGE_INT_VAL = Int64(20) << 48;
    const Int64 LARGE_INT_VAL2 = Int64(30) << 48;

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

    create_int64(LARGE_INT_VAL);
    ASSERT_EQ(num_allocations + 1, allocations.size());

    create_int64(LARGE_INT_VAL);
    create_int64(LARGE_INT_VAL);
    ASSERT_EQ(num_allocations + 3, allocations.size());

    gc();

    ASSERT_EQ(num_allocations, allocations.size());
}

TEST_F(memory_test, Root_should_prevent_garbage_collection)
{
    auto num_allocations = allocations.size();

    Root root1;
    root1 = create_int64(LARGE_INT_VAL);
    ASSERT_EQ(num_allocations + 1, allocations.size());

    gc();
    ASSERT_EQ(num_allocations + 1, allocations.size());

    {
        Root root2, root3;
        root2 = create_int64(LARGE_INT_VAL);
        root3 = create_int64(LARGE_INT_VAL);
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

TEST_F(memory_test, should_allocate_symbols_permanently)
{
    create_symbol("cleo.memory.test", "not_collected1");
    create_symbol("cleo.memory.test", "not_collected2");
    create_symbol("cleo.memory.test2", "not_collected3");

    auto num_allocations = allocations.size();

    gc();
    ASSERT_EQ(num_allocations, allocations.size());
}

TEST_F(memory_test, should_allocate_keywords_permanently)
{
    create_keyword("cleo.memory.test", "not_collected1");
    create_keyword("cleo.memory.test", "not_collected2");
    create_keyword("cleo.memory.test2", "not_collected3");

    auto num_allocations = allocations.size();

    gc();
    ASSERT_EQ(num_allocations, allocations.size());
}

TEST_F(memory_test, should_collect_dynamic_objects)
{
    Root type1{create_dynamic_object_type("cleo.memory.test", "obj1")};
    Root type2{create_dynamic_object_type("cleo.memory.test", "obj2")};
    Root type3{create_dynamic_object_type("cleo.memory.test", "obj3")};
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

TEST_F(memory_test, should_not_collect_chars_in_objects)
{
    Root type1{create_dynamic_object_type("cleo.memory.test", "obj1")};
    Root c{create_uchar(123)};

    auto num_allocations_before = allocations.size();

    Root r{create_object1(*type1, *c)};
    r = nil;
    gc();

    ASSERT_EQ(num_allocations_before, allocations.size());
}

TEST_F(memory_test, should_collect_static_objects)
{
    Root type1{create_dynamic_object_type("cleo.memory.test", "obj1")};
    Root type2{create_dynamic_object_type("cleo.memory.test", "obj2")};
    std::array<Value, 3> names{{create_symbol("a"), create_symbol("b"), create_symbol("c")}};
    std::array<Value, 3> types{{*type1, *type2, type::Int64}};
    Root type4{create_static_object_type("cleo.memory.test", "obj4", names.data(), types.data(), names.size())};
    auto num_allocations_before = allocations.size();

    Root root1, root2, root3, root4;
    root1 = create_object0(*type1);
    root2 = create_object0(*type2);
    root3 = create_int64(30);
    root4 = create_object3(*type4, *root1, *root2, *root3);
    root3 = nil;

    auto num_allocations_after = allocations.size();

    root1 = nil;
    root2 = nil;
    gc();
    ASSERT_EQ(num_allocations_after, allocations.size());

    root4 = nil;
    gc();
    ASSERT_EQ(num_allocations_before, allocations.size());
}

TEST_F(memory_test, should_trace_object_field_types)
{
    std::array<Value, 3> names{{create_symbol("a"), create_symbol("b"), create_symbol("c")}};

    auto num_allocations_before = allocations.size();

    Root type1{create_dynamic_object_type("cleo.memory.test", "obj1")};
    Root type2{create_dynamic_object_type("cleo.memory.test", "obj2")};
    std::array<Value, 3> types{{*type1, *type2, type::Int64}};
    Root type3{create_static_object_type("cleo.memory.test", "obj4", names.data(), types.data(), names.size())};
    types[0] = nil;
    types[1] = nil;

    auto num_allocations_after = allocations.size();

    type1 = nil;
    type2 = nil;
    gc();
    ASSERT_EQ(num_allocations_after, allocations.size());

    type3 = nil;
    gc();
    ASSERT_EQ(num_allocations_before, allocations.size());
}

TEST_F(memory_test, should_handle_cycles)
{
    Root type1{create_dynamic_object_type("cleo.memory.test", "obj1")};
    Root type2{create_dynamic_object_type("cleo.memory.test", "obj2")};
    Root type3{create_dynamic_object_type("cleo.memory.test", "obj3")};
    auto num_allocations_before = allocations.size();

    Root root1, root2, root3;
    root1 = create_object1(*type1, nil);
    root2 = create_object0(*type2);
    root3 = create_object2(*type3, *root1, *root2);
    set_dynamic_object_element(*root1, 0, *root3);

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
    create_int64(LARGE_INT_VAL);

    ASSERT_EQ(3u, gc_counter);
    ASSERT_EQ(num_allocations + 1, allocations.size());

    create_int64(LARGE_INT_VAL);
    create_int64(LARGE_INT_VAL);
    create_int64(LARGE_INT_VAL);

    ASSERT_EQ(0u, gc_counter);
    ASSERT_EQ(num_allocations + 4, allocations.size());

    gc_frequency = 16;
    create_int64(LARGE_INT_VAL);

    ASSERT_EQ(15u, gc_counter);
    ASSERT_EQ(num_allocations + 1, allocations.size());
}

TEST_F(memory_test, should_trace_vars)
{
    auto name1 = create_symbol("cleo.memory.test", "var1");
    auto name2 = create_symbol("cleo.memory.test", "var2");

    auto num_allocations_before = allocations.size();

    Root val1{create_int64(LARGE_INT_VAL)}, val2{create_int64(LARGE_INT_VAL2)};
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

TEST_F(memory_test, should_trace_global_stack)
{
    auto num_allocations = allocations.size();

    stack_push(create_int64(LARGE_INT_VAL));
    ASSERT_EQ(num_allocations + 1, allocations.size());

    gc();
    ASSERT_EQ(num_allocations + 1, allocations.size());

    stack_push(create_int64(LARGE_INT_VAL));
    stack_push(create_int64(LARGE_INT_VAL));
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
