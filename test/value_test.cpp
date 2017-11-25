#include <cleo/value.hpp>
#include <limits>
#include <string>
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

TEST(value_test, nil_should_have_type_NIL)
{
    Value val = get_nil();
    ASSERT_EQ(tag::NIL, get_value_tag(val));
}

TEST(value_test, there_should_be_one_instance_of_nil)
{
    Value val = get_nil();
    Value val2 = get_nil();
    ASSERT_TRUE(val == val2);
}

TEST(value_test, should_store_native_functions)
{
    auto f = [](const Value *, std::uint8_t) { return get_nil(); };
    Value val = create_native_function(f);
    ASSERT_EQ(tag::NATIVE_FUNCTION, get_value_tag(val));
    ASSERT_EQ(f, get_native_function_ptr(val));
}

TEST(value_test, should_create_a_new_instance_for_each_function)
{
    auto f = [](const Value *, std::uint8_t) { return get_nil(); };
    Value val = create_native_function(f);
    Value val2 = create_native_function(f);
    ASSERT_TRUE(val != val2);
}

TEST(value_test, should_store_int_values)
{
    Value val = create_int64(7);
    ASSERT_EQ(tag::INT64, get_value_tag(val));
    ASSERT_EQ(7, get_int64_value(val));
    val = create_int64(std::numeric_limits<Int64>::min());
    ASSERT_EQ(std::numeric_limits<Int64>::min(), get_int64_value(val));
    val = create_int64(std::numeric_limits<Int64>::max());
    ASSERT_EQ(std::numeric_limits<Int64>::max(), get_int64_value(val));
}

TEST(value_test, should_create_a_new_instance_for_each_int)
{
    Value val = create_int64(7);
    Value val2 = create_int64(7);
    ASSERT_TRUE(val != val2);
}

TEST(value_test, should_store_float_values)
{
    Value val = create_float64(7.125);
    ASSERT_EQ(tag::FLOAT64, get_value_tag(val));
    ASSERT_EQ(7.125, get_float64_value(val));
    val = create_float64(std::numeric_limits<Float64>::min());
    ASSERT_EQ(std::numeric_limits<Float64>::min(), get_float64_value(val));
    val = create_float64(std::numeric_limits<Float64>::max());
    ASSERT_EQ(std::numeric_limits<Float64>::max(), get_float64_value(val));
}

TEST(value_test, should_create_a_new_instance_for_each_float)
{
    Value val = create_float64(7);
    Value val2 = create_float64(7);
    ASSERT_TRUE(val != val2);
}

TEST(value_test, should_store_string_values)
{
    std::string example("abc\0xyz", 7);
    std::string exampleCopy = example;
    Value val = create_string(exampleCopy.data(), exampleCopy.length());
    exampleCopy.clear();
    ASSERT_EQ(tag::STRING, get_value_tag(val));
    ASSERT_EQ(example, std::string(get_string_ptr(val), get_string_len(val)));
}

TEST(value_test, should_create_a_new_instance_for_each_string)
{
    Value val = create_string("abc", 3);
    Value val2 = create_string("abc", 3);
    ASSERT_TRUE(val != val2);
}

}
}
