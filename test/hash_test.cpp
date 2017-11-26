#include <cleo/hash.hpp>
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

TEST(hash_test, should_return_0_for_nil)
{
    ASSERT_EQ(0u, get_int64_value(hash(nil)));
}

TEST(hash_test, should_return_0_for_objects)
{
    ASSERT_EQ(0u, get_int64_value(hash(create_object(nil, nullptr, 0))));
}

TEST(hash_test, should_hash_values)
{
    auto fn = create_native_function([](const Value *, std::uint8_t) { return nil; });
    auto sym = create_symbol("org.xyz", "eqsym");
    auto kw = create_keyword("org.xyz", "eqkw");
    ASSERT_EQ(std::hash<Value>{}(fn), get_int64_value(hash(fn)));
    ASSERT_EQ(std::hash<Value>{}(sym), get_int64_value(hash(sym)));
    ASSERT_EQ(std::hash<Value>{}(kw), get_int64_value(hash(kw)));
    ASSERT_EQ(std::hash<Int64>{}(55), get_int64_value(hash(create_int64(55))));
    ASSERT_EQ(std::hash<Float64>{}(5.5), get_int64_value(hash(create_float64(5.5))));
    ASSERT_EQ(std::hash<std::string>{}("hamster"), get_int64_value(hash(create_string("hamster"))));
}

}
}
