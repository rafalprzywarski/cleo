#include <cleo/equality.hpp>
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

TEST(equality_test, same_instances_should_be_equal)
{
    auto fn = create_native_function([](const Value *, std::uint8_t) { return get_nil(); });
    auto sym = create_symbol("org.xyz", "eqsym");
    auto kw = create_keyword("org.xyz", "eqkw");
    auto i = create_int64(7);
    auto flt = create_float64(3.5);
    auto s = create_string("abcd");
    auto o = create_object(sym, nullptr, 0);

    ASSERT_TRUE(get_nil() != are_equal(get_nil(), get_nil()));
    ASSERT_TRUE(get_nil() == are_equal(get_nil(), fn));
    ASSERT_TRUE(get_nil() == are_equal(get_nil(), sym));
    ASSERT_TRUE(get_nil() == are_equal(get_nil(), kw));
    ASSERT_TRUE(get_nil() == are_equal(get_nil(), i));
    ASSERT_TRUE(get_nil() == are_equal(get_nil(), flt));
    ASSERT_TRUE(get_nil() == are_equal(get_nil(), s));
    ASSERT_TRUE(get_nil() == are_equal(get_nil(), o));

    ASSERT_TRUE(get_nil() == are_equal(fn, get_nil()));
    ASSERT_TRUE(get_nil() != are_equal(fn, fn));
    ASSERT_TRUE(get_nil() == are_equal(fn, sym));
    ASSERT_TRUE(get_nil() == are_equal(fn, kw));
    ASSERT_TRUE(get_nil() == are_equal(fn, i));
    ASSERT_TRUE(get_nil() == are_equal(fn, flt));
    ASSERT_TRUE(get_nil() == are_equal(fn, s));
    ASSERT_TRUE(get_nil() == are_equal(fn, o));

    ASSERT_TRUE(get_nil() == are_equal(sym, get_nil()));
    ASSERT_TRUE(get_nil() == are_equal(sym, fn));
    ASSERT_TRUE(get_nil() != are_equal(sym, sym));
    ASSERT_TRUE(get_nil() == are_equal(sym, kw));
    ASSERT_TRUE(get_nil() == are_equal(sym, i));
    ASSERT_TRUE(get_nil() == are_equal(sym, flt));
    ASSERT_TRUE(get_nil() == are_equal(sym, s));
    ASSERT_TRUE(get_nil() == are_equal(sym, o));

    ASSERT_TRUE(get_nil() == are_equal(kw, get_nil()));
    ASSERT_TRUE(get_nil() == are_equal(kw, fn));
    ASSERT_TRUE(get_nil() == are_equal(kw, sym));
    ASSERT_TRUE(get_nil() != are_equal(kw, kw));
    ASSERT_TRUE(get_nil() == are_equal(kw, i));
    ASSERT_TRUE(get_nil() == are_equal(kw, flt));
    ASSERT_TRUE(get_nil() == are_equal(kw, s));
    ASSERT_TRUE(get_nil() == are_equal(kw, o));

    ASSERT_TRUE(get_nil() == are_equal(i, get_nil()));
    ASSERT_TRUE(get_nil() == are_equal(i, fn));
    ASSERT_TRUE(get_nil() == are_equal(i, sym));
    ASSERT_TRUE(get_nil() == are_equal(i, kw));
    ASSERT_TRUE(get_nil() != are_equal(i, i));
    ASSERT_TRUE(get_nil() == are_equal(i, flt));
    ASSERT_TRUE(get_nil() == are_equal(i, s));
    ASSERT_TRUE(get_nil() == are_equal(i, o));

    ASSERT_TRUE(get_nil() == are_equal(flt, get_nil()));
    ASSERT_TRUE(get_nil() == are_equal(flt, fn));
    ASSERT_TRUE(get_nil() == are_equal(flt, sym));
    ASSERT_TRUE(get_nil() == are_equal(flt, kw));
    ASSERT_TRUE(get_nil() == are_equal(flt, i));
    ASSERT_TRUE(get_nil() != are_equal(flt, flt));
    ASSERT_TRUE(get_nil() == are_equal(flt, s));
    ASSERT_TRUE(get_nil() == are_equal(flt, o));

    ASSERT_TRUE(get_nil() == are_equal(s, get_nil()));
    ASSERT_TRUE(get_nil() == are_equal(s, fn));
    ASSERT_TRUE(get_nil() == are_equal(s, sym));
    ASSERT_TRUE(get_nil() == are_equal(s, kw));
    ASSERT_TRUE(get_nil() == are_equal(s, i));
    ASSERT_TRUE(get_nil() == are_equal(s, flt));
    ASSERT_TRUE(get_nil() != are_equal(s, s));
    ASSERT_TRUE(get_nil() == are_equal(s, o));

    ASSERT_TRUE(get_nil() == are_equal(o, get_nil()));
    ASSERT_TRUE(get_nil() == are_equal(o, fn));
    ASSERT_TRUE(get_nil() == are_equal(o, sym));
    ASSERT_TRUE(get_nil() == are_equal(o, kw));
    ASSERT_TRUE(get_nil() == are_equal(o, i));
    ASSERT_TRUE(get_nil() == are_equal(o, flt));
    ASSERT_TRUE(get_nil() == are_equal(o, s));
    ASSERT_TRUE(get_nil() != are_equal(o, o));
}

TEST(equality_test, should_compare_integers)
{
    ASSERT_TRUE(get_nil() != are_equal(create_int64(10), create_int64(10)));
    ASSERT_TRUE(get_nil() == are_equal(create_int64(10), create_int64(20)));
}

TEST(equality_test, should_compare_floats)
{
    ASSERT_TRUE(get_nil() != are_equal(create_float64(10.25), create_float64(10.25)));
    ASSERT_TRUE(get_nil() == are_equal(create_float64(10.25), create_float64(20.25)));
}

TEST(equality_test, should_compare_strings)
{
    ASSERT_TRUE(get_nil() != are_equal(create_string(""), create_string("")));
    ASSERT_TRUE(get_nil() != are_equal(create_string("abc"), create_string("abc")));
    ASSERT_TRUE(get_nil() == are_equal(create_string("abcd"), create_string("abc")));
    ASSERT_TRUE(get_nil() == are_equal(create_string("abcd"), create_string("abce")));
    ASSERT_TRUE(get_nil() == are_equal(create_string(std::string("ab\0cd", 5)), create_string(std::string("ab\0ce", 5))));
}

TEST(equality_test, objects_should_not_be_equal)
{
    auto type = create_symbol("cleo.equality.test", "sometype");
    ASSERT_TRUE(get_nil() == are_equal(create_object(type, nullptr, 0), create_object(type, nullptr, 0)));
}

}
}
