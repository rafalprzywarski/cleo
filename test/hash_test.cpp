#include <cleo/hash.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct hash_test : Test {};

TEST(hash_test, should_return_0_for_nil)
{
    ASSERT_EQ(0u, get_int64_value(hash(nil)));
}

TEST(hash_test, should_return_0_for_objects)
{
    Root val;
    *val = create_object0(nil);
    ASSERT_EQ(0u, get_int64_value(hash(*val)));
}

TEST(hash_test, should_hash_values)
{
    Root fn{force(create_native_function([](const Value *, std::uint8_t) { return nil; }))};
    auto sym = create_symbol("org.xyz", "eqsym");
    auto kw = create_keyword("org.xyz", "eqkw");
    Root val;
    ASSERT_EQ(std::hash<Value>{}(*fn), get_int64_value(hash(*fn)));
    ASSERT_EQ(std::hash<Value>{}(sym), get_int64_value(hash(sym)));
    ASSERT_EQ(std::hash<Value>{}(kw), get_int64_value(hash(kw)));
    *val = create_int64(55);
    ASSERT_EQ(std::hash<Int64>{}(55), get_int64_value(hash(*val)));
    *val = create_float64(5.5);
    ASSERT_EQ(std::hash<Float64>{}(5.5), get_int64_value(hash(*val)));
    *val = create_string("hamster");
    ASSERT_EQ(std::hash<std::string>{}("hamster"), get_int64_value(hash(*val)));
}

}
}
