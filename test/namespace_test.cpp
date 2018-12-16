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
    auto a = create_symbol("a");
    auto b = create_symbol("b");
    auto c = create_symbol("c");
    auto a1 = create_symbol("cleo.namespace.test1", "a"), a2 = create_symbol("cleo.namespace.test2", "a");
    auto b1 = create_symbol("cleo.namespace.test1", "b"), b2 = create_symbol("cleo.namespace.test2", "b");
    auto c1 = create_symbol("cleo.namespace.test1", "c"), c2 = create_symbol("cleo.namespace.test2", "c");
    auto ka = create_keyword("a");
    auto kb = create_keyword("b");
    auto kc = create_keyword("c");

    in_ns(ns1);
    define(a1, ka);
    EXPECT_EQ_VALS(ka, lookup(a));
    EXPECT_EQ_VALS(ka, lookup(a1));

    in_ns(ns2);
    refer(ns1);
    EXPECT_EQ_VALS(ka, lookup(a));
    EXPECT_EQ_VALS(ka, lookup(a1));
    EXPECT_ANY_THROW(lookup(a2));

    in_ns(ns1);
    define(b1, kb);
    EXPECT_EQ_VALS(ka, lookup(a));
    EXPECT_EQ_VALS(ka, lookup(a1));
    EXPECT_EQ_VALS(kb, lookup(b));
    EXPECT_EQ_VALS(kb, lookup(b1));

    in_ns(ns2);
    define(c2, kc);
    EXPECT_EQ_VALS(ka, lookup(a));
    EXPECT_EQ_VALS(ka, lookup(a1));
    EXPECT_ANY_THROW(lookup(a2));
    EXPECT_ANY_THROW(lookup(b));
    EXPECT_EQ_VALS(kb, lookup(b1));
    EXPECT_ANY_THROW(lookup(b2));
    EXPECT_EQ_VALS(kc, lookup(c));
    EXPECT_ANY_THROW(lookup(c1));
    EXPECT_EQ_VALS(kc, lookup(c2));

    refer(ns1);
    EXPECT_EQ_VALS(ka, lookup(a));
    EXPECT_EQ_VALS(ka, lookup(a1));
    EXPECT_ANY_THROW(lookup(a2));
    EXPECT_EQ_VALS(kb, lookup(b));
    EXPECT_EQ_VALS(kb, lookup(b1));
    EXPECT_ANY_THROW(lookup(b2));
    EXPECT_EQ_VALS(kc, lookup(c));
    EXPECT_ANY_THROW(lookup(c1));
    EXPECT_EQ_VALS(kc, lookup(c2));

    in_ns(ns1);
    EXPECT_EQ_VALS(ka, lookup(a));
    EXPECT_EQ_VALS(ka, lookup(a1));
    EXPECT_ANY_THROW(lookup(a2));
    EXPECT_EQ_VALS(kb, lookup(b));
    EXPECT_EQ_VALS(kb, lookup(b1));
    EXPECT_ANY_THROW(lookup(b2));
    EXPECT_ANY_THROW(lookup(c));
    EXPECT_ANY_THROW(lookup(c1));
    EXPECT_EQ_VALS(kc, lookup(c2));
}

TEST_F(namespace_test, alias_should_alias_another_namespace_in_the_current_namespace)
{
    auto ns1 = create_symbol("cleo.namespace.alias.test1");
    auto ns2 = create_symbol("cleo.namespace.alias.test2");
    auto ns3 = create_symbol("cleo.namespace.alias.test3");
    auto a1 = create_symbol("a1");
    auto a2 = create_symbol("a2");
    auto s1 = create_symbol("cleo.namespace.alias.test1", "s1");
    auto s2 = create_symbol("cleo.namespace.alias.test1", "s2");
    auto s3 = create_symbol("cleo.namespace.alias.test2", "s3");
    auto k1 = create_keyword("k1");
    auto k2 = create_keyword("k2");
    auto k3 = create_keyword("k3");

    in_ns(ns1);
    define(s1, k1);
    define(s2, k2);

    in_ns(ns2);
    define(s3, k3);

    in_ns(ns3);
    alias(a1, ns1);
    alias(a2, ns2);

    EXPECT_EQ_VALS(k1, lookup(create_symbol("a1", "s1")));
    EXPECT_EQ_VALS(k2, lookup(create_symbol("a1", "s2")));
    EXPECT_EQ_VALS(k3, lookup(create_symbol("a2", "s3")));

    in_ns(ns2);

    EXPECT_ANY_THROW(lookup(create_symbol("a1", "s1")));
}

TEST_F(namespace_test, define_should_add_name_and_ns_to_meta)
{
    auto ns = create_symbol("cleo.namespace.define.test");
    auto sym1 = create_symbol("cleo.namespace.define.test", "abc");
    auto sym2 = create_symbol("cleo.namespace.define.test", "xyz");
    in_ns(ns);
    ns = get_ns(ns);

    auto var = define(sym1, nil);
    auto meta = get_var_meta(var);
    EXPECT_EQ_VALS(name_symbol(sym1), map_get(meta, NAME_KEY));
    EXPECT_EQ_REFS(ns, map_get(meta, NS_KEY));

    Root vmeta{phmap(MACRO_KEY, TRUE, NAME_KEY, sym1)};
    var = define(sym2, nil, *vmeta);
    meta = get_var_meta(var);
    EXPECT_EQ_VALS(name_symbol(sym2), map_get(meta, NAME_KEY));
    EXPECT_EQ_VALS(TRUE, map_get(meta, MACRO_KEY));
    EXPECT_EQ_REFS(ns, map_get(meta, NS_KEY));
}

}
}
