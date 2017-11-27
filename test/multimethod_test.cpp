#include <cleo/multimethod.hpp>
#include <cleo/eval.hpp>
#include <cleo/var.hpp>
#include "util.hpp"
#include <gtest/gtest.h>

namespace cleo
{
namespace test
{

TEST(multimethod_test, should_dispatch_to_the_right_method)
{
    auto name = create_symbol("cleo.multimethod.test", "ab");
    auto dispatchFn = create_native_function([](const Value *args, std::uint8_t) { return args[0]; });
    define_multimethod(name, dispatchFn);
    define_method(name, create_int64(100), create_native_function([](const Value *args, std::uint8_t) { return args[1]; }));
    define_method(name, create_int64(200), create_native_function([](const Value *args, std::uint8_t) { return args[2]; }));

    auto val1 = create_int64(77);
    auto val2 = create_int64(88);
    ASSERT_TRUE(val1 == eval(list(name, create_int64(100), val1, val2)));
    ASSERT_TRUE(val2 == eval(list(name, create_int64(200), val1, val2)));
}

TEST(multimethod_test, should_fail_when_a_matching_method_does_not_exist)
{
    auto name = create_symbol("cleo.multimethod.test", "one");
    auto dispatchFn = create_native_function([](const Value *args, std::uint8_t) { return args[0]; });
    define_multimethod(name, dispatchFn);
    define_method(name, create_int64(100), create_native_function([](const Value *, std::uint8_t) { return nil; }));
    try
    {
        eval(list(name, create_int64(200)));
        FAIL() << "expected an exception";
    }
    catch (const illegal_argument& )
    {
    }
}

}
}
