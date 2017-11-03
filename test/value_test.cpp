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
    ASSERT_EQ(type::NIL, get_value_type(val));
}

TEST(value_test, there_should_be_one_instance_of_nil)
{
    Value val = get_nil();
    Value val2 = get_nil();
    ASSERT_TRUE(val == val2);
}

TEST(value_test, should_store_int_values)
{
    Value val = create_int(7);
    ASSERT_EQ(type::INT, get_value_type(val));
    ASSERT_EQ(7, get_int_value(val));
    val = create_int(std::numeric_limits<Int>::min());
    ASSERT_EQ(std::numeric_limits<Int>::min(), get_int_value(val));
    val = create_int(std::numeric_limits<Int>::max());
    ASSERT_EQ(std::numeric_limits<Int>::max(), get_int_value(val));
}

TEST(value_test, should_create_a_new_instance_for_each_int)
{
    Value val = create_int(7);
    Value val2 = create_int(7);
    ASSERT_TRUE(val != val2);
}

TEST(value_test, should_store_string_values)
{
    std::string example("abc\0xyz", 7);
    std::string exampleCopy = example;
    Value val = create_string(exampleCopy.data(), exampleCopy.length());
    exampleCopy.clear();
    ASSERT_EQ(type::STRING, get_value_type(val));
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
