#include <cleo/print.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct pr_str_test : testing::Test
{
    std::string str(Value val)
    {
        return get_value_tag(val) == tag::STRING ?
            std::string{get_string_ptr(val), get_string_len(val)} :
            "## invalid type ##";
    }
};

TEST_F(pr_str_test, should_print_nil)
{
    EXPECT_EQ("nil", str(pr_str(nil)));
}

TEST_F(pr_str_test, should_print_integers)
{
    EXPECT_EQ("-15442", str(pr_str(create_int64(-15442))));
}

TEST_F(pr_str_test, should_print_floats)
{
    EXPECT_EQ("-3.125", str(pr_str(create_float64(-3.125))));
    EXPECT_EQ("0.0", str(pr_str(create_float64(0))));
    EXPECT_EQ("-3.0", str(pr_str(create_float64(-3))));
    EXPECT_EQ("-30.0", str(pr_str(create_float64(-30))));
    EXPECT_EQ("7e+32", str(pr_str(create_float64(7e+32))));
    EXPECT_EQ("7.25e+32", str(pr_str(create_float64(7.25e+32))));
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
    auto fn = create_native_function([](const Value *, std::uint8_t) { return nil; });
    std::ostringstream os;
    os << std::hex << fn;
    EXPECT_EQ("#cleo.core/NativeFunction[0x" + os.str() + "]", str(pr_str(fn)));
}

TEST_F(pr_str_test, should_print_strings)
{
    EXPECT_EQ("\"\"", str(pr_str(create_string(""))));
    EXPECT_EQ("\"abc\"", str(pr_str(create_string("abc"))));
    EXPECT_EQ("\"\\nabc\\rdef\\t\\n\"", str(pr_str(create_string("\nabc\rdef\t\n"))));
    EXPECT_EQ("\"\\\"x\\\\\\\'y\\\\\"", str(pr_str(create_string("\"x\\\'y\\"))));
}

TEST_F(pr_str_test, should_print_objects)
{
    auto obj = create_object0(create_symbol("somewhere", "something"));
    std::ostringstream os;
    os << std::hex << obj;
    EXPECT_EQ("#somewhere/something[0x" + os.str() + "]", str(pr_str(obj)));
}

TEST_F(pr_str_test, should_print_vectors)
{
    EXPECT_EQ("[]", str(pr_str(svec())));
    EXPECT_EQ("[nil]", str(pr_str(svec(nil))));
    EXPECT_EQ("[1 2 3]", str(pr_str(svec(i64(1), i64(2), i64(3)))));
    EXPECT_EQ("[1 [2 [3]]]", str(pr_str(svec(i64(1), svec(i64(2), svec(i64(3)))))));
}

TEST_F(pr_str_test, should_print_sequences)
{
    EXPECT_EQ("()", str(pr_str(list())));
    EXPECT_EQ("(nil)", str(pr_str(small_vector_seq(svec(nil)))));
    EXPECT_EQ("(1 2 3)", str(pr_str(list(i64(1), i64(2), i64(3)))));
    EXPECT_EQ("(1 (2 (3)))", str(pr_str(small_vector_seq(svec(i64(1), list(i64(2), list(i64(3))))))));
}

}
}
