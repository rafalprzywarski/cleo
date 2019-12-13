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
    ASSERT_EQ(tag::UTF8STRING, get_value_tag(ns));
    ASSERT_EQ("org.xxx", std::string(get_string_ptr(ns), get_string_size(ns)));
    auto name = get_symbol_name(sym);
    ASSERT_EQ(tag::UTF8STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_size(name)));
}

TEST_F(value_test, should_store_symbols_without_namespaces)
{
    Value sym = create_symbol("thing");
    ASSERT_EQ(tag::SYMBOL, get_value_tag(sym));
    ASSERT_TRUE(get_symbol_namespace(sym).is_nil());
    auto name = get_symbol_name(sym);
    ASSERT_EQ(tag::UTF8STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_size(name)));
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
    ASSERT_EQ(tag::UTF8STRING, get_value_tag(ns));
    ASSERT_EQ("org.xxx", std::string(get_string_ptr(ns), get_string_size(ns)));
    auto name = get_keyword_name(kw);
    ASSERT_EQ(tag::UTF8STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_size(name)));
}

TEST_F(value_test, should_store_keywords_without_namespaces)
{
    Value kw = create_keyword("thing");
    ASSERT_EQ(tag::KEYWORD, get_value_tag(kw));
    ASSERT_TRUE(get_keyword_namespace(kw).is_nil());
    auto name = get_keyword_name(kw);
    ASSERT_EQ(tag::UTF8STRING, get_value_tag(name));
    ASSERT_EQ("thing", std::string(get_string_ptr(name), get_string_size(name)));
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

TEST_F(value_test, should_store_char_values)
{
    Value val{create_uchar(7)};
    ASSERT_EQ(tag::UCHAR, get_value_tag(val));
    ASSERT_EQ(7u, get_uchar_value(val));
    val = create_uchar(std::numeric_limits<Char32>::min());
    ASSERT_EQ(std::numeric_limits<Char32>::min(), get_uchar_value(val));
    val = create_uchar(std::numeric_limits<Char32>::max());
    ASSERT_EQ(std::numeric_limits<Char32>::max(), get_uchar_value(val));
}

TEST_F(value_test, should_store_string_values)
{
    std::string example("abc\0xyz", 7);
    std::string exampleCopy = example;
    Root val{create_string(exampleCopy)};
    exampleCopy.clear();
    ASSERT_EQ(tag::UTF8STRING, get_value_tag(*val));
    ASSERT_EQ(example, std::string(get_string_ptr(*val), get_string_size(*val)));
    ASSERT_STREQ(example.c_str(), get_string_ptr(*val));
}

TEST_F(value_test, should_null_terminate_strings)
{
    std::string example("abcxyz");
    Root val{create_string(example)};
    ASSERT_EQ(6u, std::strlen(get_string_ptr(*val)));
    ASSERT_STREQ(example.c_str(), get_string_ptr(*val));
}

TEST_F(value_test, should_replace_invalid_utf8_values_in_strings_with_U_FFFD)
{
    Root tmp;
    auto str = [&](const char *s)
                   {
                       tmp = create_string(s);
                       return std::string(get_string_ptr(*tmp), get_string_size(*tmp));
                   };
    std::string FFFD = "\xef\xbf\xbd";

    EXPECT_EQ("A\xc2\x80", str("A\xc2\x80")) << "valid";
    EXPECT_EQ("A\xdf\xbf", str("A\xdf\xbf")) << "valid";
    EXPECT_EQ("A\xe0\xa0\x80", str("A\xe0\xa0\x80")) << "valid";
    EXPECT_EQ("A\xef\xbf\xbf", str("A\xef\xbf\xbf")) << "valid";
    EXPECT_EQ("A\xf0\x90\x80\x80", str("A\xf0\x90\x80\x80")) << "valid";
    EXPECT_EQ("A\xf3\x90\x80\x80", str("A\xf3\x90\x80\x80")) << "valid";
    EXPECT_EQ("A\xf4\x8f\xbf\xbf", str("A\xf4\x8f\xbf\xbf")) << "valid";

    EXPECT_EQ(FFFD + "z", str("\xf5z")) << "invalid first byte";
    EXPECT_EQ(FFFD + "z", str("\xf6z")) << "invalid first byte";
    EXPECT_EQ(FFFD, str("\xf7")) << "invalid first byte";
    EXPECT_EQ(FFFD, str("\xff")) << "invalid first byte";

    EXPECT_EQ(FFFD, str("\x80")) << "invalid first byte";
    EXPECT_EQ(FFFD, str("\x81")) << "invalid first byte";
    EXPECT_EQ(FFFD + FFFD + FFFD, str("\x80\x82\x8f")) << "invalid bytes";
    EXPECT_EQ(FFFD, str("\xbf")) << "invalid first byte";
    EXPECT_EQ("AB" + FFFD, str("AB\xbf")) << "invalid first byte";

    EXPECT_EQ("A" + FFFD, str("A\xc2")) << "missing second byte";
    EXPECT_EQ("A" + FFFD + FFFD + "Z", str("A\xc2\xc0Z")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + "\x01Z", str("A\xc2\x01Z")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + "\x7fZ", str("A\xc2\x7fZ")) << "invalid second byte";
    EXPECT_EQ(FFFD + "\x7f", str("\xc2\x7f")) << "invalid second byte";

    EXPECT_EQ("A" + FFFD, str("A\xe0")) << "missing second byte";
    EXPECT_EQ(FFFD + FFFD, str("\xe0\xa0")) << "missing third byte";
    EXPECT_EQ("A" + FFFD + "\x70" + FFFD + "Z", str("A\xe0\x70\x80Z")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + FFFD + FFFD + "Z", str("A\xe0\xf7\x80Z")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + "\x03" + FFFD + "Z", str("A\xe0\x03\x80Z")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + FFFD + "\x70Z", str("A\xe0\xb0\x70Z")) << "invalid third byte";
    EXPECT_EQ("A" + FFFD + FFFD + FFFD + "Z", str("A\xe0\xb0\xf7Z")) << "invalid third byte";
    EXPECT_EQ("A" + FFFD + FFFD + "\x03Z", str("A\xe0\xb0\x03Z")) << "invalid third byte";

    EXPECT_EQ("A" + FFFD, str("A\xf0")) << "missing second byte";
    EXPECT_EQ("A" + FFFD + FFFD, str("A\xf0\x90")) << "missing third byte";
    EXPECT_EQ("A" + FFFD + FFFD + FFFD, str("A\xf0\x90\x80")) << "missing fourth byte";
    EXPECT_EQ("A" + FFFD + "\x70" + FFFD + FFFD, str("A\xf0\x70\x80\x80")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + FFFD + FFFD + FFFD + "X", str("A\xf0\xf0\x80\x80X")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + FFFD + "\x70" + FFFD + "X", str("A\xf0\x90\x70\x80X")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + FFFD + FFFD + FFFD, str("A\xf0\x90\xf0\x80")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + FFFD + FFFD + "\x70", str("A\xf0\x90\x80\x70")) << "invalid second byte";
    EXPECT_EQ("A" + FFFD + FFFD + FFFD + FFFD + "X", str("A\xf0\x90\x80\xf0X")) << "invalid second byte";

    EXPECT_EQ("A" + FFFD + "Z", str("A\xc0\x80Z")) << "overlong encoding";
    EXPECT_EQ("A" + FFFD + "Z", str("A\xc1\xbfZ")) << "overlong encoding";

    EXPECT_EQ("A" + FFFD + "Z", str("A\xe0\x80\x80Z")) << "overlong encoding";
    EXPECT_EQ("A" + FFFD + "Z", str("A\xe0\x9f\xbfZ")) << "overlong encoding";

    EXPECT_EQ("A" + FFFD + "Z", str("A\xf0\x80\x80\x80Z")) << "overlong encoding";
    EXPECT_EQ("A" + FFFD + "Z", str("A\xf0\x8f\xbf\xbfZ")) << "overlong encoding";

    EXPECT_EQ("A" + FFFD + "Z", str("A\xf4\x90\x80\x80Z")) << "0x110000, too large";
    EXPECT_EQ("A" + FFFD + FFFD + FFFD + FFFD + "Z", str("A\xf7\xbf\xbf\xbfZ")) << "0x1fffff, too large";
}

TEST_F(value_test, should_provide_offsets_of_code_points)
{
    Root val{create_string("\x73\x63\x53\x43\x33\x23\x13\x03")};
    ASSERT_EQ(1u, get_string_next_offset(*val, 0));
    ASSERT_EQ(2u, get_string_next_offset(*val, 1));
    ASSERT_EQ(3u, get_string_next_offset(*val, 2));
    ASSERT_EQ(4u, get_string_next_offset(*val, 3));
    ASSERT_EQ(5u, get_string_next_offset(*val, 4));
    ASSERT_EQ(6u, get_string_next_offset(*val, 5));
    ASSERT_EQ(7u, get_string_next_offset(*val, 6));
    ASSERT_EQ(8u, get_string_next_offset(*val, 7));
    val = create_string("\xc2\xa0Z\xdf\xbf");
    ASSERT_EQ(2u, get_string_next_offset(*val, 0));
    ASSERT_EQ(3u, get_string_next_offset(*val, 2));
    ASSERT_EQ(5u, get_string_next_offset(*val, 3));
    val = create_string("\xe0\xa0\x80N\xef\xbf\xbf");
    ASSERT_EQ(3u, get_string_next_offset(*val, 0));
    ASSERT_EQ(4u, get_string_next_offset(*val, 3));
    ASSERT_EQ(7u, get_string_next_offset(*val, 4));
    val = create_string("\xf0\x90\x80\x80X\xf4\x8f\xbf\xbf");
    ASSERT_EQ(4u, get_string_next_offset(*val, 0));
    ASSERT_EQ(5u, get_string_next_offset(*val, 4));
    ASSERT_EQ(9u, get_string_next_offset(*val, 5));
}

TEST_F(value_test, should_provide_code_points_at_given_offsets)
{
    Root val{create_string("\x32" "\x7f"
                           "\xc2\x80" "\xd5\xa2" "\xdf\xbf"
                           "\xe0\xa0\x80" "\xe5\x9e\x9e" "\xef\xbf\xbf"
                           "\xf0\x90\x80\x80" "\xf2\x97\x8c\xa4" "\xf4\x8f\xbf\xbf")};
    EXPECT_EQ(0x32u, get_string_char_at_offset(*val, 0));
    EXPECT_EQ(0x7fu, get_string_char_at_offset(*val, 1));
    EXPECT_EQ(0x80u, get_string_char_at_offset(*val, 2));
    EXPECT_EQ(0x562u, get_string_char_at_offset(*val, 4));
    EXPECT_EQ(0x7ffu, get_string_char_at_offset(*val, 6));
    EXPECT_EQ(0x800u, get_string_char_at_offset(*val, 8));
    EXPECT_EQ(0x579eu, get_string_char_at_offset(*val, 11));
    EXPECT_EQ(0xffffu, get_string_char_at_offset(*val, 14));
    EXPECT_EQ(0x10000u, get_string_char_at_offset(*val, 17));
    EXPECT_EQ(0x97324u, get_string_char_at_offset(*val, 21));
    EXPECT_EQ(0x10ffffu, get_string_char_at_offset(*val, 25));
}

TEST_F(value_test, should_provide_code_points_at_given_indices)
{
    Root val{create_string("\x32" "\x7f"
                           "\xc2\x80" "\xd5\xa2" "\xdf\xbf"
                           "\xe0\xa0\x80" "\xe5\x9e\x9e" "\xef\xbf\xbf"
                           "\xf0\x90\x80\x80" "\xf2\x97\x8c\xa4" "\xf4\x8f\xbf\xbf")};
    EXPECT_EQ(0x32u, get_string_char(*val, 0));
    EXPECT_EQ(0x7fu, get_string_char(*val, 1));
    EXPECT_EQ(0x80u, get_string_char(*val, 2));
    EXPECT_EQ(0x562u, get_string_char(*val, 3));
    EXPECT_EQ(0x7ffu, get_string_char(*val, 4));
    EXPECT_EQ(0x800u, get_string_char(*val, 5));
    EXPECT_EQ(0x579eu, get_string_char(*val, 6));
    EXPECT_EQ(0xffffu, get_string_char(*val, 7));
    EXPECT_EQ(0x10000u, get_string_char(*val, 8));
    EXPECT_EQ(0x97324u, get_string_char(*val, 9));
    EXPECT_EQ(0x10ffffu, get_string_char(*val, 10));
}

TEST_F(value_test, should_provide_string_length)
{
    Root val{create_string("\x73\x63\x53\x43\x33\x23\x13\x03")};
    EXPECT_EQ(8u, get_string_len(*val));
    val = create_string("\xc2\xa0Z\xdf\xbf");
    EXPECT_EQ(3u, get_string_len(*val));
    val = create_string("\xe0\xa0\x80N\xef\xbf\xbf");
    EXPECT_EQ(3u, get_string_len(*val));
    val = create_string("\xf0\x90\x80\x80X\xf4\x8f\xbf\xbf");
    EXPECT_EQ(3u, get_string_len(*val));
}

TEST_F(value_test, should_create_a_new_instance_for_each_string)
{
    Root val{create_string("abc")};
    Root val2{create_string("abc")};
    ASSERT_FALSE(val->is(*val2));
}

TEST_F(value_test, should_store_object_values)
{
    Root type{create_dynamic_object_type("org", "xxx")};
    Root elem0, elem1, elem2;
    elem0 = create_int64(10);
    elem1 = create_float64(20);
    elem2 = create_symbol("elem3");
    const std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    Root obj{create_object(*type, elems.data(), elems.size())};
    ASSERT_EQ(tag::OBJECT, get_value_tag(*obj));
    ASSERT_EQ(elems.size(), get_dynamic_object_size(*obj));
    ASSERT_TRUE(elems[0].is(get_dynamic_object_element(*obj, 0)));
    ASSERT_TRUE(elems[1].is(get_dynamic_object_element(*obj, 1)));
    ASSERT_TRUE(elems[2].is(get_dynamic_object_element(*obj, 2)));

    obj = create_object(*type, nullptr, 0);
    ASSERT_EQ(0u, get_dynamic_object_size(*obj));
}

TEST_F(value_test, should_store_int_values_in_dynamic_objects)
{
    Root type{create_dynamic_object_type("org", "xxx")};
    Root elem0, elem1, elem2;
    elem0 = create_int64(10);
    elem1 = create_float64(20);
    elem2 = create_symbol("elem3");
    const std::array<Int64, 5> ints{{std::numeric_limits<Int64>::max(), std::numeric_limits<Int64>::min(), 0, 654, -1}};
    const std::array<Value, 3> elems{{*elem0, *elem1, *elem2}};
    Root obj{create_object(*type, ints.data(), ints.size(), elems.data(), elems.size())};
    ASSERT_EQ(tag::OBJECT, get_value_tag(*obj));
    ASSERT_EQ(5u, get_dynamic_object_int_size(*obj));
    ASSERT_EQ(std::numeric_limits<Int64>::max(), get_dynamic_object_int(*obj, 0));
    ASSERT_EQ(std::numeric_limits<Int64>::min(), get_dynamic_object_int(*obj, 1));
    ASSERT_EQ(0, get_dynamic_object_int(*obj, 2));
    ASSERT_EQ(654, get_dynamic_object_int(*obj, 3));
    ASSERT_EQ(-1, get_dynamic_object_int(*obj, 4));
    ASSERT_EQ(elems.size(), get_dynamic_object_size(*obj));
    ASSERT_TRUE(elems[0].is(get_dynamic_object_element(*obj, 0)));
    ASSERT_TRUE(elems[1].is(get_dynamic_object_element(*obj, 1)));
    ASSERT_TRUE(elems[2].is(get_dynamic_object_element(*obj, 2)));

    obj = create_object(*type, nullptr, 0);
    ASSERT_EQ(0u, get_dynamic_object_size(*obj));
}

TEST_F(value_test, should_store_int_and_ptr_values_in_static_objects)
{
    std::array<Value, 4> names{{create_symbol("n"), create_symbol("i"), create_symbol("f"), create_symbol("l")}};
    std::array<Value, 4> types{{nil, type::Int64, *type::Float64, *type::List}};
    Root type{create_static_object_type("org", "xxx", names.data(), types.data(), names.size())};
    Root elem0, elem1, elem2, elem3;
    elem0 = create_int64(7);
    elem1 = create_int64(10);
    elem2 = create_float64(20);
    elem3 = list();
    const std::array<Value, 4> elems{{*elem0, *elem1, *elem2, *elem3}};
    Root obj{create_object(*type, elems.data(), elems.size())};
    ASSERT_EQ(tag::OBJECT, get_value_tag(*obj));
    ASSERT_EQ(4u, get_static_object_size(*obj));
    ASSERT_TRUE(elems[0].is(get_static_object_element(*obj, 0)));
    ASSERT_EQ(10, get_static_object_int(*obj, 1));
    ASSERT_TRUE(elems[2].is(get_static_object_element(*obj, 2)));
    ASSERT_TRUE(elems[3].is(get_static_object_element(*obj, 3)));
}

TEST_F(value_test, should_initialize_dynamic_object_values_to_nil_or_0)
{
    Root type{create_dynamic_object_type("org", "xxx")};
    Root obj{create_object(*type, nullptr, 2, nullptr, 3)};
    ASSERT_EQ(tag::OBJECT, get_value_tag(*obj));
    ASSERT_EQ(2u, get_dynamic_object_int_size(*obj));
    ASSERT_EQ(0, get_dynamic_object_int(*obj, 0));
    ASSERT_EQ(0, get_dynamic_object_int(*obj, 1));
    ASSERT_EQ(3u, get_dynamic_object_size(*obj));
    ASSERT_TRUE(get_dynamic_object_element(*obj, 0).is_nil());
    ASSERT_TRUE(get_dynamic_object_element(*obj, 1).is_nil());
    ASSERT_TRUE(get_dynamic_object_element(*obj, 2).is_nil());
}

TEST_F(value_test, should_initialize_static_object_values_to_nil_or_0)
{
    std::array<Value, 4> names{{create_symbol("n"), create_symbol("i"), create_symbol("f"), create_symbol("l")}};
    std::array<Value, 4> types{{nil, type::Int64, *type::Float64, *type::List}};
    Root type{create_static_object_type("org", "xxx", names.data(), types.data(), names.size())};
    Root obj{create_object(*type, nullptr, 4)};
    ASSERT_EQ(tag::OBJECT, get_value_tag(*obj));
    ASSERT_EQ(4u, get_static_object_size(*obj));
    ASSERT_TRUE(get_static_object_element(*obj, 0).is_nil());
    ASSERT_EQ(0, get_static_object_int(*obj, 1));
    ASSERT_TRUE(get_static_object_element(*obj, 2).is_nil());
    ASSERT_TRUE(get_static_object_element(*obj, 3).is_nil());
}

TEST_F(value_test, should_modify_static_objects)
{
    std::array<Value, 4> names{{create_symbol("n"), create_symbol("i"), create_symbol("f"), create_symbol("l")}};
    std::array<Value, 4> types{{nil, type::Int64, *type::Float64, *type::List}};
    Root type{create_static_object_type("org", "xxx", names.data(), types.data(), names.size())};
    Root elem0, elem1, elem2, elem3;
    elem0 = create_int64(7);
    elem1 = create_int64(10);
    elem2 = create_float64(20);
    elem3 = list();
    const std::array<Value, 4> elems{{*elem0, *elem1, *elem2, *elem3}};
    Root obj{create_object(*type, elems.data(), elems.size())};

    set_static_object_element(*obj, 0, *elem1);

    EXPECT_TRUE(elems[1].is(get_static_object_element(*obj, 0)));
    EXPECT_EQ(10, get_static_object_int(*obj, 1));
    EXPECT_TRUE(elems[2].is(get_static_object_element(*obj, 2)));
    EXPECT_TRUE(elems[3].is(get_static_object_element(*obj, 3)));

    set_static_object_element(*obj, 1, *elem0);

    EXPECT_TRUE(elems[1].is(get_static_object_element(*obj, 0)));
    EXPECT_EQ(7, get_static_object_int(*obj, 1));
    EXPECT_TRUE(elems[2].is(get_static_object_element(*obj, 2)));
    EXPECT_TRUE(elems[3].is(get_static_object_element(*obj, 3)));

    set_static_object_int(*obj, 1, 13);

    EXPECT_TRUE(elems[1].is(get_static_object_element(*obj, 0)));
    EXPECT_EQ(13, get_static_object_int(*obj, 1));
    EXPECT_TRUE(elems[2].is(get_static_object_element(*obj, 2)));
    EXPECT_TRUE(elems[3].is(get_static_object_element(*obj, 3)));

    set_static_object_element(*obj, 2, *elem3);

    EXPECT_TRUE(elems[1].is(get_static_object_element(*obj, 0)));
    EXPECT_EQ(13, get_static_object_int(*obj, 1));
    EXPECT_TRUE(elems[3].is(get_static_object_element(*obj, 2)));
    EXPECT_TRUE(elems[3].is(get_static_object_element(*obj, 3)));

    set_static_object_element(*obj, 3, *elem2);

    EXPECT_TRUE(elems[1].is(get_static_object_element(*obj, 0)));
    EXPECT_EQ(13, get_static_object_int(*obj, 1));
    EXPECT_TRUE(elems[3].is(get_static_object_element(*obj, 2)));
    EXPECT_TRUE(elems[2].is(get_static_object_element(*obj, 3)));
}

TEST_F(value_test, should_modify_dynamic_objects)
{
    Root type{create_dynamic_object_type("org", "xxx")};
    Root elem0, elem1, elem2;
    elem0 = create_int64(10);
    elem1 = create_float64(20);
    elem2 = create_symbol("elem3");
    const std::array<Int64, 2> ints{{7, 8}};
    const std::array<Value, 2> elems{{*elem0, *elem1}};
    Root obj{create_object(*type, ints.data(), ints.size(), elems.data(), elems.size())};

    set_dynamic_object_element(*obj, 0, *elem2);

    ASSERT_EQ(7, get_dynamic_object_int(*obj, 0));
    ASSERT_EQ(8, get_dynamic_object_int(*obj, 1));
    ASSERT_TRUE(elem2->is(get_dynamic_object_element(*obj, 0)));
    ASSERT_TRUE(elem1->is(get_dynamic_object_element(*obj, 1)));

    set_dynamic_object_element(*obj, 1, *elem0);

    ASSERT_EQ(7, get_dynamic_object_int(*obj, 0));
    ASSERT_EQ(8, get_dynamic_object_int(*obj, 1));
    ASSERT_TRUE(elem2->is(get_dynamic_object_element(*obj, 0)));
    ASSERT_TRUE(elem0->is(get_dynamic_object_element(*obj, 1)));

    set_dynamic_object_int(*obj, 0, 10);
    ASSERT_EQ(10, get_dynamic_object_int(*obj, 0));
    ASSERT_EQ(8, get_dynamic_object_int(*obj, 1));
    ASSERT_TRUE(elem2->is(get_dynamic_object_element(*obj, 0)));
    ASSERT_TRUE(elem0->is(get_dynamic_object_element(*obj, 1)));

    set_dynamic_object_int(*obj, 1, 11);
    ASSERT_EQ(10, get_dynamic_object_int(*obj, 0));
    ASSERT_EQ(11, get_dynamic_object_int(*obj, 1));
    ASSERT_TRUE(elem2->is(get_dynamic_object_element(*obj, 0)));
    ASSERT_TRUE(elem0->is(get_dynamic_object_element(*obj, 1)));
}

TEST_F(value_test, should_create_a_new_instance_for_each_object)
{
    Root type{create_dynamic_object_type("org", "xxx")};
    Root val{create_object(*type, nullptr, 0)};
    Root val2{create_object(*type, nullptr, 0)};
    ASSERT_FALSE(val->is(*val2));
}

TEST_F(value_test, should_return_the_type_of_a_value)
{
    auto f = [](const Value *, std::uint8_t) { return force(nil); };
    Root dtype{create_dynamic_object_type("org", "xxx")};
    Root stype{create_static_object_type("org", "xxx", nullptr, nullptr, 0)};
    Root p{create_protocol("org", "abc")};
    Root val;
    ASSERT_TRUE(get_value_type(nil).is_nil());
    val = create_native_function(f);
    ASSERT_TRUE(type::NativeFunction->is(get_value_type(*val)));
    val = create_symbol("abc");
    ASSERT_TRUE(type::Symbol->is(get_value_type(*val)));
    val = create_keyword("abc");
    ASSERT_TRUE(type::Keyword->is(get_value_type(*val)));
    val = create_int64(11);
    ASSERT_TRUE(type::Int64.is(get_value_type(*val)));
    val = create_uchar(48);
    ASSERT_TRUE(type::UChar->is(get_value_type(*val)));
    val = create_float64(3.5);
    ASSERT_TRUE(type::Float64->is(get_value_type(*val)));
    val = create_string("abc");
    ASSERT_TRUE(type::UTF8String->is(get_value_type(*val)));
    val = create_object(*dtype, nullptr, 0);
    ASSERT_TRUE(dtype->is(get_value_type(*val)));
    val = create_object(*stype, nullptr, 0);
    ASSERT_TRUE(stype->is(get_value_type(*val)));
    val = create_dynamic_object_type("some", "type");
    ASSERT_TRUE(type::Type->is(get_value_type(*val)));
    ASSERT_TRUE(type::Protocol->is(get_value_type(*p)));
}

TEST_F(value_test, should_create_types_with_fields)
{
    auto x = create_symbol("x");
    auto y = create_symbol("y");
    auto z = create_symbol("z");
    std::array<Value, 3> fields{{x, y, z}};
    Root type{create_object_type("some", "type", fields.data(), nullptr, fields.size(), false, false)};
    EXPECT_EQ(0, get_object_field_index(*type, x));
    EXPECT_EQ(1, get_object_field_index(*type, y));
    EXPECT_EQ(2, get_object_field_index(*type, z));
    EXPECT_EQ(3, get_object_type_field_count(*type));
    EXPECT_FALSE(is_object_type_constructible(*type));

    type = create_object_type("some", "type", fields.data(), nullptr, fields.size(), false, true);
    EXPECT_EQ(0, get_object_field_index(*type, x));
    EXPECT_EQ(1, get_object_field_index(*type, y));
    EXPECT_EQ(2, get_object_field_index(*type, z));
    EXPECT_EQ(3, get_object_type_field_count(*type));
    EXPECT_FALSE(is_object_type_constructible(*type));

    type = create_object_type("some", "type", nullptr, nullptr, 0, true, false);
    EXPECT_TRUE(is_object_type_constructible(*type));
}

TEST_F(value_test, get_object_field_index_should_return_a_negative_value_when_a_field_is_not_found)
{
    auto x = create_symbol("x");
    auto y = create_symbol("y");
    auto z = create_symbol("z");
    auto o = create_symbol("o");
    std::array<Value, 3> fields{{x, y, z}};
    Root type{create_object_type("some", "type", fields.data(), nullptr, fields.size(), false, false)};
    EXPECT_LT(get_object_field_index(*type, o), 0);

    type = create_object_type("some", "type", fields.data(), nullptr, fields.size(), false, true);
    EXPECT_LT(get_object_field_index(*type, o), 0);

    type = create_object_type("some", "type", nullptr, nullptr, 0, true, false);
    EXPECT_LT(get_object_field_index(*type, x), 0);

    EXPECT_LT(get_object_field_index(nil, x), 0);
}

TEST_F(value_test, should_create_protocols)
{
    auto name = create_symbol("value.test", "X");
    Root p{create_protocol(name)};
    ASSERT_TRUE(get_protocol_name(*p).is(name));
}

}
}
