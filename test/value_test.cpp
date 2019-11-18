#include <cleo/value.hpp>
#include <cleo/global.hpp>
#include <limits>
#include <string>
#include <cstring>
#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct value_test : Test
{
    value_test() : Test("cleo.value.test") { }
};

TEST_F(value_test, nil_should_have_tag_OBJECT)
{
    ASSERT_EQ(tag::OBJECT, get_value_tag(nil));
}

TEST_F(value_test, should_store_native_functions)
{
    auto f = [](const Value *, std::uint8_t) { return force(nil); };
    Root val{create_native_function(f)};
    ASSERT_EQ(tag::NATIVE_FUNCTION, get_value_tag(*val));
    ASSERT_EQ(f, get_native_function_ptr(*val));
}

TEST_F(value_test, should_create_a_new_instance_for_each_function)
{
    auto f = [](const Value *, std::uint8_t) { return force(nil); };
    Root val, val2;
    val = create_native_function(f);
    val2 = create_native_function(f);
    ASSERT_FALSE(val->is(*val2));
}

TEST_F(value_test, should_store_symbols_with_namespaces)
{
    Value sym = create_symbol("org.xxx", "thing");
    ASSERT_EQ(tag::SYMBOL, get_value_tag(sym));
    auto ns = get_symbol_namespace(sym);
    ASSERT_EQ(tag::STRING, get_value_tag(ns));
    ASSERT_EQ("org.xxx", std::string(get_string_ptr(ns), get_string_len(ns)));
    auto name = get_symbol_name(sym);
    ASSERT_EQ(tag::STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_len(name)));
}

TEST_F(value_test, should_store_symbols_without_namespaces)
{
    Value sym = create_symbol("thing");
    ASSERT_EQ(tag::SYMBOL, get_value_tag(sym));
    ASSERT_TRUE(get_symbol_namespace(sym).is_nil());
    auto name = get_symbol_name(sym);
    ASSERT_EQ(tag::STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_len(name)));
}

TEST_F(value_test, should_return_same_instances_for_symbols_with_same_namespace_and_names)
{
    EXPECT_FALSE(create_symbol("abc").is(create_symbol("xyz")));
    EXPECT_TRUE(create_symbol("abc").is(create_symbol("abc")));
    EXPECT_FALSE(create_symbol("org.xxx", "abc").is(create_symbol("abc")));
    EXPECT_FALSE(create_symbol("org.xxx", "abc").is(create_symbol("org.xxx", "xyz")));
    EXPECT_TRUE(create_symbol("org.xxx", "abc").is(create_symbol("org.xxx", "abc")));
    EXPECT_FALSE(create_symbol("com.z", "abc").is(create_symbol("org.xxx", "abc")));
}

TEST_F(value_test, should_store_keywords_with_namespaces)
{
    Value kw = create_keyword("org.xxx", "thing");
    ASSERT_EQ(tag::KEYWORD, get_value_tag(kw));
    auto ns = get_keyword_namespace(kw);
    ASSERT_EQ(tag::STRING, get_value_tag(ns));
    ASSERT_EQ("org.xxx", std::string(get_string_ptr(ns), get_string_len(ns)));
    auto name = get_keyword_name(kw);
    ASSERT_EQ(tag::STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_len(name)));
}

TEST_F(value_test, should_store_keywords_without_namespaces)
{
    Value kw = create_keyword("thing");
    ASSERT_EQ(tag::KEYWORD, get_value_tag(kw));
    ASSERT_TRUE(get_keyword_namespace(kw).is_nil());
    auto name = get_keyword_name(kw);
    ASSERT_EQ(tag::STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_len(name)));
}

TEST_F(value_test, should_return_same_instances_for_keywords_with_same_namespace_and_names)
{
    EXPECT_FALSE(create_keyword("abc").is(create_keyword("xyz")));
    EXPECT_TRUE(create_keyword("abc").is(create_keyword("abc")));
    EXPECT_FALSE(create_keyword("org.xxx", "abc").is(create_keyword("abc")));
    EXPECT_FALSE(create_keyword("org.xxx", "abc").is(create_keyword("org.xxx", "xyz")));
    EXPECT_TRUE(create_keyword("org.xxx", "abc").is(create_keyword("org.xxx", "abc")));
    EXPECT_FALSE(create_keyword("com.z", "abc").is(create_keyword("org.xxx", "abc")));
}

TEST_F(value_test, should_store_int_values)
{
    Root val{create_int64(7)};
    ASSERT_EQ(tag::INT64, get_value_tag(*val));
    ASSERT_EQ(7, get_int64_value(*val));
    val = create_int64(0);
    ASSERT_EQ(0, get_int64_value(*val));
    val = create_int64(-1);
    ASSERT_EQ(-1, get_int64_value(*val));
    val = create_int64(1);
    ASSERT_EQ(1, get_int64_value(*val));
    val = create_int64(std::numeric_limits<Int64>::min());
    ASSERT_EQ(std::numeric_limits<Int64>::min(), get_int64_value(*val));
    val = create_int64(std::numeric_limits<Int64>::max());
    ASSERT_EQ(std::numeric_limits<Int64>::max(), get_int64_value(*val));
}

TEST_F(value_test, should_store_float_values)
{
    Root val{create_float64(7.125)};
    ASSERT_EQ(tag::FLOAT64, get_value_tag(*val));
    ASSERT_EQ(7.125, get_float64_value(*val));
    val = create_float64(std::numeric_limits<Float64>::min());
    ASSERT_EQ(std::numeric_limits<Float64>::min(), get_float64_value(*val));
    val = create_float64(std::numeric_limits<Float64>::max());
    ASSERT_EQ(std::numeric_limits<Float64>::max(), get_float64_value(*val));
    val = create_float64(std::numeric_limits<Float64>::quiet_NaN());
    ASSERT_TRUE(std::isnan(get_float64_value(*val)));
}

TEST_F(value_test, should_store_string_values)
{
    std::string example("abc\0xyz", 7);
    std::string exampleCopy = example;
    Root val{create_string(exampleCopy)};
    exampleCopy.clear();
    ASSERT_EQ(tag::STRING, get_value_tag(*val));
    ASSERT_EQ(example, std::string(get_string_ptr(*val), get_string_len(*val)));
    ASSERT_STREQ(example.c_str(), get_string_ptr(*val));
}

TEST_F(value_test, should_null_terminate_strings)
{
    std::string example("abcxyz");
    Root val{create_string(example)};
    ASSERT_EQ(6u, std::strlen(get_string_ptr(*val)));
    ASSERT_STREQ(example.c_str(), get_string_ptr(*val));
}

TEST_F(value_test, should_create_a_new_instance_for_each_string)
{
    Root val{create_string("abc")};
    Root val2{create_string("abc")};
    ASSERT_FALSE(val->is(*val2));
}

TEST_F(value_test, should_store_object_values)
{
    Root type{create_object_type("org", "xxx")};
    Root elem0, elem1, elem2;
    elem0 = create_int64(10);
    elem1 = create_float64(20);
    elem2 = create_symbol("elem3");
    const std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    Root obj{create_object(*type, elems.data(), elems.size())};
    ASSERT_EQ(tag::OBJECT, get_value_tag(*obj));
    ASSERT_EQ(elems.size(), get_object_size(*obj));
    ASSERT_TRUE(elems[0].is(get_object_element(*obj, 0)));
    ASSERT_TRUE(elems[1].is(get_object_element(*obj, 1)));
    ASSERT_TRUE(elems[2].is(get_object_element(*obj, 2)));

    obj = create_object(*type, nullptr, 0);
    ASSERT_EQ(0u, get_object_size(*obj));
}

TEST_F(value_test, should_store_int_values_in_objects)
{
    Root type{create_object_type("org", "xxx")};
    Root elem0, elem1, elem2;
    elem0 = create_int64(10);
    elem1 = create_float64(20);
    elem2 = create_symbol("elem3");
    const std::array<Int64, 5> ints{{std::numeric_limits<Int64>::max(), std::numeric_limits<Int64>::min(), 0, 654, -1}};
    const std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    Root obj{create_object(*type, ints.data(), ints.size(), elems.data(), elems.size())};
    ASSERT_EQ(tag::OBJECT, get_value_tag(*obj));
    ASSERT_EQ(5u, get_object_int_size(*obj));
    ASSERT_EQ(std::numeric_limits<Int64>::max(), get_object_int(*obj, 0));
    ASSERT_EQ(std::numeric_limits<Int64>::min(), get_object_int(*obj, 1));
    ASSERT_EQ(0, get_object_int(*obj, 2));
    ASSERT_EQ(654, get_object_int(*obj, 3));
    ASSERT_EQ(-1, get_object_int(*obj, 4));
    ASSERT_EQ(elems.size(), get_object_size(*obj));
    ASSERT_TRUE(elems[0].is(get_object_element(*obj, 0)));
    ASSERT_TRUE(elems[1].is(get_object_element(*obj, 1)));
    ASSERT_TRUE(elems[2].is(get_object_element(*obj, 2)));

    obj = create_object(*type, nullptr, 0);
    ASSERT_EQ(0u, get_object_size(*obj));
}

TEST_F(value_test, should_initialize_object_values_to_nil_or_0)
{
    Root type{create_object_type("org", "xxx")};
    Root obj{create_object(*type, nullptr, 2, nullptr, 3)};
    ASSERT_EQ(tag::OBJECT, get_value_tag(*obj));
    ASSERT_EQ(2u, get_object_int_size(*obj));
    ASSERT_EQ(0, get_object_int(*obj, 0));
    ASSERT_EQ(0, get_object_int(*obj, 1));
    ASSERT_EQ(3u, get_object_size(*obj));
    ASSERT_TRUE(get_object_element(*obj, 0).is_nil());
    ASSERT_TRUE(get_object_element(*obj, 1).is_nil());
    ASSERT_TRUE(get_object_element(*obj, 2).is_nil());
}

TEST_F(value_test, should_modify_objects)
{
    Root type{create_object_type("org", "xxx")};
    Root elem0, elem1, elem2;
    elem0 = create_int64(10);
    elem1 = create_float64(20);
    elem2 = create_symbol("elem3");
    const std::array<Int64, 2> ints{{7, 8}};
    const std::array<Value, 2> elems{{*elem0, *elem1}};
    Root obj{create_object(*type, ints.data(), ints.size(), elems.data(), elems.size())};

    set_object_element(*obj, 0, *elem2);

    ASSERT_EQ(7, get_object_int(*obj, 0));
    ASSERT_EQ(8, get_object_int(*obj, 1));
    ASSERT_TRUE(elem2->is(get_object_element(*obj, 0)));
    ASSERT_TRUE(elem1->is(get_object_element(*obj, 1)));

    set_object_element(*obj, 1, *elem0);

    ASSERT_EQ(7, get_object_int(*obj, 0));
    ASSERT_EQ(8, get_object_int(*obj, 1));
    ASSERT_TRUE(elem2->is(get_object_element(*obj, 0)));
    ASSERT_TRUE(elem0->is(get_object_element(*obj, 1)));

    set_object_int(*obj, 0, 10);
    ASSERT_EQ(10, get_object_int(*obj, 0));
    ASSERT_EQ(8, get_object_int(*obj, 1));
    ASSERT_TRUE(elem2->is(get_object_element(*obj, 0)));
    ASSERT_TRUE(elem0->is(get_object_element(*obj, 1)));

    set_object_int(*obj, 1, 11);
    ASSERT_EQ(10, get_object_int(*obj, 0));
    ASSERT_EQ(11, get_object_int(*obj, 1));
    ASSERT_TRUE(elem2->is(get_object_element(*obj, 0)));
    ASSERT_TRUE(elem0->is(get_object_element(*obj, 1)));
}

TEST_F(value_test, should_create_a_new_instance_for_each_object)
{
    Root type{create_object_type("org", "xxx")};
    Root val{create_object(*type, nullptr, 0)};
    Root val2{create_object(*type, nullptr, 0)};
    ASSERT_FALSE(val->is(*val2));
}

TEST_F(value_test, should_return_the_type_of_a_value)
{
    auto f = [](const Value *, std::uint8_t) { return force(nil); };
    Root type{create_object_type("org", "xxx")};
    Root val;
    ASSERT_TRUE(get_value_type(nil).is_nil());
    val = create_native_function(f);
    ASSERT_TRUE(type::NativeFunction->is(get_value_type(*val)));
    val = create_symbol("abc");
    ASSERT_TRUE(type::Symbol->is(get_value_type(*val)));
    val = create_keyword("abc");
    ASSERT_TRUE(type::Keyword->is(get_value_type(*val)));
    val = create_int64(11);
    ASSERT_TRUE(type::Int64->is(get_value_type(*val)));
    val = create_float64(3.5);
    ASSERT_TRUE(type::Float64->is(get_value_type(*val)));
    val = create_string("abc");
    ASSERT_TRUE(type::String->is(get_value_type(*val)));
    val = create_object(*type, nullptr, 0);
    ASSERT_TRUE(type->is(get_value_type(*val)));
    val = create_object_type("some", "type");
    ASSERT_TRUE(type::Type->is(get_value_type(*val)));
}

TEST_F(value_test, should_create_types_with_fields)
{
    auto x = create_symbol("x");
    auto y = create_symbol("y");
    auto z = create_symbol("z");
    auto o = create_symbol("o");
    std::array<Value, 3> fields{{x, y, z}};
    Root type{create_object_type("some", "type", fields.data(), fields.size(), false, false)};
    EXPECT_EQ(0, get_object_field_index(*type, x));
    EXPECT_EQ(1, get_object_field_index(*type, y));
    EXPECT_EQ(2, get_object_field_index(*type, z));
    EXPECT_LT(get_object_field_index(*type, o), 0);
    EXPECT_EQ(3, get_object_type_field_count(*type));
    EXPECT_FALSE(is_object_type_constructible(*type));

    type = create_object_type("some", "type", fields.data(), fields.size(), false, true);
    EXPECT_EQ(0, get_object_field_index(*type, x));
    EXPECT_EQ(1, get_object_field_index(*type, y));
    EXPECT_EQ(2, get_object_field_index(*type, z));
    EXPECT_LT(get_object_field_index(*type, o), 0);
    EXPECT_EQ(3, get_object_type_field_count(*type));
    EXPECT_FALSE(is_object_type_constructible(*type));

    type = create_object_type("some", "type", nullptr, 0, true, false);
    EXPECT_LT(get_object_field_index(*type, x), 0);
    EXPECT_TRUE(is_object_type_constructible(*type));
}

}
}
