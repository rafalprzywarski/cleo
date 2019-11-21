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
        return get_value_tag(*val) == tag::STRING ?
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
    val = create_string(std::string("\x80\xff\x97\x01\0\x1f", 6));
    EXPECT_EQ("\"\\x80\\xff\\x97\\x01\\0\\x1f\"", str(pr_str(*val)));
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
        return get_value_tag(*val) == tag::STRING ?
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
