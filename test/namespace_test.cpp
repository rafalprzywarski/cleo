#include <cleo/global.hpp>
#include <cleo/eval.hpp>
#include <gtest/gtest.h>
#include "util.hpp"

namespace cleo
{
namespace test
{

struct namespace_test : Test
{
    namespace_test() : Test("cleo.namespace.test") { }
};

TEST_F(namespace_test, refer_should_copy_var_refs_from_another_namespace)
{
    auto ns1 = create_symbol("cleo.namespace.test1");
    auto ns2 = create_symbol("cleo.namespace.test2");
    auto a1 = create_symbol("cleo.namespace.test1", "a"), a2 = create_symbol("cleo.namespace.test2", "a");
    auto b1 = create_symbol("cleo.namespace.test1", "b"), b2 = create_symbol("cleo.namespace.test2", "b");
    auto c1 = create_symbol("cleo.namespace.test1", "c"), c2 = create_symbol("cleo.namespace.test2", "c");
    auto ka = create_keyword("a");
    auto kb = create_keyword("b");
    auto kc = create_keyword("c");

    in_ns(ns1);
    define(a1, ka);
    EXPECT_EQ_VALS(ka, lookup(a1));

    in_ns(ns2);
    refer(ns1);
    EXPECT_EQ_VALS(ka, lookup(a2));

    in_ns(ns1);
    define(b1, kb);
    EXPECT_EQ_VALS(ka, lookup(a1));
    EXPECT_EQ_VALS(kb, lookup(b1));

    in_ns(ns2);
    define(c2, kc);
    EXPECT_EQ_VALS(ka, lookup(a2));
    ASSERT_ANY_THROW(lookup(b2));
    EXPECT_EQ_VALS(kc, lookup(c2));

    refer(ns1);
    EXPECT_EQ_VALS(ka, lookup(a2));
    EXPECT_EQ_VALS(kb, lookup(b2));
    EXPECT_EQ_VALS(kc, lookup(c2));

    in_ns(ns1);
    EXPECT_EQ_VALS(ka, lookup(a1));
    EXPECT_EQ_VALS(kb, lookup(b1));
    ASSERT_ANY_THROW(lookup(c1));
}

}
}
