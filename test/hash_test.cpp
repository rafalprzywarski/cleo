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
    Root type{create_object_type("some", "type")};
    Root val{create_object0(*type)};
    ASSERT_EQ(0u, hash(*val));
}

TEST_F(hash_test, should_hash_values)
{
    Root fn{create_native_function([](const Value *, std::uint8_t) { return force(nil); })};
    auto sym = create_symbol("org.xyz", "eqsym");
    auto kw = create_keyword("org.xyz", "eqkw");
    Root val;
    ASSERT_EQ(std::hash<Value>{}(*fn), hash(*fn));
    ASSERT_EQ(std::hash<Value>{}(sym), hash(sym));
    ASSERT_EQ(std::hash<Value>{}(kw), hash(kw));
    val = create_int64(55);
    ASSERT_EQ(std::hash<Int64>{}(55), hash(*val));
    val = create_float64(5.5);
    ASSERT_EQ(std::hash<Float64>{}(5.5), hash(*val));
    val = create_string("hamster");
    ASSERT_EQ(std::hash<std::string>{}("hamster"), hash(*val));
}

}
}
