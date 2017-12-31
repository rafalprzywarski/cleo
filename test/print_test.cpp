#include <cleo/print.hpp>
#include <cleo/global.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct pr_str_test : Test
{
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
    Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); })};
    std::ostringstream os;
    os << std::hex << *fn;
    EXPECT_EQ("#cleo.core/NativeFunction[0x" + os.str() + "]", str(pr_str(*fn)));
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
}

TEST_F(pr_str_test, should_print_objects)
{
    Root obj{create_object0(create_symbol("somewhere", "something"))};
    std::ostringstream os;
    os << std::hex << *obj;
    EXPECT_EQ("#somewhere/something[0x" + os.str() + "]", str(pr_str(*obj)));
}

TEST_F(pr_str_test, should_print_vectors)
{
    Root val;
    val = svec();
    EXPECT_EQ("[]", str(pr_str(*val)));
    val = svec(nil);
    EXPECT_EQ("[nil]", str(pr_str(*val)));
    Root elem0, elem1, elem2;
    elem0 = i64(1);
    elem1 = i64(2);
    elem2 = i64(3);
    val = svec(*elem0, *elem1, *elem2);
    EXPECT_EQ("[1 2 3]", str(pr_str(*val)));
    elem0 = i64(1);
    elem1 = i64(2);
    elem2 = i64(3);
    elem2 = svec(*elem2);
    elem1 = svec(*elem1, *elem2);
    val = svec(*elem0, *elem1);
    EXPECT_EQ("[1 [2 [3]]]", str(pr_str(*val)));
}

TEST_F(pr_str_test, should_print_maps)
{
    Root val{smap()};
    EXPECT_EQ("{}", str(pr_str(*val)));
    val = smap(nil, nil);
    EXPECT_EQ("{nil nil}", str(pr_str(*val)));
    auto a = create_keyword("a");
    auto b = create_keyword("b");
    auto c = create_keyword("c");
    val = smap(a, b, c, 20, 30, 40);
    EXPECT_EQ("{30 40, :c 20, :a :b}", str(pr_str(*val)));
}

TEST_F(pr_str_test, should_print_sets)
{
    Root val{sset()};
    EXPECT_EQ("#{}", str(pr_str(*val)));
    val = sset(nil, nil);
    EXPECT_EQ("#{nil}", str(pr_str(*val)));
    auto a = create_keyword("a");
    auto b = create_keyword("b");
    auto c = create_keyword("c");
    val = sset(a, b, c, 20, 30, 40);
    EXPECT_EQ("#{:a :b :c 20 30 40}", str(pr_str(*val)));
}

TEST_F(pr_str_test, should_print_sequences)
{
    Root val;
    val = list();
    EXPECT_EQ("()", str(pr_str(*val)));
    val = svec(nil);
    val = small_vector_seq(*val);
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
    val = svec(*elem0, *elem1);
    val = small_vector_seq(*val);
    EXPECT_EQ("(1 (2 (3)))", str(pr_str(*val)));
}

}
}
