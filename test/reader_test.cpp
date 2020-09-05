#include <cleo/reader.hpp>
#include <cleo/equality.hpp>
#include <cleo/multimethod.hpp>
#include <cleo/var.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct reader_test : Test
{
    reader_test() : Test("cleo.reader.test") { }

    static Force read_str(const std::string& s)
    {
        Root sr{create_string(s)};
        ReaderStream stream{*sr};
        return read(stream);
    }

    static void assert_read_error(Value exType, const std::string& msg, Int64 line, Int64 col, const std::string& source)
    {
        try
        {
            Root result{read_str(source)};
            FAIL() << "expected an exception; got " << to_string(*result) << " instead; source: " << source;
        }
        catch (Exception const& )
        {
            cleo::Root e{cleo::catch_exception()};
            EXPECT_TRUE(exType.is(get_value_type(*e)));
            Value exMsg = exception_message(*e);
            ASSERT_TRUE(get_value_type(exMsg).is(*type::UTF8String));
            EXPECT_EQ(msg, std::string(get_string_ptr(exMsg), get_string_size(exMsg)));
            if (isa(get_value_type(*e), *type::ReadError))
            {
                EXPECT_EQ(line, read_error_line(*e));
                EXPECT_EQ(col, read_error_column(*e));
            }
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

    static void assert_read_error(const std::string& msg, const std::string& source, std::uint32_t row, std::uint32_t col)
    {
        assert_read_error(*type::ReadError, msg, row, col, source);
    }

    static void assert_unexpected_end_of_input(const std::string& source, std::uint32_t row, std::uint32_t col)
    {
        assert_read_error(*type::UnexpectedEndOfInput, "unexpected end of input", row, col, source);
    }
};


TEST_F(reader_test, should_parse_a_sequence_of_characters_as_a_symbol)
{
    Root val;
    val = read_str("abc123");
    EXPECT_EQ_VALS(create_symbol("abc123"), *val);
    val = read_str("nil0");
    EXPECT_EQ_VALS(create_symbol("nil0"), *val);
    val = read_str("true0");
    EXPECT_EQ_VALS(create_symbol("true0"), *val);
    val = read_str("other/true");
    EXPECT_EQ_VALS(create_symbol("other", "true"), *val);
    val = read_str("false0");
    EXPECT_EQ_VALS(create_symbol("false0"), *val);
    val = read_str("+x");
    EXPECT_EQ_VALS(create_symbol("+x"), *val);
    val = read_str("-x");
    EXPECT_EQ_VALS(create_symbol("-x"), *val);
    val = read_str(".");
    EXPECT_EQ_VALS(create_symbol("."), *val);
    val = read_str("ab.cd");
    EXPECT_EQ_VALS(create_symbol("ab.cd"), *val);
    val = read_str("*<>=!?");
    EXPECT_EQ_VALS(create_symbol("*<>=!?"), *val);
    val = read_str("&");
    EXPECT_EQ_VALS(create_symbol("&"), *val);
    val = read_str("&abc");
    EXPECT_EQ_VALS(create_symbol("&abc"), *val);
    val = read_str("abc#");
    EXPECT_EQ_VALS(create_symbol("abc#"), *val);
    val = read_str("__abc__");
    EXPECT_EQ_VALS(create_symbol("__abc__"), *val);
}

TEST_F(reader_test, should_parse_symbols_with_namespaces)
{
    Root val;
    val = read_str("abc123/xyz");
    EXPECT_EQ_VALS(create_symbol("abc123", "xyz"), *val);
    val = read_str("a.b.c/d.e.f");
    EXPECT_EQ_VALS(create_symbol("a.b.c", "d.e.f"), *val);
    val = read_str("a.b.c//");
    EXPECT_EQ_VALS(create_symbol("a.b.c", "/"), *val);
    val = read_str("a.b.c//////");
    EXPECT_EQ_VALS(create_symbol("a.b.c", "/////"), *val);
    val = read_str("cleo.core/seq");
    EXPECT_EQ_VALS(SEQ, *val);
}

TEST_F(reader_test, should_parse_single_slashes_as_symbols)
{
    Root source{create_string("//")};
    Root form, ex;
    ReaderStream stream{*source};

    form = read(stream);
    EXPECT_FALSE(stream.eos());
    ex = create_symbol("/");
    EXPECT_EQ_VALS(*ex, *form);

    form = read(stream);
    EXPECT_TRUE(stream.eos());
    EXPECT_EQ_VALS(*ex, *form);
}

TEST_F(reader_test, should_not_treat_newline_as_part_of_symbol)
{
    Root val{read_str("abc123\n")};
    EXPECT_EQ_VALS(create_symbol("abc123"), *val);
}

TEST_F(reader_test, should_parse_true)
{
    Root val{read_str("true")};
    EXPECT_EQ_REFS(TRUE, *val);
}

TEST_F(reader_test, should_parse_a_sequence_of_digits_as_an_integer)
{
    Root ex, val;
    ex = create_int64(1); val = read_str("1");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(23); val = read_str("23");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(32134); val = read_str("32134");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(9223372036854775807ull); val = read_str("9223372036854775807");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_negative_integers)
{
    Root ex, val;
    ex = create_int64(-58); val = read_str("-58");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(9223372036854775808ull); val = read_str("-9223372036854775808");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_hexadecimal_integers)
{
    Root ex, val;
    ex = create_int64(0x12345); val = read_str("0x12345");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(0xaaee); val = read_str("0xaaEe");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(0xaaee); val = read_str("0XaaEe");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(0x7fffffffffff); val = read_str("0x7fffffffffff");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_int64(-0x8000000000000000); val = read_str("-0x8000000000000000");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_octal_integers)
{
    Root ex, val;
    ex = create_int64(012345); val = read_str("012345");
    EXPECT_EQ_REFS(type::Int64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_decimal_fractions_as_floating_point_values)
{
    Root ex, val;
    ex = create_float64(1); val = read_str("1.0");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(3); val = read_str("3.");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(2.725); val = read_str("2.725");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(-32.125); val = read_str("-32.125");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_numbers_in_e_notation)
{
    Root ex, val;
    ex = create_float64(1e3); val = read_str("1e3");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(3e-3); val = read_str("3e-3");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(3e-3); val = read_str("03e-3");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(2.725E4); val = read_str("2.725E4");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(-32.125e-2); val = read_str("-32.125E-2");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(2.725E4); val = read_str("2.725e+4");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(-32.125e2); val = read_str("-32.125E+2");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(1.79769e+308); val = read_str("1.79769e+308");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_float64(2.22508e-308); val = read_str("2.22508e-308");
    EXPECT_EQ_REFS(*type::Float64, get_value_type(*val));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_fail_when_given_malformed_numbers)
{
    assert_read_error("malformed number: 1abc", "1abc x", 1, 1);
    assert_read_error("malformed number: 1e3e5", "1e3e5\n", 1, 1);
    assert_read_error("malformed number: 1-3", "1-3,", 1, 1);
    assert_read_error("malformed number: 1e3e5", "1e3e5[", 1, 1);
}

TEST_F(reader_test, should_fail_when_an_integer_is_of_range)
{
    assert_read_error("integer out of range: 9223372036854775808", "9223372036854775808", 1, 1);
    assert_read_error("integer out of range: -9223372036854775809", "-9223372036854775809", 1, 1);
    assert_read_error("integer out of range: 0x8000000000000000", "0x8000000000000000", 1, 1);
    assert_read_error("integer out of range: -0x8000000000000001", "-0x8000000000000001", 1, 1);
}

TEST_F(reader_test, should_fail_when_a_float_value_is_of_range)
{
    assert_read_error("floating-point value out of range: 1.79769e+309", "1.79769e+309", 1, 1);
    assert_read_error("floating-point value out of range: 2.22507e-309", "2.22507e-309", 1, 1);
}

TEST_F(reader_test, should_parse_an_empty_list)
{
    Root ex, val;
    ex = list();
    val = read_str("()");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("( )");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_a_list_of_expressions)
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

TEST_F(reader_test, should_parse_quoted_strings)
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

TEST_F(reader_test, should_not_treat_strings_as_parts_of_symbols)
{
    Root ex, val;
    ex = create_symbol("ab");
    ex = list(*ex, "cd"); val = read_str("(ab\"cd\")");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_escaped_newline_in_strings)
{
    Root ex, val;
    ex = create_string("ab\ncd"); val = read_str("\"ab\\ncd\"");
    EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\n\n\n"); val = read_str("\"\\n\\n\\n\"");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_escaped_backslashes_in_strings)
{
    Root ex, val;
    ex = create_string("ab\\cd"); val = read_str("\"ab\\\\cd\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\\\\\\"); val = read_str("\"\\\\\\\\\\\\\""); EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_escaped_quotes_in_strings)
{
    Root ex, val;
    ex = create_string("ab\"cd"); val = read_str("\"ab\\\"cd\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\"\"\""); val = read_str("\"\\\"\\\"\\\"\""); EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_escaped_unicode_characters)
{
    Root ex, val;
    ex = create_string(std::string("\0", 1)); val = read_str("\"\\u0000\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\x01"); val = read_str("\"\\u0001\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\x09"); val = read_str("\"\\u0009\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\n"); val = read_str("\"\\u000a\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\x0f"); val = read_str("\"\\u000f\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\n"); val = read_str("\"\\u000A\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\x0f"); val = read_str("\"\\u000F\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\x7f"); val = read_str("\"\\u007f\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\xc2\x80"); val = read_str("\"\\u0080\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\xdf\xbf"); val = read_str("\"\\u07ff\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\xdb\x9c"); val = read_str("\"\\u06dc\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\xe0\xa0\x80"); val = read_str("\"\\u0800\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\xeb\x9f\xa3"); val = read_str("\"\\ub7e3\""); EXPECT_EQ_VALS(*ex, *val);
    ex = create_string("\xef\xbf\xbf"); val = read_str("\"\\uffff\""); EXPECT_EQ_VALS(*ex, *val);

    assert_read_error("invalid character length: 0", "\"\\uz\"", 1, 4);
    assert_read_error("invalid character length: 0", "\" \\u", 1, 5);
    assert_read_error("invalid character length: 1", "\"\\u0z\"", 1, 5);
    assert_read_error("invalid character length: 1", "\" \\u0", 1, 6);
    assert_read_error("invalid character length: 2", "\"\\u00z\"", 1, 6);
    assert_read_error("invalid character length: 2", "\" \\u00", 1, 7);
    assert_read_error("invalid character length: 3", "\"\\u000z\"", 1, 7);
    assert_read_error("invalid character length: 3", "\" \\u000", 1, 8);
}

TEST_F(reader_test, should_parse_characters)
{
    Root val;
    val = read_str("\\a");
    EXPECT_EQ_VALS(create_uchar('a'), *val);
    val = read_str("\\?");
    EXPECT_EQ_VALS(create_uchar('?'), *val);
    val = read_str("\\7");
    EXPECT_EQ_VALS(create_uchar('7'), *val);
    val = read_str("\\\\");
    EXPECT_EQ_VALS(create_uchar('\\'), *val);
    val = read_str("\\\'");
    EXPECT_EQ_VALS(create_uchar('\''), *val);
    val = read_str("\\\"");
    EXPECT_EQ_VALS(create_uchar('\"'), *val);
    val = read_str("\\(");
    EXPECT_EQ_VALS(create_uchar('('), *val);
    val = read_str("\\(()");
    EXPECT_EQ_VALS(create_uchar('('), *val);
    val = read_str("\\#");
    EXPECT_EQ_VALS(create_uchar('#'), *val);
    val = read_str("\\##");
    EXPECT_EQ_VALS(create_uchar('#'), *val);

    val = read_str("\\newline");
    EXPECT_EQ_VALS(create_uchar('\n'), *val);
    val = read_str("\\space");
    EXPECT_EQ_VALS(create_uchar(' '), *val);
    val = read_str("\\tab");
    EXPECT_EQ_VALS(create_uchar('\t'), *val);
    val = read_str("\\formfeed");
    EXPECT_EQ_VALS(create_uchar('\f'), *val);
    val = read_str("\\backspace");
    EXPECT_EQ_VALS(create_uchar('\b'), *val);
    val = read_str("\\return");
    EXPECT_EQ_VALS(create_uchar('\r'), *val);

    val = read_str("\\u00");
    EXPECT_EQ_VALS(create_uchar(0), *val);
    val = read_str("\\ua7(");
    EXPECT_EQ_VALS(create_uchar(0xa7), *val);
    val = read_str("\\ufF\n");
    EXPECT_EQ_VALS(create_uchar(0xff), *val);
    val = read_str("\\u0000");
    EXPECT_EQ_VALS(create_uchar(0), *val);
    val = read_str("\\ua5b7#");
    EXPECT_EQ_VALS(create_uchar(0xa5b7), *val);
    val = read_str("\\uffff ");
    EXPECT_EQ_VALS(create_uchar(0xffff), *val);
    val = read_str("\\u000000");
    EXPECT_EQ_VALS(create_uchar(0), *val);
    val = read_str("\\u10b7c8$");
    EXPECT_EQ_VALS(create_uchar(0x10b7c8), *val);
    val = read_str("\\u10ffff ");
    EXPECT_EQ_VALS(create_uchar(0x10ffff), *val);

    assert_unexpected_end_of_input("\\", 1, 2);
    assert_unexpected_end_of_input("  \\", 1, 4);
    assert_read_error("invalid character: \\bad", "\n\\bad", 2, 1);
    assert_read_error("invalid character: \\x1234", "\\x1234", 1, 1);
    assert_read_error("invalid character: \\y123456", "\\y123456", 1, 1);
    assert_read_error("invalid character: \\u1z", "\\u1z", 1, 1);
    assert_read_error("invalid character: \\u12z", "\\u12z", 1, 1);
    assert_read_error("invalid character: \\u123z", "\\u123z", 1, 1);
    assert_read_error("invalid character: \\u1234z", "\\u1234z", 1, 1);
    assert_read_error("invalid character: \\u12345z", "\\u12345z", 1, 1);
    assert_read_error("invalid character: \\u10ffffz", "\\u10ffffz", 1, 1);
    assert_read_error("invalid character: \\u110000", "\\u110000", 1, 1);
}

TEST_F(reader_test, should_parse_a_sequence_of_characters_beginning_with_a_colon_as_a_keyword)
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
    val = read_str(":abc/def");
    EXPECT_EQ_VALS(create_keyword("abc", "def"), *val);
    val = read_str(":x/y/z");
    EXPECT_EQ_VALS(create_keyword("x", "y/z"), *val);
}

TEST_F(reader_test, double_colon_should_resolve_namespace_aliases_in_a_keyword)
{
    Root val;
    in_ns(create_symbol("cleo.reader.keyword.test"));
    val = read_str("::abc123");
    EXPECT_EQ_VALS(create_keyword("cleo.reader.keyword.test", "abc123"), *val);

    in_ns(create_symbol("cleo.reader.test"));
    alias(create_symbol("rkt"), create_symbol("cleo.reader.keyword.test"));
    val = read_str("::rkt/abc1234");
    EXPECT_EQ_VALS(create_keyword("cleo.reader.keyword.test", "abc1234"), *val);
}

TEST_F(reader_test, should_parse_an_apostrophe_as_quote)
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

TEST_F(reader_test, should_parse_an_empty_vector)
{
    Root ex, val;
    ex = array();
    val = read_str("[]");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("[ ]");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_a_vector_of_expressions)
{
    Root ex, val;
    ex = array(1); val = read_str("[1]");
    EXPECT_EQ_VALS(*ex, *val);
    ex = array(create_symbol("+"), create_symbol("abc"), -3); val = read_str("[+ abc -3]");
    EXPECT_EQ_VALS(*ex, *val);
    Root l7{array(7)};
    ex = array(create_symbol("x"), -3);
    ex = array(*ex, 1);
    ex = array(*ex, *l7);
    val = read_str("[[[x -3] 1] [7]]");
    EXPECT_EQ_VALS(*ex, *val);
    ex = array(create_keyword("x")); val = read_str("[:x]");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_fail_when_parsing_an_unmatched_closing_bracket)
{
    assert_read_error("unexpected ]", "]", 1, 1);
}

TEST_F(reader_test, should_fail_when_parsing_an_unmatched_closing_paren)
{
    assert_read_error("unexpected )", ")", 1, 1);
}

TEST_F(reader_test, should_parse_nil)
{
    EXPECT_EQ_REFS(nil, *Root(read_str("nil")));
    EXPECT_EQ_REFS(nil, *Root(read_str("nil ")));
    Root ex, val;
    ex = list(nil); val = read_str("(nil)");
    EXPECT_EQ_VALS(*ex, *val);
    ex = array(nil); val = read_str("[nil]");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_fail_when_missing_a_closing_paren)
{
    assert_unexpected_end_of_input("(", 1, 2);
    assert_unexpected_end_of_input("( 5 ", 1, 5);
    assert_unexpected_end_of_input("(()", 1, 4);
}

TEST_F(reader_test, should_fail_when_missing_a_closing_bracket)
{
    assert_unexpected_end_of_input("[", 1, 2);
    assert_unexpected_end_of_input("[ 5 ", 1, 5);
    assert_unexpected_end_of_input("[[]", 1, 4);
    assert_unexpected_end_of_input("{", 1, 2);
    assert_unexpected_end_of_input("#{", 1, 3);
}

TEST_F(reader_test, should_fail_when_missing_a_closing_quote)
{
    assert_unexpected_end_of_input("\"", 1, 2);
}

TEST_F(reader_test, should_fail_when_invoked_with_something_else_than_a_string)
{
    Root source{create_string("abc")};
    ASSERT_NO_THROW(read(*source));
    ASSERT_ANY_THROW(read(create_symbol("abc")));
}

TEST_F(reader_test, should_parse_an_empty_map)
{
    Root ex, val;
    ex = amap();
    val = read_str("{}");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("{ }");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_a_map_of_expressions)
{
    Root ex, val;
    ex = amap(1, 2); val = read_str("{1 2}");
    EXPECT_EQ_VALS(*ex, *val);
    ex = amap(create_symbol("+"), create_keyword("abc"), -3, nil); val = read_str("{+ :abc, -3 nil}");
    EXPECT_EQ_VALS(*ex, *val);
    ex = amap(create_symbol("x"), -3);
    ex = amap(*ex, 1);
    ex = amap(*ex, 7);
    val = read_str("{{{x -3} 1} 7}");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_fail_when_a_value_in_a_map_is_missing)
{
    assert_read_error("map literal must contain an even number of forms", "{1}", 1, 1);
    assert_read_error("map literal must contain an even number of forms", "{1 3 4}", 1, 1);
}

TEST_F(reader_test, should_parse_an_empty_set)
{
    Root ex, val;
    ex = create_array_set();
    val = read_str("#{}");
    EXPECT_EQ_VALS(*ex, *val);
    EXPECT_EQ_VALS(*type::ArraySet, get_value_type(*val));
    val = read_str("#{ }");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_a_set_of_expressions)
{
    Root ex, val;
    ex = aset(1, 2); val = read_str("#{1 2}");
    EXPECT_EQ_VALS(*ex, *val);
    ex = aset(create_symbol("+"), create_keyword("abc"), -3, nil); val = read_str("#{ + :abc -3 nil}");
    EXPECT_EQ_VALS(*ex, *val);
    ex = aset(create_symbol("x"), -3);
    ex = aset(*ex, 1);
    ex = aset(*ex, 7);
    val = read_str("#{#{#{x -3} 1} 7}");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_fail_when_given_a_single_hash)
{
    assert_read_error("unexpected #", "# {1}", 1, 1);
    assert_read_error("unexpected #", "# ", 1, 1);
    assert_read_error("unexpected #", "#", 1, 1);
}

TEST_F(reader_test, should_fail_when_a_key_in_a_set_is_duplicated)
{
    assert_read_error("duplicate key: 6", "#{5 6 6 7}", 1, 7);
}

TEST_F(reader_test, should_parse_the_at_symbol_as_deref)
{
    Root ex, val;
    ex = list(DEREF, 7);
    val = read_str("@7");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("@ 7");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(DEREF, create_symbol("abc123"));
    val = read_str("@abc123");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(1);
    ex = list(DEREF, *ex);
    val = read_str("@(1)");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("@ (1)");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_tilde_as_unquote)
{
    Root ex, val;
    ex = list(UNQUOTE, 7);
    val = read_str("~7");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("~ 7");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(UNQUOTE, create_symbol("abc123"));
    val = read_str("~abc123");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(1);
    ex = list(UNQUOTE, *ex);
    val = read_str("~(1)");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("~ (1)");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_parse_tilde_and_the_at_symbol_as_unquote_splicing)
{
    Root ex, val;
    ex = list(UNQUOTE_SPLICING, 7);
    val = read_str("~@7");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("~@ 7");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(UNQUOTE_SPLICING, create_symbol("abc123"));
    val = read_str("~@abc123");
    EXPECT_EQ_VALS(*ex, *val);
    ex = list(1);
    ex = list(UNQUOTE_SPLICING, *ex);
    val = read_str("~@(1)");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("~@ (1)");
    EXPECT_EQ_VALS(*ex, *val);

    ex = list(DEREF, create_symbol("abc123"));
    ex = list(UNQUOTE, *ex);
    val = read_str("~ @abc123");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, should_syntax_quote_simple_values)
{
    Root ex, val;
    ex = read_str("7");
    val = read_str("`7");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str(":k");
    val = read_str("`:k");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, syntax_quote_should_resolve_symbols)
{
    in_ns(create_symbol("cleo.reader.syntax-quote.test"));
    Root ex, val;
    ex = list(QUOTE, create_symbol("cleo.reader.syntax-quote.test", "x"));
    val = read_str("`x");
    EXPECT_EQ_VALS(*ex, *val);

    in_ns(create_symbol("cleo.reader.syntax-quote.test2"));
    define(create_symbol("cleo.reader.syntax-quote.test2", "z"), create_keyword(":zz"));
    in_ns(create_symbol("cleo.reader.syntax-quote.test"));
    refer(create_symbol("cleo.reader.syntax-quote.test2"));

    ex = list(QUOTE, create_symbol("cleo.reader.syntax-quote.test2", "z"));
    val = read_str("`z");
    EXPECT_EQ_VALS(*ex, *val);

    ex = list(QUOTE, create_symbol("some", "abc"));
    val = read_str("`some/abc");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, syntax_quote_should_not_quote_special_symbols)
{
    auto expect_no_quote = [](Value sym)
    {
        Root ex{list(QUOTE, sym)};
        Root val{read_str("`" + to_string(sym))};
        EXPECT_EQ_VALS(*ex, *val);
    };

    expect_no_quote(QUOTE);
    expect_no_quote(FN);
    expect_no_quote(DEF);
    expect_no_quote(LET);
    expect_no_quote(DO);
    expect_no_quote(IF);
    expect_no_quote(LOOP);
    expect_no_quote(RECUR);
    expect_no_quote(THROW);
    expect_no_quote(TRY);
    expect_no_quote(CATCH);
    expect_no_quote(FINALLY);
    expect_no_quote(VA);
}

TEST_F(reader_test, syntax_quote_should_generate_non_namespace_qualified_symbols)
{
    in_ns(create_symbol("cleo.reader.syntax-quote.gen.test"));
    define(create_symbol("cleo.reader.syntax-quote.gen.test", "some#"), create_keyword("bad"));

    Override<Int64> override_next_id(next_id, 37);
    Root val, ex;
    ex = read_str("(quote some__37__auto__)");
    val = read_str("`some#");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(quote abc/some#)");
    val = read_str("`abc/some#");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati (cleo.core/list (quote x__38__auto__)) (cleo.core/list (quote y__39__auto__)) (cleo.core/list (quote z__40__auto__))))");
    val = read_str("`[x# y# z#]");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati (cleo.core/list (quote x__41__auto__)) (cleo.core/list (quote y__42__auto__)) (cleo.core/list (quote x__41__auto__)) (cleo.core/list (quote y__42__auto__))))");
    val = read_str("`[x# y# x# y#]");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati (cleo.core/list (cleo.core/apply cleo.core/list (cleo.core/concati (cleo.core/list (cleo.core/apply cleo.core/vector (cleo.core/concati (cleo.core/list (quote x__43__auto__))))) (cleo.core/list (cleo.core/apply cleo.core/hash-map (cleo.core/concati (cleo.core/list (quote x__43__auto__)) (cleo.core/list (quote x__43__auto__))))) (cleo.core/list (cleo.core/apply cleo.core/hash-set (cleo.core/concati (cleo.core/list (quote x__43__auto__)))))))) (cleo.core/list (quote x__43__auto__))))");
    val = read_str("`[([x#] {x# x#} #{x#}) x#]");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati (cleo.core/list (quote x__45__auto__)) (cleo.core/list (quote x__44__auto__))))");
    val = read_str("`[x# ~`x#]");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, syntax_quote_should_resolve_symbols_in_vectors)
{
    in_ns(create_symbol("cleo.reader.syntax-quote.test"));
    Root val, ex;
    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati))");
    val = read_str("`[]");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati (cleo.core/list 7)))");
    val = read_str("`[7]");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati (cleo.core/list 7) (cleo.core/list (quote cleo.reader.syntax-quote.test/x)) (cleo.core/list (quote cleo.reader.syntax-quote.test/y)) (cleo.core/list 20)))");
    val = read_str("`[7 x y 20]");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, syntax_quote_should_resolve_symbols_in_lists)
{
    in_ns(create_symbol("cleo.reader.syntax-quote.test"));
    Root val, ex;
    ex = read_str("(cleo.core/apply cleo.core/list (cleo.core/concati))");
    val = read_str("`()");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/list (cleo.core/concati (cleo.core/list 7)))");
    val = read_str("`(7)");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/list (cleo.core/concati (cleo.core/list 7) (cleo.core/list (quote cleo.reader.syntax-quote.test/x)) (cleo.core/list (quote cleo.reader.syntax-quote.test/y)) (cleo.core/list 20)))");
    val = read_str("`(7 x y 20)");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, syntax_quote_should_resolve_symbols_in_sets)
{
    in_ns(create_symbol("cleo.reader.syntax-quote.test"));
    Root val, ex;
    ex = read_str("(cleo.core/apply cleo.core/hash-set (cleo.core/concati))");
    val = read_str("`#{}");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/hash-set (cleo.core/concati (cleo.core/list 7)))");
    val = read_str("`#{7}");
    EXPECT_EQ_VALS(*ex, *val);

    Root ex1{read_str("(cleo.core/apply cleo.core/hash-set (cleo.core/concati (cleo.core/list 7) (cleo.core/list (quote cleo.reader.syntax-quote.test/x))))")};
    Root ex2{read_str("(cleo.core/apply cleo.core/hash-set (cleo.core/concati (cleo.core/list 7) (cleo.core/list (quote cleo.reader.syntax-quote.test/x))))")};
    val = read_str("`#{7 x}");
    EXPECT_EQ_VALS_ALT2(*ex1, *ex2, *val);
}

TEST_F(reader_test, syntax_quote_should_resolve_symbols_in_maps)
{
    in_ns(create_symbol("cleo.reader.syntax-quote.test"));
    Root val, ex;
    ex = read_str("(cleo.core/apply cleo.core/hash-map (cleo.core/concati))");
    val = read_str("`{}");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/hash-map (cleo.core/concati (cleo.core/list 7) (cleo.core/list 9)))");
    val = read_str("`{7 9}");
    EXPECT_EQ_VALS(*ex, *val);

    Root ex1{read_str("(cleo.core/apply cleo.core/hash-map (cleo.core/concati (cleo.core/list 7) (cleo.core/list (quote cleo.reader.syntax-quote.test/x)) (cleo.core/list (quote cleo.reader.syntax-quote.test/y)) (cleo.core/list 20)))")};
    Root ex2{read_str("(cleo.core/apply cleo.core/hash-map (cleo.core/concati (cleo.core/list (quote cleo.reader.syntax-quote.test/y)) (cleo.core/list 20) (cleo.core/list 7) (cleo.core/list (quote cleo.reader.syntax-quote.test/x))))")};
    val = read_str("`{7 x y 20}");
    EXPECT_EQ_VALS_ALT2(*ex1, *ex2, *val);
}

TEST_F(reader_test, syntax_quote_should_not_quote_unquoted_expressions)
{
    Root val, ex;
    ex = create_symbol("x");
    val = read_str("`~x");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati (cleo.core/list x)))");
    val = read_str("`[~x]");
    EXPECT_EQ_VALS(*ex, *val);

    ex = nil;
    val = read_str("`(cleo.core/unquote)");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, unquote_splicing_should_splice_a_sequence_into_another_sequence)
{
    Root val, ex;
    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati (cleo.core/list 1) x (cleo.core/list 2)))");
    val = read_str("`[1 ~@x 2]");
    EXPECT_EQ_VALS(*ex, *val);

    ex = read_str("(cleo.core/apply cleo.core/vector (cleo.core/concati nil))");
    val = read_str("`[(cleo.core/unquote-splicing)]");
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(reader_test, unquote_splicing_should_fail_when_not_inside_a_sequence)
{
    EXPECT_ANY_THROW(read_str("`~@x"));
}

TEST_F(reader_test, read_should_allow_reading_successive_forms)
{
    Root source{create_string(";\n:a ;\n:b :c ,;\n ")};
    Root form, ex;
    ReaderStream stream{*source};

    form = read(stream);
    EXPECT_FALSE(stream.eos());
    ex = create_keyword("a");
    EXPECT_EQ_VALS(*ex, *form);

    form = read(stream);
    EXPECT_FALSE(stream.eos());
    ex = create_keyword("b");
    EXPECT_EQ_VALS(*ex, *form);

    form = read(stream);
    EXPECT_TRUE(stream.eos());
    ex = create_keyword("c");
    EXPECT_EQ_VALS(*ex, *form);
}

TEST_F(reader_test, should_follow_new_lines_when_reporting_position)
{
    assert_read_error("duplicate key: 3", "#{3 3}", 1, 5);
    assert_read_error("duplicate key: 3", "#{3 \n3}", 2, 1);
    assert_read_error("duplicate key: 3", "\n#{3 3}", 2, 5);
    assert_read_error("duplicate key: 3", "  \n#{3 3}", 2, 5);
    assert_read_error("duplicate key: 3", "  \n\n#{3   3}", 3, 7);
    assert_read_error("unexpected ]", "\n]", 2, 1);
    assert_read_error("unexpected ]", "\n   ]", 2, 4);
    assert_read_error("unexpected ]", "  \n\n  \n   ]", 4, 4);
}

TEST_F(reader_test, should_parse_vars)
{
    Root ex, val;
    in_ns(create_symbol("cleo.reader.vars.test"));
    auto var = define(create_symbol("cleo.reader.vars.test", "abc"), nil);
    val = read_str("#'abc");
    EXPECT_EQ_VALS(var, *val);
    val = read_str("#' abc");
    EXPECT_EQ_VALS(var, *val);
    val = read_str("#'cleo.reader.vars.test/abc");
    EXPECT_EQ_VALS(var, *val);
    val = read_str("#' cleo.reader.vars.test/abc");
    EXPECT_EQ_VALS(var, *val);
}

TEST_F(reader_test, should_fail_when_var_name_is_missing)
{
    assert_unexpected_end_of_input("#'", 1, 3);
    assert_read_error("unexpected }", "#' }", 1, 4);
    assert_read_error("expected a symbol", "#' [9]", 1, 4);
}

TEST_F(reader_test, should_skip_comments)
{
    Root ex, val;
    val = read_str("(1 2; \n3)");
    ex = read_str("(1 2 3)");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("(1 ;2 \n3)");
    ex = read_str("(1 3)");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str("(1 ;2 ) \n3);");
    ex = read_str("(1 3)");
    EXPECT_EQ_VALS(*ex, *val);
    val = read_str(";\n(1 ;2 ) \n ;\n; \n 3);");
    ex = read_str("(1 3)");
    EXPECT_EQ_VALS(*ex, *val);
}

}
}
