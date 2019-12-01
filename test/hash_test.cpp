#include <cleo/hash.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct hash_test : Test
{
    hash_test() : Test("cleo.hash.test") { }
    Int64 hash(Value val)
    {
        Root h{cleo::hash(val)};
        return get_int64_value(*h);
    }
};

TEST_F(hash_test, should_return_0_for_nil)
{
    ASSERT_EQ(0u, hash(nil));
}

TEST_F(hash_test, should_return_0_for_objects)
{
    Root type{create_dynamic_object_type("some", "type")};
    Root val{create_object0(*type)};
    ASSERT_EQ(0u, hash(*val));
}

TEST_F(hash_test, should_hash_values)
{
    Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); })};
    auto sym = create_symbol("org.xyz", "eqsym");
    auto kw = create_keyword("org.xyz", "eqkw");
    Root val;
    EXPECT_EQ(Int64(std::hash<Value>{}(*fn)), hash(*fn));
    EXPECT_EQ(std::uint32_t(std::uint32_t(std::hash<std::string>{}("org.xyz")) * 31 + std::hash<std::string>{}("eqsym")), hash(sym));
    EXPECT_EQ(std::uint32_t(std::uint32_t(std::hash<std::string>{}("org.xyz")) * 31 + std::hash<std::string>{}("eqsym")), hash(sym));
    EXPECT_EQ(std::uint32_t(std::uint32_t(std::hash<std::string>{}("org.xyz")) * 31 + std::hash<std::string>{}("eqkw")), hash(kw));
    EXPECT_EQ(std::uint32_t(std::uint32_t(std::hash<std::string>{}("org.xyz")) * 31 + std::hash<std::string>{}("eqkw")), hash(kw));
    val = create_int64(55);
    EXPECT_EQ(Int64(std::hash<Int64>{}(55)), hash(*val));
    EXPECT_EQ(Int64(std::hash<Char32>{}(123456)), hash(create_char32(123456)));
    val = create_float64(5.5);
    EXPECT_EQ(Int64(std::hash<Float64>{}(5.5)), hash(*val));
    val = create_string("hamster");
    EXPECT_EQ(std::uint32_t(std::hash<std::string>{}("hamster")), hash(*val));
    EXPECT_EQ(std::uint32_t(std::hash<std::string>{}("hamster")), hash(*val));
}

}
}
