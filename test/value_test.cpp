#include <cleo/value.hpp>
#include <limits>
#include <string>
#include <array>
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

TEST(value_test, should_store_symbols_with_namespaces)
{
    Value sym = create_symbol("org.xxx", 7, "thing", 5);
    ASSERT_EQ(tag::SYMBOL, get_value_tag(sym));
    auto ns = get_symbol_namespace(sym);
    ASSERT_EQ(tag::STRING, get_value_tag(ns));
    ASSERT_EQ("org.xxx", std::string(get_string_ptr(ns), get_string_len(ns)));
    auto name = get_symbol_name(sym);
    ASSERT_EQ(tag::STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_len(name)));
}

TEST(value_test, should_store_symbols_without_namespaces)
{
    Value sym = create_symbol("thing", 5);
    ASSERT_EQ(tag::SYMBOL, get_value_tag(sym));
    ASSERT_TRUE(get_nil() == get_symbol_namespace(sym));
    auto name = get_symbol_name(sym);
    ASSERT_EQ(tag::STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_len(name)));
}

TEST(value_test, should_return_same_instances_for_symbols_with_same_namespace_and_names)
{
    EXPECT_TRUE(create_symbol("abc", 3) != create_symbol("xyz", 3));
    EXPECT_TRUE(create_symbol("abc", 3) == create_symbol("abc", 3));
    EXPECT_TRUE(create_symbol("org.xxx", 7, "abc", 3) != create_symbol("abc", 3));
    EXPECT_TRUE(create_symbol("org.xxx", 7, "abc", 3) != create_symbol("org.xxx", 7, "xyz", 3));
    EXPECT_TRUE(create_symbol("org.xxx", 7, "abc", 3) == create_symbol("org.xxx", 7, "abc", 3));
    EXPECT_TRUE(create_symbol("com.z", 5, "abc", 3) != create_symbol("org.xxx", 7, "abc", 3));
}

TEST(value_test, should_store_keywords_with_namespaces)
{
    Value kw = create_keyword("org.xxx", 7, "thing", 5);
    ASSERT_EQ(tag::KEYWORD, get_value_tag(kw));
    auto ns = get_keyword_namespace(kw);
    ASSERT_EQ(tag::STRING, get_value_tag(ns));
    ASSERT_EQ("org.xxx", std::string(get_string_ptr(ns), get_string_len(ns)));
    auto name = get_keyword_name(kw);
    ASSERT_EQ(tag::STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_len(name)));
}

TEST(value_test, should_store_keywords_without_namespaces)
{
    Value kw = create_keyword("thing", 5);
    ASSERT_EQ(tag::KEYWORD, get_value_tag(kw));
    ASSERT_TRUE(get_nil() == get_keyword_namespace(kw));
    auto name = get_keyword_name(kw);
    ASSERT_EQ(tag::STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_len(name)));
}

TEST(value_test, should_return_same_instances_for_keywords_with_same_namespace_and_names)
{
    EXPECT_TRUE(create_keyword("abc", 3) != create_keyword("xyz", 3));
    EXPECT_TRUE(create_keyword("abc", 3) == create_keyword("abc", 3));
    EXPECT_TRUE(create_keyword("org.xxx", 7, "abc", 3) != create_keyword("abc", 3));
    EXPECT_TRUE(create_keyword("org.xxx", 7, "abc", 3) != create_keyword("org.xxx", 7, "xyz", 3));
    EXPECT_TRUE(create_keyword("org.xxx", 7, "abc", 3) == create_keyword("org.xxx", 7, "abc", 3));
    EXPECT_TRUE(create_keyword("com.z", 5, "abc", 3) != create_keyword("org.xxx", 7, "abc", 3));
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

TEST(value_test, should_store_object_values)
{
    auto type = create_symbol("org.xxx", 7);
    const std::array<Value, 3> elems{{create_int64(10), create_float64(20), create_symbol("elem3", 5)}};
    Value obj = create_object(type, elems.data(), elems.size());
    ASSERT_EQ(tag::OBJECT, get_value_tag(obj));
    ASSERT_EQ(elems.size(), get_object_size(obj));
    ASSERT_TRUE(elems[0] == get_object_element(obj, 0));
    ASSERT_TRUE(elems[1] == get_object_element(obj, 1));
    ASSERT_TRUE(elems[2] == get_object_element(obj, 2));

    ASSERT_EQ(0u, get_object_size(create_object(type, nullptr, 0)));
}

TEST(value_test, should_create_a_new_instance_for_each_object)
{
    auto type = create_symbol("org.xxx", 7);
    Value val = create_object(type, nullptr, 0);
    Value val2 = create_object(type, nullptr, 0);
    ASSERT_TRUE(val != val2);
}

TEST(value_test, should_return_the_type_of_a_value)
{
    auto f = [](const Value *, std::uint8_t) { return get_nil(); };
    auto type = create_symbol("org.xxx", 7);
    ASSERT_TRUE(get_nil() == get_value_type(get_nil()));
    ASSERT_TRUE(create_symbol("cleo.core", 9, "NativeFunction", 14) == get_value_type(create_native_function(f)));
    ASSERT_TRUE(create_symbol("cleo.core", 9, "Symbol", 6) == get_value_type(create_symbol("abc", 3)));
    ASSERT_TRUE(create_symbol("cleo.core", 9, "Keyword", 7) == get_value_type(create_keyword("abc", 3)));
    ASSERT_TRUE(create_symbol("cleo.core", 9, "Int64", 5) == get_value_type(create_int64(11)));
    ASSERT_TRUE(create_symbol("cleo.core", 9, "Float64", 7) == get_value_type(create_float64(3.5)));
    ASSERT_TRUE(create_symbol("cleo.core", 9, "String", 6) == get_value_type(create_string("abc", 3)));
    ASSERT_TRUE(type == get_value_type(create_object(type, nullptr, 0)));
}

}
}
