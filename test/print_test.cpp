#include <cleo/print.hpp>
#include <cleo/global.hpp>
#include <gmock/gmock.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

using namespace testing;

struct pr_str_test : Test
{
    pr_str_test() : Test("cleo.pr-str.test") { }

    static std::string str(Force f)
    {
        Root val{f};
        return get_value_tag(*val) == tag::UTF8STRING ?
            std::string{get_string_ptr(*val), get_string_len(*val)} :
            "## invalid type ##";
    }
};

TEST_F(pr_str_test, should_print_nil)
{
    EXPECT_EQ("nil", str(pr_str(nil)));
}

TEST_F(pr_str_test, should_print_integers)
{
    Root val;
    val = create_int64(-15442);
    EXPECT_EQ("-15442", str(pr_str(*val)));
}

TEST_F(pr_str_test, should_print_chars)
{
    EXPECT_EQ("\\backspace", str(pr_str(create_uchar(8))));
    EXPECT_EQ("\\tab", str(pr_str(create_uchar(9))));
    EXPECT_EQ("\\newline", str(pr_str(create_uchar(10))));
    EXPECT_EQ("\\formfeed", str(pr_str(create_uchar(12))));
    EXPECT_EQ("\\return", str(pr_str(create_uchar(13))));
    EXPECT_EQ("\\space", str(pr_str(create_uchar(32))));

    EXPECT_EQ("\\a", str(pr_str(create_uchar('a'))));
    EXPECT_EQ("\\b", str(pr_str(create_uchar('b'))));
    EXPECT_EQ("\\y", str(pr_str(create_uchar('y'))));
    EXPECT_EQ("\\z", str(pr_str(create_uchar('z'))));
    EXPECT_EQ("\\A", str(pr_str(create_uchar('A'))));
    EXPECT_EQ("\\Z", str(pr_str(create_uchar('Z'))));
    EXPECT_EQ("\\7", str(pr_str(create_uchar('7'))));
    EXPECT_EQ("\\$", str(pr_str(create_uchar('$'))));
    EXPECT_EQ("\\!", str(pr_str(create_uchar(33))));
    EXPECT_EQ("\\~", str(pr_str(create_uchar(126))));

    EXPECT_EQ("\\u00", str(pr_str(create_uchar(0))));
    EXPECT_EQ("\\u01", str(pr_str(create_uchar(1))));
    EXPECT_EQ("\\u0f", str(pr_str(create_uchar(15))));
    EXPECT_EQ("\\u1f", str(pr_str(create_uchar(31))));
    EXPECT_EQ("\\u7f", str(pr_str(create_uchar(127))));
    EXPECT_EQ("\\uff", str(pr_str(create_uchar(255))));

    EXPECT_EQ("\\u0100", str(pr_str(create_uchar(0x100))));
    EXPECT_EQ("\\u1234", str(pr_str(create_uchar(0x1234))));
    EXPECT_EQ("\\uffff", str(pr_str(create_uchar(0xffff))));

    EXPECT_EQ("\\u010000", str(pr_str(create_uchar(0x10000))));
    EXPECT_EQ("\\u123456", str(pr_str(create_uchar(0x123456))));
    EXPECT_EQ("\\uffffff", str(pr_str(create_uchar(0xffffff))));
}

TEST_F(pr_str_test, should_print_floats)
{
    Root val;
    val = create_float64(-3.125);
    EXPECT_EQ("-3.125", str(pr_str(*val)));
    val = create_float64(0);
    EXPECT_EQ("0.0", str(pr_str(*val)));
    val = create_float64(-3);
    EXPECT_EQ("-3.0", str(pr_str(*val)));
    val = create_float64(-30);
    EXPECT_EQ("-30.0", str(pr_str(*val)));
    val = create_float64(7e+32);
    EXPECT_EQ("7e+32", str(pr_str(*val)));
    val = create_float64(7.25e+32);
    EXPECT_EQ("7.25e+32", str(pr_str(*val)));
}

TEST_F(pr_str_test, should_print_keywords)
{
    EXPECT_EQ(":abc", str(pr_str(create_keyword("abc"))));
    EXPECT_EQ(":ns/abc", str(pr_str(create_keyword("ns", "abc"))));
}

TEST_F(pr_str_test, should_print_symbols)
{
    EXPECT_EQ("abc", str(pr_str(create_symbol("abc"))));
    EXPECT_EQ("ns/abc", str(pr_str(create_symbol("ns", "abc"))));
}

TEST_F(pr_str_test, should_print_native_functions)
{
    {
        Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); })};
        std::ostringstream os;
        os << std::hex << fn->bits();
        EXPECT_EQ("#cleo.core/NativeFunction[nil 0x" + os.str() + "]", str(pr_str(*fn)));
    }
    {
        Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); }, create_symbol("abc"))};
        std::ostringstream os;
        os << std::hex << fn->bits();
        EXPECT_EQ("#cleo.core/NativeFunction[abc 0x" + os.str() + "]", str(pr_str(*fn)));
    }
}

TEST_F(pr_str_test, should_print_strings)
{
    Root val;
    val = create_string("");
    EXPECT_EQ("\"\"", str(pr_str(*val)));
    val = create_string("abc");
    EXPECT_EQ("\"abc\"", str(pr_str(*val)));
    val = create_string("\nabc\rdef\t\n");
    EXPECT_EQ("\"\\nabc\\rdef\\t\\n\"", str(pr_str(*val)));
    val = create_string("\"x\\\'y\\");
    EXPECT_EQ("\"\\\"x\\\\\\\'y\\\\\"", str(pr_str(*val)));
    val = create_string(std::string("\x7f\x80\xff\x97\x01\0\x1f", 7));
    EXPECT_EQ("\"\\x7f\\x80\\xff\\x97\\x01\\0\\x1f\"", str(pr_str(*val)));
}

TEST_F(pr_str_test, should_print_objects)
{
    Root t{create_dynamic_object_type("somewhere", "something")};
    Root obj{create_object0(*t)};
    std::ostringstream os;
    os << std::hex << obj->bits();
    EXPECT_EQ("#somewhere/something[0x" + os.str() + "]", str(pr_str(*obj)));
}

TEST_F(pr_str_test, should_print_types)
{
    Root t{create_dynamic_object_type("somewhere", "something")};
    std::ostringstream os;
    EXPECT_EQ("somewhere/something", str(pr_str(*t)));
}

TEST_F(pr_str_test, should_print_vectors)
{
    Root val;
    val = array();
    EXPECT_EQ("[]", str(pr_str(*val)));
    val = array(nil);
    EXPECT_EQ("[nil]", str(pr_str(*val)));
    Root elem0, elem1, elem2;
    elem0 = i64(1);
    elem1 = i64(2);
    elem2 = i64(3);
    val = array(*elem0, *elem1, *elem2);
    EXPECT_EQ("[1 2 3]", str(pr_str(*val)));
    elem0 = i64(1);
    elem1 = i64(2);
    elem2 = i64(3);
    elem2 = array(*elem2);
    elem1 = array(*elem1, *elem2);
    val = array(*elem0, *elem1);
    EXPECT_EQ("[1 [2 [3]]]", str(pr_str(*val)));
}

TEST_F(pr_str_test, should_print_array_maps)
{
    Root val{amap()};
    EXPECT_EQ("{}", str(pr_str(*val)));
    val = amap(nil, nil);
    EXPECT_EQ("{nil nil}", str(pr_str(*val)));
    auto a = create_keyword("a");
    auto b = create_keyword("b");
    auto c = create_keyword("c");
    val = amap(a, b, c, 20, 30, 40);
    EXPECT_EQ("{30 40, :c 20, :a :b}", str(pr_str(*val)));
}

TEST_F(pr_str_test, should_print_persistent_hash_maps)
{
    Root val{phmap()};
    EXPECT_EQ("{}", str(pr_str(*val)));
    val = phmap(nil, nil);
    EXPECT_EQ("{nil nil}", str(pr_str(*val)));
    auto a = create_keyword("a");
    auto b = create_keyword("b");
    auto c = create_keyword("c");
    val = phmap(a, b, c, 20, 30, 40);
    EXPECT_THAT(str(pr_str(*val)), AnyOf(
        "{30 40, :c 20, :a :b}",
        "{30 40, :a :b, :c 20}",
        "{:c 20, :a :b, 30 40}",
        "{:c 20, 30 40, :a :b}",
        "{:a :b, 30 40, :c 20}",
        "{:a :b, :c 20, 30 40}"));
}

TEST_F(pr_str_test, should_print_sets)
{
    Root val{aset()};
    EXPECT_EQ("#{}", str(pr_str(*val)));
    val = aset(nil, nil);
    EXPECT_EQ("#{nil}", str(pr_str(*val)));
    auto a = create_keyword("a");
    auto b = create_keyword("b");
    auto c = create_keyword("c");
    val = aset(a, b, c, 20, 30, 40);
    EXPECT_EQ("#{:a :b :c 20 30 40}", str(pr_str(*val)));
}

TEST_F(pr_str_test, should_print_sequences)
{
    Root val;
    val = list();
    EXPECT_EQ("()", str(pr_str(*val)));
    val = array(nil);
    val = array_seq(*val);
    EXPECT_EQ("(nil)", str(pr_str(*val)));
    Root elem0, elem1, elem2;
    elem0 = i64(1);
    elem1 = i64(2);
    elem2 = i64(3);
    val = list(*elem0, *elem1, *elem2);
    EXPECT_EQ("(1 2 3)", str(pr_str(*val)));
    elem0 = i64(1);
    elem1 = i64(2);
    elem2 = i64(3);
    elem2 = list(*elem2);
    elem1 = list(*elem1, *elem2);
    val = array(*elem0, *elem1);
    val = array_seq(*val);
    EXPECT_EQ("(1 (2 (3)))", str(pr_str(*val)));
}

struct print_str_test : Test
{
    print_str_test() : Test("cleo.print-str.test") { }

    static std::string str(Force f)
    {
        Root val{f};
        return get_value_tag(*val) == tag::UTF8STRING ?
            std::string{get_string_ptr(*val), get_string_len(*val)} :
            "## invalid type ##";
    }
};

TEST_F(print_str_test, should_print_nil)
{
    EXPECT_EQ("nil", str(print_str(nil)));
}

TEST_F(print_str_test, should_print_integers)
{
    Root val;
    val = create_int64(-15442);
    EXPECT_EQ("-15442", str(print_str(*val)));
}

TEST_F(print_str_test, should_print_chars)
{
    EXPECT_EQ("\x08", str(print_str(create_uchar(8))));
    EXPECT_EQ("\x09", str(print_str(create_uchar(9))));
    EXPECT_EQ("\n", str(print_str(create_uchar(10))));
    EXPECT_EQ("\x0c", str(print_str(create_uchar(12))));
    EXPECT_EQ("\r", str(print_str(create_uchar(13))));
    EXPECT_EQ(" ", str(print_str(create_uchar(32))));

    EXPECT_EQ("a", str(print_str(create_uchar('a'))));
    EXPECT_EQ("b", str(print_str(create_uchar('b'))));
    EXPECT_EQ("y", str(print_str(create_uchar('y'))));
    EXPECT_EQ("z", str(print_str(create_uchar('z'))));
    EXPECT_EQ("A", str(print_str(create_uchar('A'))));
    EXPECT_EQ("Z", str(print_str(create_uchar('Z'))));
    EXPECT_EQ("7", str(print_str(create_uchar('7'))));
    EXPECT_EQ("$", str(print_str(create_uchar('$'))));
    EXPECT_EQ("!", str(print_str(create_uchar(33))));
    EXPECT_EQ("~", str(print_str(create_uchar(126))));

    EXPECT_EQ(std::string("\0", 1), str(print_str(create_uchar(0))));
    EXPECT_EQ("\x01", str(print_str(create_uchar(1))));
    EXPECT_EQ("\x0f", str(print_str(create_uchar(15))));
    EXPECT_EQ("\x1f", str(print_str(create_uchar(31))));
    EXPECT_EQ("\x7f", str(print_str(create_uchar(127))));
    EXPECT_EQ("\xc2\x80", str(print_str(create_uchar(0x80))));
    EXPECT_EQ("\xc2\x81", str(print_str(create_uchar(0x81))));
    EXPECT_EQ("\xc2\x82", str(print_str(create_uchar(0x82))));
    EXPECT_EQ("\xc2\x84", str(print_str(create_uchar(0x84))));
    EXPECT_EQ("\xc2\x88", str(print_str(create_uchar(0x88))));
    EXPECT_EQ("\xc2\x90", str(print_str(create_uchar(0x90))));
    EXPECT_EQ("\xc2\xa0", str(print_str(create_uchar(0xa0))));
    EXPECT_EQ("\xc3\x80", str(print_str(create_uchar(0xc0))));
    EXPECT_EQ("\xc4\x80", str(print_str(create_uchar(0x100))));
    EXPECT_EQ("\xc8\x80", str(print_str(create_uchar(0x200))));
    EXPECT_EQ("\xd0\x80", str(print_str(create_uchar(0x400))));
    EXPECT_EQ("\xdf\xbf", str(print_str(create_uchar(0x7ff))));
    EXPECT_EQ("\xe0\xa0\x80", str(print_str(create_uchar(0x800))));
    EXPECT_EQ("\xe0\xa0\x81", str(print_str(create_uchar(0x801))));
    EXPECT_EQ("\xe0\xa0\x82", str(print_str(create_uchar(0x802))));
    EXPECT_EQ("\xe0\xa0\x84", str(print_str(create_uchar(0x804))));
    EXPECT_EQ("\xe0\xa0\x88", str(print_str(create_uchar(0x808))));
    EXPECT_EQ("\xe0\xa0\x90", str(print_str(create_uchar(0x810))));
    EXPECT_EQ("\xe0\xa0\xa0", str(print_str(create_uchar(0x820))));
    EXPECT_EQ("\xe0\xa1\x80", str(print_str(create_uchar(0x840))));
    EXPECT_EQ("\xe0\xa2\x80", str(print_str(create_uchar(0x880))));
    EXPECT_EQ("\xe0\xa4\x80", str(print_str(create_uchar(0x900))));
    EXPECT_EQ("\xe0\xa8\x80", str(print_str(create_uchar(0xa00))));
    EXPECT_EQ("\xe0\xb0\x80", str(print_str(create_uchar(0xc00))));
    EXPECT_EQ("\xe1\x80\x80", str(print_str(create_uchar(0x1000))));
    EXPECT_EQ("\xe2\x80\x80", str(print_str(create_uchar(0x2000))));
    EXPECT_EQ("\xe4\x80\x80", str(print_str(create_uchar(0x4000))));
    EXPECT_EQ("\xe8\x80\x80", str(print_str(create_uchar(0x8000))));
    EXPECT_EQ("\xef\xbf\xbf", str(print_str(create_uchar(0xffff))));
    EXPECT_EQ("\xf0\x90\x80\x80", str(print_str(create_uchar(0x10000))));
    EXPECT_EQ("\xf0\x90\x80\x81", str(print_str(create_uchar(0x10001))));
    EXPECT_EQ("\xf0\x90\x80\x82", str(print_str(create_uchar(0x10002))));
    EXPECT_EQ("\xf0\x90\x80\x84", str(print_str(create_uchar(0x10004))));
    EXPECT_EQ("\xf0\x90\x80\x88", str(print_str(create_uchar(0x10008))));
    EXPECT_EQ("\xf0\x90\x80\x90", str(print_str(create_uchar(0x10010))));
    EXPECT_EQ("\xf0\x90\x80\xa0", str(print_str(create_uchar(0x10020))));
    EXPECT_EQ("\xf0\x90\x81\x80", str(print_str(create_uchar(0x10040))));
    EXPECT_EQ("\xf0\x90\x82\x80", str(print_str(create_uchar(0x10080))));
    EXPECT_EQ("\xf0\x90\x84\x80", str(print_str(create_uchar(0x10100))));
    EXPECT_EQ("\xf0\x90\x88\x80", str(print_str(create_uchar(0x10200))));
    EXPECT_EQ("\xf0\x90\x90\x80", str(print_str(create_uchar(0x10400))));
    EXPECT_EQ("\xf0\x90\xa0\x80", str(print_str(create_uchar(0x10800))));
    EXPECT_EQ("\xf0\x91\x80\x80", str(print_str(create_uchar(0x11000))));
    EXPECT_EQ("\xf0\x92\x80\x80", str(print_str(create_uchar(0x12000))));
    EXPECT_EQ("\xf0\x94\x80\x80", str(print_str(create_uchar(0x14000))));
    EXPECT_EQ("\xf0\x98\x80\x80", str(print_str(create_uchar(0x18000))));
    EXPECT_EQ("\xf0\xa0\x80\x80", str(print_str(create_uchar(0x20000))));
    EXPECT_EQ("\xf1\x80\x80\x80", str(print_str(create_uchar(0x40000))));
    EXPECT_EQ("\xf2\x80\x80\x80", str(print_str(create_uchar(0x80000))));
    EXPECT_EQ("\xf4\x80\x80\x80", str(print_str(create_uchar(0x100000))));
    EXPECT_EQ("\xf4\x8f\xbf\xbf", str(print_str(create_uchar(0x10ffff))));
}

TEST_F(print_str_test, should_print_floats)
{
    Root val;
    val = create_float64(-3.125);
    EXPECT_EQ("-3.125", str(print_str(*val)));
    val = create_float64(0);
    EXPECT_EQ("0.0", str(print_str(*val)));
    val = create_float64(-3);
    EXPECT_EQ("-3.0", str(print_str(*val)));
    val = create_float64(-30);
    EXPECT_EQ("-30.0", str(print_str(*val)));
    val = create_float64(7e+32);
    EXPECT_EQ("7e+32", str(print_str(*val)));
    val = create_float64(7.25e+32);
    EXPECT_EQ("7.25e+32", str(print_str(*val)));
}

TEST_F(print_str_test, should_print_keywords)
{
    EXPECT_EQ(":abc", str(print_str(create_keyword("abc"))));
    EXPECT_EQ(":ns/abc", str(print_str(create_keyword("ns", "abc"))));
}

TEST_F(print_str_test, should_print_symbols)
{
    EXPECT_EQ("abc", str(print_str(create_symbol("abc"))));
    EXPECT_EQ("ns/abc", str(print_str(create_symbol("ns", "abc"))));
}

TEST_F(print_str_test, should_print_native_functions)
{
    {
        Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); })};
        std::ostringstream os;
        os << std::hex << fn->bits();
        EXPECT_EQ("#cleo.core/NativeFunction[nil 0x" + os.str() + "]", str(print_str(*fn)));
    }
    {
        Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); }, create_symbol("abc"))};
        std::ostringstream os;
        os << std::hex << fn->bits();
        EXPECT_EQ("#cleo.core/NativeFunction[abc 0x" + os.str() + "]", str(print_str(*fn)));
    }
}

TEST_F(print_str_test, should_print_strings)
{
    Root val;
    val = create_string("");
    EXPECT_EQ("", str(print_str(*val)));
    val = create_string("abc");
    EXPECT_EQ("abc", str(print_str(*val)));
    val = create_string("\nabc\rdef\t\n");
    EXPECT_EQ("\nabc\rdef\t\n", str(print_str(*val)));
    val = create_string("\"x\\\'y\\");
    EXPECT_EQ("\"x\\\'y\\", str(print_str(*val)));
}

TEST_F(print_str_test, should_print_objects)
{
    Root t{create_dynamic_object_type("somewhere", "something")};
    Root obj{create_object0(*t)};
    std::ostringstream os;
    os << std::hex << obj->bits();
    EXPECT_EQ("#somewhere/something[0x" + os.str() + "]", str(print_str(*obj)));
}

TEST_F(print_str_test, should_print_vectors)
{
    Root val;
    val = array();
    EXPECT_EQ("[]", str(print_str(*val)));
    val = array(nil);
    EXPECT_EQ("[nil]", str(print_str(*val)));
    Root elem0, elem1, elem2;
    elem0 = i64(1);
    elem1 = i64(2);
    elem2 = create_string("3");
    val = array(*elem0, *elem1, *elem2);
    EXPECT_EQ("[1 2 3]", str(print_str(*val)));
    elem0 = i64(1);
    elem1 = create_string("2");
    elem2 = i64(3);
    elem2 = array(*elem2);
    elem1 = array(*elem1, *elem2);
    val = array(*elem0, *elem1);
    EXPECT_EQ("[1 [2 [3]]]", str(print_str(*val)));
}

TEST_F(print_str_test, should_print_maps)
{
    Root val{amap()};
    EXPECT_EQ("{}", str(print_str(*val)));
    val = amap(nil, nil);
    EXPECT_EQ("{nil nil}", str(print_str(*val)));
    auto a = create_keyword("a");
    auto b = create_keyword("b");
    auto c = create_keyword("c");
    val = amap(a, b, c, "20", "30", "40");
    EXPECT_EQ("{30 40, :c 20, :a :b}", str(print_str(*val)));
}

TEST_F(print_str_test, should_print_sets)
{
    Root val{aset()};
    EXPECT_EQ("#{}", str(print_str(*val)));
    val = aset(nil, nil);
    EXPECT_EQ("#{nil}", str(print_str(*val)));
    auto a = create_keyword("a");
    auto b = create_keyword("b");
    auto c = create_keyword("c");
    val = aset(a, b, c, "20", 30, 40);
    EXPECT_EQ("#{:a :b :c 20 30 40}", str(print_str(*val)));
}

TEST_F(print_str_test, should_print_sequences)
{
    Root val;
    val = list();
    EXPECT_EQ("()", str(print_str(*val)));
    val = array(nil);
    val = array_seq(*val);
    EXPECT_EQ("(nil)", str(print_str(*val)));
    Root elem0, elem1, elem2;
    elem0 = create_string("1");
    elem1 = i64(2);
    elem2 = i64(3);
    val = list(*elem0, *elem1, *elem2);
    EXPECT_EQ("(1 2 3)", str(print_str(*val)));
    elem0 = i64(1);
    elem1 = create_string("2");
    elem2 = i64(3);
    elem2 = list(*elem2);
    elem1 = list(*elem1, *elem2);
    val = array(*elem0, *elem1);
    val = array_seq(*val);
    EXPECT_EQ("(1 (2 (3)))", str(print_str(*val)));
}

}
}
