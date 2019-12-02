#include <cleo/string_seq.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct string_seq_test : Test
{
    string_seq_test() : Test("cleo.string-seq.test") { }
};

TEST_F(string_seq_test, seq_should_return_nil_for_an_empty_string)
{
    Root str{create_string("")};
    Root s{string_seq(*str)};
    ASSERT_EQ_VALS(nil, *s);
}

TEST_F(string_seq_test, seq_should_create_a_sequence_of_chracters)
{
    Root str{create_string("\x32" "\x7f"
                           "\xc2\x80"
                           "\xe0\xa0\x80"
                           "\xf0\x90\x80\x80" "\xf2\x97\x8c\xa4")};
    Root s{string_seq(*str)};
    ASSERT_FALSE(s->is_nil());
    EXPECT_EQ(create_uchar(0x32), string_seq_first(*s));
    s = string_seq_next(*s);
    ASSERT_FALSE(s->is_nil());
    EXPECT_EQ(create_uchar(0x7f), string_seq_first(*s));
    s = string_seq_next(*s);
    ASSERT_FALSE(s->is_nil());
    EXPECT_EQ(create_uchar(0x80), string_seq_first(*s));
    s = string_seq_next(*s);
    ASSERT_FALSE(s->is_nil());
    EXPECT_EQ(create_uchar(0x800), string_seq_first(*s));
    s = string_seq_next(*s);
    ASSERT_FALSE(s->is_nil());
    EXPECT_EQ(create_uchar(0x10000), string_seq_first(*s));
    s = string_seq_next(*s);
    ASSERT_FALSE(s->is_nil());
    EXPECT_EQ(create_uchar(0x97324), string_seq_first(*s));
    s = string_seq_next(*s);
    ASSERT_TRUE(s->is_nil());
}

}
}
