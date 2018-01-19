#include <cleo/eval.hpp>
#include <cleo/reader.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct math_test : Test
{
    math_test() : Test("cleo.math.test")
    {
        refer(CLEO_CORE);
    }

    Force read_str(const std::string& s)
    {
        Root ss{create_string(s)};
        return read(*ss);
    }
};

TEST_F(math_test, integer_addition)
{
    Root val{read_str("(+ 2457245677 823458723)")}, ex{create_int64(3280704400)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = list(PLUS, Int64((std::uint64_t(1) << 63) - 1), 1);
    EXPECT_THROW(eval(*val), Exception);

    val = list(PLUS, 1, Int64((std::uint64_t(1) << 63) - 1));
    EXPECT_THROW(eval(*val), Exception);

    val = list(PLUS, Int64(std::uint64_t(1) << 63), -1);
    EXPECT_THROW(eval(*val), Exception);

    val = list(PLUS, -1, Int64(std::uint64_t(1) << 63));
    EXPECT_THROW(eval(*val), Exception);

    val = list(PLUS, Int64((std::uint64_t(1) << 63) - 1), Int64(std::uint64_t(1) << 63));
    val = eval(*val);
    ex = i64(-1);
    EXPECT_EQ_VALS(*ex, *val);

    val = list(PLUS, Int64(std::uint64_t(1) << 63), Int64((std::uint64_t(1) << 63) - 1));
    val = eval(*val);
    ex = i64(-1);
    EXPECT_EQ_VALS(*ex, *val);

    val = list(PLUS, Int64((std::uint64_t(1) << 63) - (std::uint64_t(1) << 17)), Int64(std::uint64_t(1) << 63));
    val = eval(*val);
    ex = i64(-Int64(std::uint64_t(1) << 17));
    EXPECT_EQ_VALS(*ex, *val);

    val = list(PLUS, Int64(std::uint64_t(1) << 63), Int64((std::uint64_t(1) << 63) - (std::uint64_t(1) << 17)));
    val = eval(*val);
    ex = i64(-Int64(std::uint64_t(1) << 17));
    EXPECT_EQ_VALS(*ex, *val);
}

TEST_F(math_test, integer_subtraction)
{
    Root val{read_str("(- 2457245677 823458723)")}, ex{create_int64(1633786954)};
    val = eval(*val);
    EXPECT_EQ_VALS(*ex, *val);

    val = list(MINUS, Int64(std::uint64_t(1) << 63), 1);
    EXPECT_THROW(eval(*val), Exception);

    val = list(MINUS, 0, Int64(std::uint64_t(1) << 63));
    EXPECT_THROW(eval(*val), Exception);

    val = list(MINUS, Int64((std::uint64_t(1) << 63) - 1), -1);
    EXPECT_THROW(eval(*val), Exception);

    val = list(MINUS, -2, Int64((std::uint64_t(1) << 63) - 1));
    EXPECT_THROW(eval(*val), Exception);

    val = list(MINUS, Int64((std::uint64_t(1) << 63) - 1), Int64((std::uint64_t(1) << 63) - 2));
    val = eval(*val);
    ex = i64(1);
    EXPECT_EQ_VALS(*ex, *val);

    val = list(MINUS, Int64((std::uint64_t(1) << 63) - 2), Int64((std::uint64_t(1) << 63) - 1));
    val = eval(*val);
    ex = i64(-1);
    EXPECT_EQ_VALS(*ex, *val);

    val = list(MINUS, Int64((std::uint64_t(1) << 63) + (std::uint64_t(1) << 17)), Int64(std::uint64_t(1) << 63));
    val = eval(*val);
    ex = i64(Int64(std::uint64_t(1) << 17));
    EXPECT_EQ_VALS(*ex, *val);

    val = list(MINUS, Int64(std::uint64_t(1) << 63), Int64((std::uint64_t(1) << 63) + (std::uint64_t(1) << 17)));
    val = eval(*val);
    ex = i64(-Int64(std::uint64_t(1) << 17));
    EXPECT_EQ_VALS(*ex, *val);
}

}
}
