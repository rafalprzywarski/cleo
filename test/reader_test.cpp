#include <cleo/reader.hpp>
#include <cleo/equality.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

#define EXPECT_EQ_VALS(ex, val) \
    EXPECT_TRUE(nil != are_equal(ex, val)) << "expected: " << to_string(ex) << ", actual: " << to_string(val);

Force read_str(const std::string& s)
{
    Root sr{create_string(s)};
    return read(*sr);
}

void assert_read_error(const std::string& msg, const std::string& source)
{
    try
    {
        Root result{read_str(source)};
        FAIL() << "expected an exception; got " << to_string(*result) << " instead; source: " << source;
    }
    catch (reader_error const& e)
    {
        EXPECT_EQ(msg, e.what());
    }
    catch (std::exception const& e)
    {
        FAIL() << "unexpected exception with message: " << e.what();
    }
    catch (...)
    {
        FAIL() << "unknown exception";
    }
}

TEST(reader_test, should_parse_a_sequence_of_characters_as_an_symbol)
{
    Root val;
    val = read_str("abc123");
    EXPECT_EQ_VALS(create_symbol("abc123"), *val);
    val = read_str("nil0");
    EXPECT_EQ_VALS(create_symbol("nil0"), *val);
    val = read_str("true0");
    EXPECT_EQ_VALS(create_symbol("true0"), *val);
    val = read_str("false0");
    EXPECT_EQ_VALS(create_symbol("false0"), *val);
    val = read_str("+x");
    EXPECT_EQ_VALS(create_symbol("+x"), *val);
    val = read_str("-x");
    EXPECT_EQ_VALS(create_symbol("-x"), *val);
}

TEST(reader_test, should_not_treat_newline_as_part_of_symbol)
{
    Root val{read_str("abc123\n")};
    EXPECT_EQ(create_symbol("abc123"), *val) << to_string(*val);
}

TEST(reader_test, should_parse_a_sequence_of_digits_as_an_integer)
{
    Root ex, val;
    ex = create_int64(1); val = read_str("1");
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(23); val = read_str("23");
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(32134); val = read_str("32134");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_negative_integers)
{
    Root ex, val;
    ex = create_int64(-58); val = read_str("-58");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_an_empty_list)
{
    Root ex, val;
    ex = list();
    val = read_str("()");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("( )");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_a_list_of_expressions)
{
    Root ex, val;
    ex = list(1); val = read_str("(1)");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(create_symbol("+"), create_symbol("abc"), -3); val = read_str("(+ abc -3)");
    EXPECT_EQ_VALS(*ex, *val);
    Root l7{list(7)};
    ex = list(create_symbol("x"), -3);
    ex = list(*ex, 1);
    ex = list(*ex, *l7);
    val = read_str("(((x -3) 1) (7))");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(create_keyword("x")); val = read_str("(:x)");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_quoted_strings)
{
    Root ex, val;
    ex = create_string(""); val = read_str("\"\"");
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("  "); val = read_str("\"  \"");
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("abc123"); val = read_str("\"abc123\"");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list("a", "b"); val = read_str("(\"a\" \"b\")");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_not_treat_strings_as_parts_of_symbols)
{
    Root ex, val;
    ex = create_symbol("ab");
    ex = list(*ex, "cd"); val = read_str("(ab\"cd\")");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_escaped_newline_in_strings)
{
    Root ex, val;
    ex = create_string("ab\ncd"); val = read_str("\"ab\\ncd\"");
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\n\n\n"); val = read_str("\"\\n\\n\\n\"");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_escaped_backslashes_in_strings)
{
    Root ex, val;
    ex = create_string("ab\\cd"); val = read_str("\"ab\\\\cd\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\\\\\\"); val = read_str("\"\\\\\\\\\\\\\""); EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_escaped_quotes_in_strings)
{
    Root ex, val;
    ex = create_string("ab\"cd"); val = read_str("\"ab\\\"cd\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\"\"\""); val = read_str("\"\\\"\\\"\\\"\""); EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_a_sequence_of_characters_beginning_with_a_colon_as_a_keyword)
{
    Root val;
    val = read_str(":abc123");
    EXPECT_EQ_VALS(create_keyword("abc123"), *val);
    val = read_str(":nil0");
    EXPECT_EQ_VALS(create_keyword("nil0"), *val);
    val = read_str(":true0");
    EXPECT_EQ_VALS(create_keyword("true0"), *val);
    val = read_str(":false0");
    EXPECT_EQ_VALS(create_keyword("false0"), *val);
    val = read_str(":+x");
    EXPECT_EQ_VALS(create_keyword("+x"), *val);
    val = read_str(":-x");
    EXPECT_EQ_VALS(create_keyword("-x"), *val);
}

TEST(reader_test, should_parse_an_apostrophe_as_quote)
{
    auto quote = create_symbol("quote");
    Root ex, val;
    ex = list(quote, 7);
    val = read_str("'7");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("' 7");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(quote, create_symbol("abc123"));
    val = read_str("'abc123");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(1);
    ex = list(quote, *ex);
    val = read_str("'(1)");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("' (1)");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_an_empty_vector)
{
    Root ex, val;
    ex = svec();
    val = read_str("[]");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("[ ]");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_parse_a_vector_of_expressions)
{
    Root ex, val;
    ex = svec(1); val = read_str("[1]");
    EXPECT_EQ_VALS(*ex, *val);
    ex = svec(create_symbol("+"), create_symbol("abc"), -3); val = read_str("[+ abc -3]");
    EXPECT_EQ_VALS(*ex, *val);
    Root l7{svec(7)};
    ex = svec(create_symbol("x"), -3);
    ex = svec(*ex, 1);
    ex = svec(*ex, *l7);
    val = read_str("[[[x -3] 1] [7]]");
    EXPECT_EQ_VALS(*ex, *val);
    ex = svec(create_keyword("x")); val = read_str("[:x]");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_fail_when_parsing_an_unmatched_closing_bracket)
{
    assert_read_error("unexpected ]", "]");
}

TEST(reader_test, should_fail_when_parsing_an_unmatched_closing_paren)
{
    assert_read_error("unexpected )", ")");
}

TEST(reader_test, should_parse_nil)
{
    EXPECT_EQ(nil, *Root(read_str("nil")));
    EXPECT_EQ(nil, *Root(read_str("nil ")));
    Root ex, val;
    ex = list(nil); val = read_str("(nil)");
    EXPECT_EQ_VALS(*ex, *val);
    ex = svec(nil); val = read_str("[nil]");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST(reader_test, should_fail_when_missing_a_closing_paren)
{
    assert_read_error("unexpected end of input", "(");
    assert_read_error("unexpected end of input", "( 5 ");
    assert_read_error("unexpected end of input", "(()");
}

TEST(reader_test, should_fail_when_missing_a_closing_bracket)
{
    assert_read_error("unexpected end of input", "[");
    assert_read_error("unexpected end of input", "[ 5 ");
    assert_read_error("unexpected end of input", "[[]");
}

TEST(reader_test, should_fail_when_missing_a_closing_quote)
{
    assert_read_error("unexpected end of input", "\"");
}

TEST(reader_test, should_fail_when_invoked_with_something_else_than_a_string)
{
    ASSERT_ANY_THROW(read(create_symbol("abc")));
}


}
}