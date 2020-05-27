#include <cleo/persistent_hash_set.hpp>
#include <cleo/multimethod.hpp>
#include "util.hpp"
#include <algorithm>

namespace cleo
{
namespace test
{

struct persistent_hash_set_test : Test
{
    Root HashString{create_dynamic_object_type("cleo.persistent_hash_set.test", "HashString")};

    static Force string_value(Value val)
    {
        Value s = get_dynamic_object_element(val, 0);
        std::string ss{get_string_ptr(s), get_string_size(s)};
        return create_int64(std::uint32_t(std::strtoull(ss.c_str(), nullptr, 32)));
    }

    static Value are_hash_strings_equal(Value left, Value right)
    {
        return are_equal(get_dynamic_object_element(left, 0), get_dynamic_object_element(right, 0));
    }

    static Force pr_str_hash_string(Value val)
    {
        return pr_str(get_dynamic_object_element(val, 0));
    }

    persistent_hash_set_test() : Test("cleo.persistent_hash_set.test")
    {
        Root f{create_native_function1<string_value, &OBJ_EQ>()};
        define_method(HASH_OBJ, *HashString, *f);

        f = create_native_function2<are_hash_strings_equal, &OBJ_EQ>();
        Root args{array(*HashString, *HashString)};
        define_method(OBJ_EQ, *args, *f);

        f = create_native_function1<pr_str_hash_string, &OBJ_EQ>();
        define_method(PR_STR_OBJ, *HashString, *f);
    }

    Force create_key(const std::string& s)
    {
        Root k{create_string(s)};
        return create_object1(*HashString, *k);
    }

    template <typename Step>
    void expect_in(const std::string& name, const Step& step, Value m, const std::string& key)
    {
        Root ek{create_key(key)};
        ASSERT_EQ_VALS(*ek, persistent_hash_set_get(m, *ek))
            << name << " " << testing::PrintToString(step) << " key: " << key;
        ASSERT_EQ_REFS(TRUE, persistent_hash_set_contains(m, *ek))
            << name << " " << testing::PrintToString(step) << " key: " << key;
    }

    template <typename Step>
    void expect_not_in(const std::string& name, const Step& step, Value m, const std::string& key, Value k)
    {
        ASSERT_EQ_REFS(nil, persistent_hash_set_get(m, k))
            << name << " " << testing::PrintToString(step) << " key: " << key;
        ASSERT_EQ_REFS(nil, persistent_hash_set_contains(m, k))
            << name << " " << testing::PrintToString(step) << " key: " << key;
    }

    template <typename Step>
    void expect_not_in(const std::string& name, const Step& step, Value m, const std::string& key)
    {
        Root k{create_key(key)};
        expect_not_in(name, step, m, key, *k);
    }

    template <typename Step>
    void expect_nil_not_in(const std::string& name, const Step& step, Value m)
    {
        expect_not_in(name, step, m, "nil", nil);
    }

    template <typename Step>
    void check_pm(const std::string& name, const Step& step, Value m, const std::unordered_set<std::string>& expected)
    {
        for (const auto& ekv : expected)
            expect_in(name, step, m, ekv);
        ASSERT_EQ(Int64(expected.size()), get_persistent_hash_set_size(m))
            << name << " " << testing::PrintToString(step);
    }

    void test_conj(std::vector<std::string> ks)
    {
        Root pm{create_persistent_hash_set()};
        std::unordered_set<std::string> expected;

        for (auto const& k : ks)
            ASSERT_NO_FATAL_FAILURE({
                std::string bad_key = k + "*";
                Root ek{create_key(k)};
                Root new_pm{persistent_hash_set_conj(*pm, *ek)};

                check_pm("original", k, *pm, expected);
                if (expected.count(k) == 0)
                    expect_not_in("original", k, *pm, k);

                check_optimal_structure(*new_pm);

                pm = *new_pm;
                expected.insert(k);

                check_pm("new", k, *pm, expected);

                expect_not_in("new", k, *pm, bad_key);
                expect_nil_not_in("new", k, *pm);
            });
    }

    void test_conj_permutations(std::vector<std::string> ks)
    {
        do
        {
            ASSERT_NO_FATAL_FAILURE(test_conj(ks)) << testing::PrintToString(ks);
        }
        while (std::next_permutation(begin(ks), end(ks)));
    }

    void test_disj_permutations(std::vector<std::string> ks)
    {
        do
        {
            ASSERT_NO_FATAL_FAILURE(test_disj(ks, ks)) << testing::PrintToString(ks);
        }
        while (std::next_permutation(begin(ks), end(ks)));
    }

    void test_disj(std::vector<std::string> initial, std::vector<std::string> ks)
    {
        Root pm{create_persistent_hash_set()};
        std::unordered_set<std::string> expected;
        for (auto const& k : initial)
        {
            Root ek{create_key(k)};
            pm = persistent_hash_set_conj(*pm, *ek);
            expected.insert(k);
        }

        for (auto const& k : ks)
            ASSERT_NO_FATAL_FAILURE({
                std::string bad_key = k + "*";
                Root kval{create_key(k)};
                Root new_pm{persistent_hash_set_disj(*pm, *kval)};

                check_pm("original", k, *pm, expected);
                if (expected.count(k) != 0)
                    expect_in("original", k, *pm, k);

                check_optimal_structure(*new_pm);

                pm = *new_pm;
                expected.erase(k);

                expect_not_in("new", k, *pm, k);
                check_pm("new", k, *pm, expected);

                expect_not_in("new", k, *pm, bad_key);
                expect_nil_not_in("new", k, *pm);
            });
    }

    Int64 node_arity(Value node)
    {
        if (!get_value_type(node).is(*type::PersistentHashSetArrayNode))
            return 0;
        return __builtin_popcount(std::uint32_t(std::uint64_t(get_dynamic_object_int(node, 0)) >> 32));
    }

    Int64 payload_arity(Value node)
    {
        if (!get_value_type(node).is(*type::PersistentHashSetArrayNode))
            return 0;
        return __builtin_popcount(std::uint32_t(get_dynamic_object_int(node, 0)));
    }

    Int64 branch_size(Value node)
    {
        if (get_value_type(node).is(*type::PersistentHashSetCollisionNode))
        {
            return get_dynamic_object_size(node);
        }
        if (get_value_type(node).is(*type::PersistentHashSetArrayNode))
        {
            std::uint64_t value_node_map = get_dynamic_object_int(node, 0);
            std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
            std::uint32_t node_map{static_cast<std::uint32_t>(value_node_map >> 32)};
            auto node_size = get_dynamic_object_size(node);
            Int64 size = __builtin_popcount(value_map);
            for (Int64 i = 0; i < __builtin_popcount(node_map); ++i)
                size += branch_size(get_dynamic_object_element(node, node_size - i - 1));
            return size;
        }
        return 1;
    }

    void check_node_invariant(Value node)
    {
        ASSERT_GE(branch_size(node), 2 * node_arity(node) + payload_arity(node));

        if (get_value_type(node).is(*type::PersistentHashSetArrayNode))
        {
            std::uint64_t value_node_map = get_dynamic_object_int(node, 0);
            std::uint32_t node_map{static_cast<std::uint32_t>(value_node_map >> 32)};
            auto node_size = get_dynamic_object_size(node);
            for (Int64 i = 0; i < __builtin_popcount(node_map); ++i)
                check_node_invariant(get_dynamic_object_element(node, node_size - i - 1));
        }
    }

    void check_optimal_structure(Value set)
    {
        ASSERT_EQ_REFS(*type::PersistentHashSet, get_value_type(set));
        check_node_invariant(get_static_object_element(set, 1));
    }

    Force create_set(std::vector<std::string> ks)
    {
        Root m{create_persistent_hash_set()};
        for (auto& k : ks)
        {
            Root ek{create_key(k)};
            m = persistent_hash_set_conj(*m, *ek);
        }
        return *m;
    }

    void test_equality(std::vector<std::vector<std::string>> kss)
    {
        Roots sets{kss.size()};

        for (decltype(kss.size()) i = 0; i != kss.size(); ++i)
            sets.set(i, create_set(kss[i]));

        auto sorted_kss = kss;
        for (auto& ks : sorted_kss)
            std::sort(begin(ks), end(ks));

        for (decltype(kss.size()) i = 0; i != kss.size(); ++i)
            for (decltype(kss.size()) j = 0; j != kss.size(); ++j)
            {
                auto expected = sorted_kss[i] == sorted_kss[j] ? TRUE : nil;

                EXPECT_EQ_REFS(expected, are_persistent_hash_sets_equal(sets[i], sets[j])) << "left: " << testing::PrintToString(kss[i]) << " right: " << testing::PrintToString(kss[j]);
            }
    }

    void check_sequence(std::vector<std::string> ks)
    {
        Root m{create_set(ks)};
        Root s{persistent_hash_set_seq(*m)};
        m = nil;
        auto size = ks.size();
        for (decltype(size) i = 0; i < size; ++i)
        {
            ASSERT_FALSE(s->is_nil()) << testing::PrintToString(ks[i]) << " " << testing::PrintToString(ks);
            Root k{create_key(ks[i])};
            auto entry{get_persistent_hash_set_seq_first(*s)};

            ASSERT_EQ_VALS(*k, entry) << testing::PrintToString(ks);

            s = get_persistent_hash_set_seq_next(*s);
        }
        ASSERT_EQ_REFS(nil, *s) << testing::PrintToString(ks);
    }
};

struct persistent_hash_set_permutation_test : persistent_hash_set_test
{
    Override<decltype(gc_frequency)> ovf{gc_frequency, 64};
};

TEST_F(persistent_hash_set_test, should_be_created_empty)
{
    Root m{create_persistent_hash_set()};
    ASSERT_EQ(0, get_persistent_hash_set_size(*m));
    ASSERT_FALSE(bool(persistent_hash_set_contains(*m, *ONE)));
    ASSERT_EQ_REFS(nil, persistent_hash_set_get(*m, *ONE));
}

TEST_F(persistent_hash_set_test, hash_should_return_the_key_value_from_base32)
{
    Root k{create_key("0")};
    EXPECT_EQ(0, hash_value(*k));
    k = create_key("v2f");
    EXPECT_EQ((31 * 32 + 2) * 32 + 15, hash_value(*k));
    k = create_key("vvvvvvv");
    EXPECT_EQ(~std::uint32_t(0), hash_value(*k)) << "should use only 32 bits";
    k = create_key("3vvvvvv");
    EXPECT_EQ(~std::uint32_t(0), hash_value(*k));
}

TEST_F(persistent_hash_set_test, hash_should_ignore_any_text_after_the_value)
{
    Root k{create_key("0$")};
    EXPECT_EQ(0, hash_value(*k));
    k = create_key("v2f-abc");
    EXPECT_EQ((31 * 32 + 2) * 32 + 15, hash_value(*k));
}

TEST_F(persistent_hash_set_test, conj_single_values)
{
    test_conj({
        "v",
        "v"});
}

TEST_F(persistent_hash_set_test, conj_root_collisions)
{
    test_conj({
        "v-a",
        "v-b",
        "v-b",
        "v-c",
        "v-a",
        "v-a",
        "v-b",
        "v-b",
        "v-c",
        "v-c",
    });
}

TEST_F(persistent_hash_set_test, conj_root_array_no_collisions)
{
    test_conj({
        "0-a",
        "1-a",
        "2-a",
        "2-a",
        "2-a",
        "3-a",
        "4-a",
        "5-a",
        "6-a",
        "7-a",
        "8-a",
        "9-a",
        "a-a",
        "b-a",
        "c-a",
        "d-a",
        "e-a",
        "f-a",
        "g-a",
        "h-a",
        "i-a",
        "j-a",
        "k-a",
        "l-a",
        "m-a",
        "n-a",
        "o-a",
        "p-a",
        "q-a",
        "r-a",
        "s-a",
        "t-a",
        "u-a",
        "v-a",

        "0-a",
        "1-a",
        "2-a",
        "h-a",
        "t-a",
        "u-a",
        "v-a",
    });
}

TEST_F(persistent_hash_set_test, conj_root_array_level_1_collisions)
{
    test_conj({
        "0-a",
        "1-a",
        "1-b",
        "1-c",
        "0-b",
        "0-c",
        "0-d",
        "2-a",
        "3-a",
        "4-a",
        "5-a",
        "6-a",
        "7-a",
        "8-a",
        "9-a",
        "a-a",
        "b-a",
        "c-a",
        "d-a",
        "e-a",
        "f-a",
        "g-a",
        "h-a",
        "i-a",
        "j-a",
        "k-a",
        "l-a",
        "m-a",
        "n-a",
        "o-a",
        "p-a",
        "q-a",
        "r-a",
        "s-a",
        "t-a",
        "u-a",
        "v-a",
        "v-b",
        "v-c",

        "0-a",
        "1-a",
        "1-b",
        "1-c",
        "0-b",
        "0-c",
        "0-d",
        "v-a",
        "v-b",
        "v-c",
    });
}

TEST_F(persistent_hash_set_test, conj_root_array_level_1_array)
{
    test_conj({
        "00-a",
        "10-a",
        "20-a",
        "30-a",
        "40-a",
        "50-a",
        "60-a",
        "70-a",
        "80-a",
        "90-a",
        "a0-a",
        "b0-a",
        "c0-a",
        "d0-a",
        "e0-a",
        "f0-a",
        "g0-a",
        "h0-a",
        "i0-a",
        "j0-a",
        "k0-a",
        "l0-a",
        "m0-a",
        "n0-a",
        "o0-a",
        "p0-a",
        "q0-a",
        "r0-a",
        "s0-a",
        "t0-a",
        "u0-a",
        "v0-a",

        "00-a",
        "10-a",
        "20-a",
        "u0-a",
        "v0-a",
    });

    test_conj({
        "00-a",
        "00-b",
        "00-c",
        "10-a",
        "20-a",

        "00-a",
        "00-b",
        "00-c",
        "10-a",
        "20-a",
    });

    test_conj({
        "00-a",
        "10-a",
        "20-a",
        "01-a",
        "11-a",
        "21-a",

        "00-a",
        "10-a",
        "20-a",
        "01-a",
        "11-a",
        "21-a",
    });
}

TEST_F(persistent_hash_set_test, conj_root_array_level_2_array)
{
    test_conj({
        "000-a",
        "100-a",
        "200-a",
        "300-a",
        "400-a",
        "500-a",
        "600-a",
        "700-a",
        "800-a",
        "900-a",
        "a00-a",
        "b00-a",
        "c00-a",
        "d00-a",
        "e00-a",
        "f00-a",
        "g00-a",
        "h00-a",
        "i00-a",
        "j00-a",
        "k00-a",
        "l00-a",
        "m00-a",
        "n00-a",
        "o00-a",
        "p00-a",
        "q00-a",
        "r00-a",
        "s00-a",
        "t00-a",
        "u00-a",
        "v00-a",

        "000-a",
        "100-a",
        "200-a",
        "u00-a",
        "v00-a",
    });

    test_conj({
        "000-a",
        "000-b",
        "000-c",
        "100-a",
        "200-a",

        "00-a",
        "00-b",
        "00-c",
        "10-a",
        "20-a",
    });

    test_conj({
        "000-a",
        "100-a",
        "200-a",
        "001-a",
        "101-a",
        "201-a",

        "000-a",
        "100-a",
        "200-a",
        "001-a",
        "101-a",
        "201-a",
    });
}

TEST_F(persistent_hash_set_permutation_test, conj_order)
{
    test_conj_permutations({
        "0310-a",
        "3210-a",
        "3210-b",
        "4210-a",
        "4210-b",
    });
    test_conj_permutations({
        "1-a",
        "100-a",
        "10000-a",
        "1000000-a",
        "1000000-b",
    });
}

TEST_F(persistent_hash_set_test, disj_empty)
{
    test_disj({}, {"0-x"});
}

TEST_F(persistent_hash_set_test, disj_single_value)
{
    test_disj({
        "0-a",
    }, {
        "0-b",
        "0-a",
    });
}

TEST_F(persistent_hash_set_test, disj_root_collisions)
{
    test_disj({
        "0-a",
        "0-b",
        "0-c",
        "0-d",
        "0-e",
    }, {
        "1-e",
        "0-e",
        "0-a",
        "0-c",
        "0-b",
        "0-d",
    });
}

TEST_F(persistent_hash_set_test, disj_root_array_no_collisions)
{
    test_disj({
        "0-a",
        "1-a",
        "2-a",
        "3-a",
        "4-a",
        "5-a",
        "6-a",
        "7-a",
        "8-a",
        "9-a",
        "a-a",
        "b-a",
        "c-a",
        "d-a",
        "e-a",
        "f-a",
        "g-a",
        "h-a",
        "i-a",
        "j-a",
        "k-a",
        "l-a",
        "m-a",
        "n-a",
        "o-a",
        "p-a",
        "q-a",
        "r-a",
        "s-a",
        "t-a",
        "u-a",
        "v-a",
    }, {
        "v-b",
        "0-b",

        "v-a",
        "0-a",

        "u-a",

        "1-a",
        "2-a",
        "3-a",
        "4-a",
        "5-a",
        "6-a",
        "7-a",
        "8-a",
        "9-a",
        "a-a",
        "b-a",
        "c-a",
        "d-a",
        "e-a",
        "f-a",
        "g-a",
        "h-a",
        "i-a",
        "j-a",
        "k-a",
        "l-a",
        "m-a",
        "n-a",
        "o-a",
        "p-a",
        "q-a",
        "r-a",
        "s-a",
        "t-a",
    });

    test_disj({
        "0-a",
        "1-a",
    }, {
        "0-a",
        "1-a"
    });

    test_disj({
        "0-a",
        "1-a",
    }, {
        "1-a",
        "0-a"
    });
}

TEST_F(persistent_hash_set_test, disj_root_array_level_1_collisions)
{
    test_disj({
        "0-a",
        "1-a",
        "1-b",
        "1-c",
        "0-b",
        "0-c",
        "0-d",
        "2-a",
        "3-a",
        "4-a",
        "5-a",
        "6-a",
        "7-a",
        "8-a",
        "9-a",
        "a-a",
        "b-a",
        "c-a",
        "d-a",
        "e-a",
        "f-a",
        "g-a",
        "h-a",
        "i-a",
        "j-a",
        "k-a",
        "l-a",
        "m-a",
        "n-a",
        "o-a",
        "p-a",
        "q-a",
        "r-a",
        "s-a",
        "t-a",
        "u-a",
        "v-a",
        "v-b",
        "v-c",
        "v-d",
    }, {
        "v-d",
        "v-b",
        "v-a",
        "v-c",
        "0-a",
        "0-b",
        "0-c",
        "0-d",
        "1-a",
        "1-b",
        "1-c",
        "2-a",
        "3-a",
        "4-a",
        "5-a",
        "6-a",
        "7-a",
        "8-a",
        "9-a",
        "a-a",
        "b-a",
        "c-a",
        "d-a",
        "e-a",
        "f-a",
        "g-a",
        "h-a",
        "i-a",
        "j-a",
        "k-a",
        "l-a",
        "m-a",
        "n-a",
        "o-a",
        "p-a",
        "q-a",
        "r-a",
        "s-a",
        "t-a",
        "u-a",
    });

    test_disj({
        "0-a",
        "0-b",
        "0-c",
        "0-d",
        "1-a",
    }, {
        "1-a",
        "0-a",
        "0-b",
        "0-c",
        "0-d",
    });

    test_disj({
        "0-a",
        "0-b",
        "0-c",
        "0-d",
        "1-a",
    }, {
        "0-a",
        "0-b",
        "0-c",
        "0-d",
        "1-a",
    });
}

TEST_F(persistent_hash_set_test, disj_root_array_level_1_array)
{
    test_disj({
        "00-a",
        "10-a",
        "20-a",
        "30-a",
        "40-a",
        "50-a",
        "60-a",
        "70-a",
        "80-a",
        "90-a",
        "a0-a",
        "b0-a",
        "c0-a",
        "d0-a",
        "e0-a",
        "f0-a",
        "g0-a",
        "h0-a",
        "i0-a",
        "j0-a",
        "k0-a",
        "l0-a",
        "m0-a",
        "n0-a",
        "o0-a",
        "p0-a",
        "q0-a",
        "r0-a",
        "s0-a",
        "t0-a",
        "u0-a",
        "v0-a",
    }, {
        "00-a",
        "v0-a",
        "10-a",
        "20-a",
        "30-a",
        "40-a",
        "50-a",
        "60-a",
        "70-a",
        "80-a",
        "90-a",
        "a0-a",
        "b0-a",
        "c0-a",
        "d0-a",
        "e0-a",
        "f0-a",
        "g0-a",
        "h0-a",
        "i0-a",
        "j0-a",
        "k0-a",
        "l0-a",
        "m0-a",
        "n0-a",
        "o0-a",
        "p0-a",
        "q0-a",
        "r0-a",
        "s0-a",
        "t0-a",
        "u0-a",
    });

    test_disj({
        "00-a",
        "00-b",
        "00-c",
        "10-a",
        "20-a",
    }, {
        "00-a",
        "00-b",
        "00-c",
        "10-a",
        "20-a",
    });

    test_disj({
        "00-a",
        "10-a",
        "20-a",
        "01-a",
        "11-a",
        "21-a",
    }, {
        "00-a",
        "10-a",
        "20-a",
        "01-a",
        "11-a",
        "21-a",
    });
}

TEST_F(persistent_hash_set_test, disj_root_array_level_2_array)
{
    test_disj({
        "000-a",
        "100-a",
        "200-a",
        "300-a",
        "400-a",
        "500-a",
        "600-a",
        "700-a",
        "800-a",
        "900-a",
        "a00-a",
        "b00-a",
        "c00-a",
        "d00-a",
        "e00-a",
        "f00-a",
        "g00-a",
        "h00-a",
        "i00-a",
        "j00-a",
        "k00-a",
        "l00-a",
        "m00-a",
        "n00-a",
        "o00-a",
        "p00-a",
        "q00-a",
        "r00-a",
        "s00-a",
        "t00-a",
        "u00-a",
        "v00-a",
    }, {
        "000-a",
        "v00-a",
        "100-a",
        "200-a",
        "300-a",
        "400-a",
        "500-a",
        "600-a",
        "700-a",
        "800-a",
        "900-a",
        "a00-a",
        "b00-a",
        "c00-a",
        "d00-a",
        "e00-a",
        "f00-a",
        "g00-a",
        "h00-a",
        "i00-a",
        "j00-a",
        "k00-a",
        "l00-a",
        "m00-a",
        "n00-a",
        "o00-a",
        "p00-a",
        "q00-a",
        "r00-a",
        "s00-a",
        "u00-a",
        "t00-a",
    });

    test_disj({
        "000-a",
        "000-b",
        "000-c",
        "100-a",
        "200-a",

        "00-a",
        "00-b",
        "00-c",
        "10-a",
        "20-a",
    }, {
        "000-a",
        "100-a",
        "200-a",
        "000-b",
        "000-c",
        "00-a",
        "00-b",
        "00-c",
        "10-a",
        "20-a",
    });
}

TEST_F(persistent_hash_set_permutation_test, disj_order)
{
    test_disj_permutations({
        "0310-a",
        "3210-a",
        "3210-b",
        "4210-a",
        "4210-b",
    });
    test_disj_permutations({
        "1-a",
        "100-a",
        "10000-a",
        "1000000-a",
        "1000000-b",
    });
}

TEST_F(persistent_hash_set_test, equality)
{
    test_equality({
        {},
        {"7-a"},
        {"5-a"},
        {"7-b"},
        {
            "0-a",
            "0-b",
            "0-c",
        }, {
            "1-a",
            "1-b",
            "1-c",
        }, {
            "0-a",
            "0-b",
            "0-d",
        }, {
            "0-a",
            "0-b",
            "0-c",
            "0-d",
        }, {
            "0-a",
            "0-b",
            "0-c",
        }, {
            "0-c",
            "0-a",
            "0-b",
        }, {
            "0-a",
            "1-a",
            "2-a",
            "3-a",
            "4-a",
            "5-a",
            "6-a",
            "7-a",
            "8-a",
            "9-a",
            "a-a",
            "b-a",
            "c-a",
            "d-a",
            "e-a",
            "f-a",
            "g-a",
            "h-a",
            "i-a",
            "j-a",
            "k-a",
            "l-a",
            "m-a",
            "n-a",
            "o-a",
            "p-a",
            "q-a",
            "r-a",
            "s-a",
            "t-a",
            "u-a",
            "v-a",
        }, {
            "0-a",
            "1-a",
            "2-a",
            "3-a",
            "4-a",
            "5-a",
            "6-a",
            "7-a",
            "8-a",
            "9-a",
            "a-a",
            "c-a",
            "d-a",
            "e-a",
            "f-a",
            "g-a",
            "h-a",
            "i-a",
            "j-a",
            "k-a",
            "l-a",
            "m-a",
            "n-a",
            "o-a",
            "p-a",
            "q-a",
            "r-a",
            "s-a",
            "t-a",
            "u-a",
            "v-a",
        }, {
            "0-b", // different key
            "1-a",
            "2-a",
            "3-a",
            "4-a",
            "5-a",
            "6-a",
            "7-a",
            "8-a",
            "9-a",
            "a-a",
            "b-a",
            "c-a",
            "d-a",
            "e-a",
            "f-a",
            "g-a",
            "h-a",
            "i-a",
            "j-a",
            "k-a",
            "l-a",
            "m-a",
            "n-a",
            "o-a",
            "p-a",
            "q-a",
            "r-a",
            "s-a",
            "t-a",
            "u-a",
            "v-a",
        }, {
            "0-a",
            "1-a",
            "1-b",
            "1-c",
            "0-b",
            "0-c",
            "0-d",
            "2-a",
            "3-a",
            "4-a",
            "5-a",
            "6-a",
            "7-a",
            "8-a",
            "9-a",
            "a-a",
            "b-a",
            "c-a",
            "d-a",
            "e-a",
            "f-a",
            "g-a",
            "h-a",
            "i-a",
            "j-a",
            "k-a",
            "l-a",
            "m-a",
            "n-a",
            "o-a",
            "p-a",
            "q-a",
            "r-a",
            "s-a",
            "t-a",
            "u-a",
            "v-a",
            "v-b",
            "v-c",
            "v-d",
        }, {
            "0-a",
            "1-a",
            "1-b",
            "1-d", // different key
            "0-b",
            "0-c",
            "0-d",
            "2-a",
            "3-a",
            "4-a",
            "5-a",
            "6-a",
            "7-a",
            "8-a",
            "9-a",
            "a-a",
            "b-a",
            "c-a",
            "d-a",
            "e-a",
            "f-a",
            "g-a",
            "h-a",
            "i-a",
            "j-a",
            "k-a",
            "l-a",
            "m-a",
            "n-a",
            "o-a",
            "p-a",
            "q-a",
            "r-a",
            "s-a",
            "t-a",
            "u-a",
            "v-a",
            "v-b",
            "v-c",
            "v-d",
        }, {
            "00-a",
            "10-a",
            "20-a",
            "30-a",
            "40-a",
            "50-a",
            "60-a",
            "70-a",
            "80-a",
            "90-a",
            "a0-a",
            "b0-a",
            "c0-a",
            "d0-a",
            "e0-a",
            "f0-a",
            "g0-a",
            "h0-a",
            "i0-a",
            "j0-a",
            "k0-a",
            "l0-a",
            "m0-a",
            "n0-a",
            "o0-a",
            "p0-a",
            "q0-a",
            "r0-a",
            "s0-a",
            "t0-a",
            "u0-a",
            "v0-a",
        }, {
            "00-a",
            "10-a",
            "20-a",
            "30-a",
            "40-a",
            "50-a",
            "60-a",
            "70-a",
            "80-a",
            "90-a",
            "a0-a",
            "b0-a",
            "c0-a",
            "d0-a",
            "e0-a",
            "f0-a",
            "g0-a",
            "h0-a",
            "i0-a",
            "j0-a",
            "k0-a",
            "l0-a",
            "m0-a",
            "n0-a",
            "o0-a",
            "p0-a",
            "q0-a",
            "r0-a",
            "s0-a",
            "t0-a",
            "u0-a",
            "v0-b", // different key
        }, {
            "000-a",
            "000-b",
            "000-c",
            "100-a",
            "200-a",
            "00-a",
            "00-b",
            "00-c",
            "10-a",
            "20-a",
        }, {
            "000-a",
            "000-b",
            "000-d", // different key
            "100-a",
            "200-a",
            "00-a",
            "00-b",
            "00-c",
            "10-a",
            "20-a",
        }
    });
}

TEST_F(persistent_hash_set_test, sequence)
{
    check_sequence({});
    check_sequence({"7-a"});
    check_sequence({
        "7-a",
        "7-b",
    });
    check_sequence({
        "7-a",
        "7-b",
        "7-c",
        "7-d",
    });
    check_sequence({
        "0-a",
        "1-a",
        "2-a",
        "3-a",
        "4-a",
        "c-a",
        "d-a",
        "e-a",
        "f-a",
        "g-a",
        "n-a",
        "o-a",
        "p-a",
        "q-a",
        "r-a",
        "s-a",
        "t-a",
        "u-a",
        "v-a",
    });
    check_sequence({
        "2-a",
        "3-a",
        "4-a",
        "1-a",
        "1-b",
        "1-c",
        "0-a",
        "0-b",
        "0-c",
        "0-d",
    });
    check_sequence({
        "2-a",
        "2-b",
        "2-c",
        "1-a",
        "1-b",
        "1-c",
    });
    check_sequence({
        "000-a",
        "100-a",
        "200-a",
        "300-a",
        "400-a",
        "500-a",
        "600-a",
        "700-a",
    });
    check_sequence({
        "000-a",
        "100-a",
        "200-a",
        "300-a",
        "400-a",
        "500-a",
        "600-a",
        "700-a",
    });
    check_sequence({
        "2-a",
        "010-a",
        "110-a",
        "210-a",
        "000-a",
        "100-a",
        "200-a",
        "300-a",
        "400-a",
        "500-a",
        "600-a",
        "700-a",
    });
    check_sequence({
        "021-a",
        "121-a",
        "221-a",
        "011-a",
        "111-a",
        "211-a",
        "001-a",
        "101-a",
        "201-a",
        "020-a",
        "120-a",
        "220-a",
        "010-a",
        "110-a",
        "210-a",
        "000-a",
        "100-a",
        "200-a",
    });
    check_sequence({
        "21-a",
        "21-b",
        "21-c",
        "11-a",
        "11-b",
        "11-c",
        "01-a",
        "01-b",
        "01-c",
        "20-a",
        "20-b",
        "20-c",
        "10-a",
        "10-b",
        "10-c",
        "00-a",
        "00-b",
        "00-c",
    });
}

}
}
