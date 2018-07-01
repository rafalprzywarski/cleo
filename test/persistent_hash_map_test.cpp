#include <cleo/persistent_hash_map.hpp>
#include <cleo/multimethod.hpp>
#include "util.hpp"
#include <algorithm>

namespace cleo
{
namespace test
{

struct persistent_hash_map_test : Test
{
    Root HashString{create_type("cleo.persistent_hash_map.test", "HashString")};

    static Force string_value(Value val)
    {
        Value s = get_object_element(val, 0);
        std::string ss{get_string_ptr(s), get_string_len(s)};
        return create_int64(std::uint32_t(std::strtoull(ss.c_str(), nullptr, 32)));
    }

    static Value are_hash_strings_equal(Value left, Value right)
    {
        return are_equal(get_object_element(left, 0), get_object_element(right, 0));
    }

    static Force pr_str_hash_string(Value val)
    {
        return pr_str(get_object_element(val, 0));
    }

    persistent_hash_map_test() : Test("cleo.persistent_hash_map.test")
    {
        Root f{create_native_function1<string_value>()};
        define_method(HASH_OBJ, *HashString, *f);

        f = create_native_function2<are_hash_strings_equal>();
        Root args{array(*HashString, *HashString)};
        define_method(OBJ_EQ, *args, *f);

        f = create_native_function1<pr_str_hash_string>();
        define_method(PR_STR_OBJ, *HashString, *f);
    }

    Force create_key(const std::string& s)
    {
        Root k{create_string(s)};
        return create_object1(*HashString, *k);
    }

    template <typename Step>
    void expect_in(const std::string& name, const Step& step, Value m, const std::string& key, int value)
    {
        Root ek{create_key(key)};
        Root ev{create_int64(value)};
        ASSERT_EQ_VALS(*ev, persistent_hash_map_get(m, *ek))
            << name << " " << testing::PrintToString(step) << " key: " << key;
        ASSERT_EQ_REFS(TRUE, persistent_hash_map_contains(m, *ek))
            << name << " " << testing::PrintToString(step) << " key: " << key;
    }

    template <typename Step>
    void expect_not_in(const std::string& name, const Step& step, Value m, const std::string& key, Value k)
    {
        ASSERT_EQ_REFS(nil, persistent_hash_map_get(m, k))
            << name << " " << testing::PrintToString(step) << " key: " << key;
        ASSERT_EQ_REFS(nil, persistent_hash_map_contains(m, k))
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
    void check_pm(const std::string& name, const Step& step, Value m, const std::unordered_map<std::string, int>& expected)
    {
        for (const auto& ekv : expected)
            expect_in(name, step, m, ekv.first, ekv.second);
        ASSERT_EQ(Int64(expected.size()), get_persistent_hash_map_size(m))
            << name << " " << testing::PrintToString(step);
    }

    void test_assoc(std::vector<std::pair<std::string, int>> kvs)
    {
        Root pm{create_persistent_hash_map()};
        std::unordered_map<std::string, int> expected;

        for (auto const& kv : kvs)
            ASSERT_NO_FATAL_FAILURE({
                std::string bad_key = kv.first + "*";
                Root k{create_key(kv.first)};
                Root v{create_int64(kv.second)};
                Root new_pm{persistent_hash_map_assoc(*pm, *k, *v)};

                check_pm("original", kv, *pm, expected);
                if (expected.count(kv.first) == 0)
                    expect_not_in("original", kv, *pm, kv.first);

                check_optimal_structure(*new_pm);

                pm = *new_pm;
                expected[kv.first] = kv.second;

                check_pm("new", kv, *pm, expected);

                expect_not_in("new", kv, *pm, bad_key);
                expect_nil_not_in("new", kv, *pm);
            });
    }

    void test_assoc_permutations(std::vector<std::pair<std::string, int>> kvs)
    {
        do
        {
            ASSERT_NO_FATAL_FAILURE(test_assoc(kvs)) << testing::PrintToString(kvs);
        }
        while (std::next_permutation(begin(kvs), end(kvs)));
    }

    void test_dissoc_permutations(std::vector<std::pair<std::string, int>> kvs, std::vector<std::string> ks)
    {
        do
        {
            ASSERT_NO_FATAL_FAILURE(test_dissoc(kvs, ks)) << testing::PrintToString(kvs) << " " << testing::PrintToString(ks);
        }
        while (std::next_permutation(begin(ks), end(ks)));
    }

    void test_dissoc(std::vector<std::pair<std::string, int>> initial, std::vector<std::string> ks)
    {
        Root pm{create_persistent_hash_map()};
        std::unordered_map<std::string, int> expected;
        for (auto const& kv : initial)
        {
            Root k{create_key(kv.first)};
            Root v{create_int64(kv.second)};
            pm = persistent_hash_map_assoc(*pm, *k, *v);
            expected[kv.first] = kv.second;
        }

        for (auto const& k : ks)
            ASSERT_NO_FATAL_FAILURE({
                std::string bad_key = k + "*";
                Root kval{create_key(k)};
                Root new_pm{persistent_hash_map_dissoc(*pm, *kval)};

                check_pm("original", k, *pm, expected);
                if (expected.count(k) != 0)
                    expect_in("original", k, *pm, k, expected.at(k));

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
        if (!get_value_type(node).is(*type::PersistentHashMapArrayNode))
            return 0;
        return __builtin_popcount(std::uint32_t(std::uint64_t(get_object_int(node, 0)) >> 32));
    }

    Int64 payload_arity(Value node)
    {
        if (!get_value_type(node).is(*type::PersistentHashMapArrayNode))
            return 0;
        return __builtin_popcount(std::uint32_t(get_object_int(node, 0)));
    }

    Int64 branch_size(Value node)
    {
        if (get_value_type(node).is(*type::PersistentHashMapCollisionNode))
        {
            return get_object_size(node) / 2;
        }
        if (get_value_type(node).is(*type::PersistentHashMapArrayNode))
        {
            std::uint64_t value_node_map = get_object_int(node, 0);
            std::uint32_t value_map{static_cast<std::uint32_t>(value_node_map)};
            std::uint32_t node_map{static_cast<std::uint32_t>(value_node_map >> 32)};
            auto node_size = get_object_size(node);
            Int64 size = __builtin_popcount(value_map);
            for (Int64 i = 0; i < __builtin_popcount(node_map); ++i)
                size += branch_size(get_object_element(node, node_size - i - 1));
            return size;
        }
        return 1;
    }

    void check_node_invariant(Value node)
    {
        ASSERT_GE(branch_size(node), 2 * node_arity(node) + payload_arity(node));

        if (get_value_type(node).is(*type::PersistentHashMapArrayNode))
        {
            std::uint64_t value_node_map = get_object_int(node, 0);
            std::uint32_t node_map{static_cast<std::uint32_t>(value_node_map >> 32)};
            auto node_size = get_object_size(node);
            for (Int64 i = 0; i < __builtin_popcount(node_map); ++i)
                check_node_invariant(get_object_element(node, node_size - i - 1));
        }
    }

    void check_optimal_structure(Value map)
    {
        ASSERT_EQ_REFS(*type::PersistentHashMap, get_value_type(map));
        check_node_invariant(get_object_element(map, 0));
    }

    Force create_map(std::vector<std::pair<std::string, int>> kvs)
    {
        Root m{create_persistent_hash_map()};
        for (auto& kv : kvs)
        {
            Root k{create_key(kv.first)};
            Root v{create_int64(kv.second)};
            m = persistent_hash_map_assoc(*m, *k, *v);
        }
        return *m;
    }

    void test_equality(std::vector<std::vector<std::pair<std::string, int>>> kvss)
    {
        Roots maps{kvss.size()};

        for (decltype(kvss.size()) i = 0; i != kvss.size(); ++i)
            maps.set(i, create_map(kvss[i]));

        auto sorted_kvss = kvss;
        for (auto& kvs : sorted_kvss)
            std::sort(begin(kvs), end(kvs));

        for (decltype(kvss.size()) i = 0; i != kvss.size(); ++i)
            for (decltype(kvss.size()) j = 0; j != kvss.size(); ++j)
            {
                auto expected = sorted_kvss[i] == sorted_kvss[j] ? TRUE : nil;

                EXPECT_EQ_REFS(expected, are_persistent_hash_maps_equal(maps[i], maps[j])) << "left: " << testing::PrintToString(kvss[i]) << " right: " << testing::PrintToString(kvss[j]);
            }
    }

    void check_sequence(std::vector<std::pair<std::string, int>> kvs)
    {
        Root m{create_map(kvs)};
        Root s{persistent_hash_map_seq(*m)};
        m = nil;
        auto size = kvs.size();
        for (decltype(size) i = 0; i < size; ++i)
        {
            ASSERT_FALSE(s->is_nil()) << testing::PrintToString(kvs[i]) << " " << testing::PrintToString(kvs);
            Root k{create_key(kvs[i].first)};
            Root v{create_int64(kvs[i].second)};
            Root expected{array(*k, *v)};
            auto entry{get_persistent_hash_map_seq_first(*s)};

            ASSERT_EQ_REFS(*type::Array, get_value_type(entry)) << testing::PrintToString(kvs);
            ASSERT_EQ_VALS(*expected, entry) << testing::PrintToString(kvs);

            s = get_persistent_hash_map_seq_next(*s);
        }
        ASSERT_EQ_REFS(nil, *s) << testing::PrintToString(kvs);
    }
};

struct persistent_hash_map_permutation_test : persistent_hash_map_test
{
    Override<decltype(gc_frequency)> ovf{gc_frequency, 64};
};

TEST_F(persistent_hash_map_test, should_be_created_empty)
{
    Root m{create_persistent_hash_map()};
    ASSERT_EQ(0, get_persistent_hash_map_size(*m));
    ASSERT_FALSE(bool(persistent_hash_map_contains(*m, *ONE)));
    ASSERT_EQ_REFS(nil, persistent_hash_map_get(*m, *ONE));
}

TEST_F(persistent_hash_map_test, hash_should_return_the_key_value_from_base32)
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

TEST_F(persistent_hash_map_test, hash_should_ignore_any_text_after_the_value)
{
    Root k{create_key("0$")};
    EXPECT_EQ(0, hash_value(*k));
    k = create_key("v2f-abc");
    EXPECT_EQ((31 * 32 + 2) * 32 + 15, hash_value(*k));
}

TEST_F(persistent_hash_map_test, assoc_single_values)
{
    test_assoc({
        {"v", 1},
        {"v", 2},
        {"v", 3}});
}

TEST_F(persistent_hash_map_test, assoc_root_collisions)
{
    test_assoc({
        {"v-a", 10},
        {"v-b", 20},
        {"v-b", 30},
        {"v-c", 40},
        {"v-a", 50},
        {"v-a", 50},
        {"v-b", 60},
        {"v-b", 60},
        {"v-c", 70},
        {"v-c", 70},
    });
}

TEST_F(persistent_hash_map_test, assoc_root_array_no_collisions)
{
    test_assoc({
        {"0-a", 10},
        {"1-a", 20},
        {"2-a", 30},
        {"2-a", 31},
        {"2-a", 32},
        {"3-a", 40},
        {"4-a", 50},
        {"5-a", 60},
        {"6-a", 70},
        {"7-a", 80},
        {"8-a", 90},
        {"9-a", 100},
        {"a-a", 110},
        {"b-a", 120},
        {"c-a", 130},
        {"d-a", 140},
        {"e-a", 150},
        {"f-a", 160},
        {"g-a", 170},
        {"h-a", 180},
        {"i-a", 190},
        {"j-a", 200},
        {"k-a", 210},
        {"l-a", 220},
        {"m-a", 230},
        {"n-a", 240},
        {"o-a", 250},
        {"p-a", 260},
        {"q-a", 270},
        {"r-a", 280},
        {"s-a", 290},
        {"t-a", 300},
        {"u-a", 310},
        {"v-a", 320},

        {"0-a", 11},
        {"1-a", 21},
        {"2-a", 31},
        {"h-a", 181},
        {"t-a", 301},
        {"u-a", 311},
        {"v-a", 321},
    });
}

TEST_F(persistent_hash_map_test, assoc_root_array_level_1_collisions)
{
    test_assoc({
        {"0-a", 10},
        {"1-a", 20},
        {"1-b", 21},
        {"1-c", 22},
        {"0-b", 11},
        {"0-c", 12},
        {"0-d", 13},
        {"2-a", 30},
        {"3-a", 40},
        {"4-a", 50},
        {"5-a", 60},
        {"6-a", 70},
        {"7-a", 80},
        {"8-a", 90},
        {"9-a", 100},
        {"a-a", 110},
        {"b-a", 120},
        {"c-a", 130},
        {"d-a", 140},
        {"e-a", 150},
        {"f-a", 160},
        {"g-a", 170},
        {"h-a", 180},
        {"i-a", 190},
        {"j-a", 200},
        {"k-a", 210},
        {"l-a", 220},
        {"m-a", 230},
        {"n-a", 240},
        {"o-a", 250},
        {"p-a", 260},
        {"q-a", 270},
        {"r-a", 280},
        {"s-a", 290},
        {"t-a", 300},
        {"u-a", 310},
        {"v-a", 320},
        {"v-b", 321},
        {"v-c", 322},

        {"0-a", 14},
        {"1-a", 23},
        {"1-b", 24},
        {"1-c", 25},
        {"0-b", 15},
        {"0-c", 16},
        {"0-d", 17},
        {"v-a", 323},
        {"v-b", 324},
        {"v-c", 325},
    });
}

TEST_F(persistent_hash_map_test, assoc_root_array_level_1_array)
{
    test_assoc({
        {"00-a", 10},
        {"10-a", 20},
        {"20-a", 30},
        {"30-a", 40},
        {"40-a", 50},
        {"50-a", 60},
        {"60-a", 70},
        {"70-a", 80},
        {"80-a", 90},
        {"90-a", 100},
        {"a0-a", 110},
        {"b0-a", 120},
        {"c0-a", 130},
        {"d0-a", 140},
        {"e0-a", 150},
        {"f0-a", 160},
        {"g0-a", 170},
        {"h0-a", 180},
        {"i0-a", 190},
        {"j0-a", 200},
        {"k0-a", 210},
        {"l0-a", 220},
        {"m0-a", 230},
        {"n0-a", 240},
        {"o0-a", 250},
        {"p0-a", 260},
        {"q0-a", 270},
        {"r0-a", 280},
        {"s0-a", 290},
        {"t0-a", 300},
        {"u0-a", 310},
        {"v0-a", 320},

        {"00-a", 11},
        {"10-a", 21},
        {"20-a", 31},
        {"u0-a", 311},
        {"v0-a", 321},
    });

    test_assoc({
        {"00-a", 10},
        {"00-b", 11},
        {"00-c", 12},
        {"10-a", 20},
        {"20-a", 30},

        {"00-a", 110},
        {"00-b", 111},
        {"00-c", 112},
        {"10-a", 120},
        {"20-a", 130},
    });

    test_assoc({
        {"00-a", 10},
        {"10-a", 20},
        {"20-a", 30},
        {"01-a", 40},
        {"11-a", 50},
        {"21-a", 60},

        {"00-a", 110},
        {"10-a", 120},
        {"20-a", 130},
        {"01-a", 140},
        {"11-a", 150},
        {"21-a", 160},
    });
}

TEST_F(persistent_hash_map_test, assoc_root_array_level_2_array)
{
    test_assoc({
        {"000-a", 10},
        {"100-a", 20},
        {"200-a", 30},
        {"300-a", 40},
        {"400-a", 50},
        {"500-a", 60},
        {"600-a", 70},
        {"700-a", 80},
        {"800-a", 90},
        {"900-a", 100},
        {"a00-a", 110},
        {"b00-a", 120},
        {"c00-a", 130},
        {"d00-a", 140},
        {"e00-a", 150},
        {"f00-a", 160},
        {"g00-a", 170},
        {"h00-a", 180},
        {"i00-a", 190},
        {"j00-a", 200},
        {"k00-a", 210},
        {"l00-a", 220},
        {"m00-a", 230},
        {"n00-a", 240},
        {"o00-a", 250},
        {"p00-a", 260},
        {"q00-a", 270},
        {"r00-a", 280},
        {"s00-a", 290},
        {"t00-a", 300},
        {"u00-a", 310},
        {"v00-a", 320},

        {"000-a", 11},
        {"100-a", 21},
        {"200-a", 31},
        {"u00-a", 311},
        {"v00-a", 321},
    });

    test_assoc({
        {"000-a", 10},
        {"000-b", 11},
        {"000-c", 12},
        {"100-a", 20},
        {"200-a", 30},

        {"00-a", 110},
        {"00-b", 111},
        {"00-c", 112},
        {"10-a", 120},
        {"20-a", 130},
    });

    test_assoc({
        {"000-a", 10},
        {"100-a", 20},
        {"200-a", 30},
        {"001-a", 40},
        {"101-a", 50},
        {"201-a", 60},

        {"000-a", 110},
        {"100-a", 120},
        {"200-a", 130},
        {"001-a", 140},
        {"101-a", 150},
        {"201-a", 160},
    });
}

TEST_F(persistent_hash_map_permutation_test, assoc_order)
{
    test_assoc_permutations({
        {"0310-a", 10},
        {"3210-a", 20},
        {"3210-b", 30},
        {"4210-a", 40},
        {"4210-b", 50},
    });
    test_assoc_permutations({
        {"1-a", 10},
        {"100-a", 30},
        {"10000-a", 50},
        {"1000000-a", 60},
        {"1000000-b", 70},
    });
}

TEST_F(persistent_hash_map_test, dissoc_empty)
{
    test_dissoc({}, {"0-x"});
}

TEST_F(persistent_hash_map_test, dissoc_single_value)
{
    test_dissoc({
        {"0-a", 10},
    }, {
        "0-b",
        "0-a",
    });
}

TEST_F(persistent_hash_map_test, dissoc_root_collisions)
{
    test_dissoc({
        {"0-a", 10},
        {"0-b", 20},
        {"0-c", 30},
        {"0-d", 40},
        {"0-e", 50},
    }, {
        "1-e",
        "0-e",
        "0-a",
        "0-c",
        "0-b",
        "0-d",
    });
}

TEST_F(persistent_hash_map_test, dissoc_root_array_no_collisions)
{
    test_dissoc({
        {"0-a", 10},
        {"1-a", 20},
        {"2-a", 30},
        {"3-a", 40},
        {"4-a", 50},
        {"5-a", 60},
        {"6-a", 70},
        {"7-a", 80},
        {"8-a", 90},
        {"9-a", 100},
        {"a-a", 110},
        {"b-a", 120},
        {"c-a", 130},
        {"d-a", 140},
        {"e-a", 150},
        {"f-a", 160},
        {"g-a", 170},
        {"h-a", 180},
        {"i-a", 190},
        {"j-a", 200},
        {"k-a", 210},
        {"l-a", 220},
        {"m-a", 230},
        {"n-a", 240},
        {"o-a", 250},
        {"p-a", 260},
        {"q-a", 270},
        {"r-a", 280},
        {"s-a", 290},
        {"t-a", 300},
        {"u-a", 310},
        {"v-a", 320},
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

    test_dissoc({
        {"0-a", 10},
        {"1-a", 20}
    }, {
        "0-a",
        "1-a"
    });

    test_dissoc({
        {"0-a", 10},
        {"1-a", 20}
    }, {
        "1-a",
        "0-a"
    });
}

TEST_F(persistent_hash_map_test, dissoc_root_array_level_1_collisions)
{
    test_dissoc({
        {"0-a", 10},
        {"1-a", 20},
        {"1-b", 21},
        {"1-c", 22},
        {"0-b", 11},
        {"0-c", 12},
        {"0-d", 13},
        {"2-a", 30},
        {"3-a", 40},
        {"4-a", 50},
        {"5-a", 60},
        {"6-a", 70},
        {"7-a", 80},
        {"8-a", 90},
        {"9-a", 100},
        {"a-a", 110},
        {"b-a", 120},
        {"c-a", 130},
        {"d-a", 140},
        {"e-a", 150},
        {"f-a", 160},
        {"g-a", 170},
        {"h-a", 180},
        {"i-a", 190},
        {"j-a", 200},
        {"k-a", 210},
        {"l-a", 220},
        {"m-a", 230},
        {"n-a", 240},
        {"o-a", 250},
        {"p-a", 260},
        {"q-a", 270},
        {"r-a", 280},
        {"s-a", 290},
        {"t-a", 300},
        {"u-a", 310},
        {"v-a", 320},
        {"v-b", 321},
        {"v-c", 322},
        {"v-d", 323},
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

    test_dissoc({
        {"0-a", 10},
        {"0-b", 11},
        {"0-c", 12},
        {"0-d", 13},
        {"1-a", 10},
    }, {
        "1-a",
        "0-a",
        "0-b",
        "0-c",
        "0-d",
    });

    test_dissoc({
        {"0-a", 10},
        {"0-b", 11},
        {"0-c", 12},
        {"0-d", 13},
        {"1-a", 10},
    }, {
        "0-a",
        "0-b",
        "0-c",
        "0-d",
        "1-a",
    });
}

TEST_F(persistent_hash_map_test, dissoc_root_array_level_1_array)
{
    test_dissoc({
        {"00-a", 10},
        {"10-a", 20},
        {"20-a", 30},
        {"30-a", 40},
        {"40-a", 50},
        {"50-a", 60},
        {"60-a", 70},
        {"70-a", 80},
        {"80-a", 90},
        {"90-a", 100},
        {"a0-a", 110},
        {"b0-a", 120},
        {"c0-a", 130},
        {"d0-a", 140},
        {"e0-a", 150},
        {"f0-a", 160},
        {"g0-a", 170},
        {"h0-a", 180},
        {"i0-a", 190},
        {"j0-a", 200},
        {"k0-a", 210},
        {"l0-a", 220},
        {"m0-a", 230},
        {"n0-a", 240},
        {"o0-a", 250},
        {"p0-a", 260},
        {"q0-a", 270},
        {"r0-a", 280},
        {"s0-a", 290},
        {"t0-a", 300},
        {"u0-a", 310},
        {"v0-a", 320},
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

    test_dissoc({
        {"00-a", 10},
        {"00-b", 11},
        {"00-c", 12},
        {"10-a", 20},
        {"20-a", 30},
    }, {
        "00-a",
        "00-b",
        "00-c",
        "10-a",
        "20-a",
    });

    test_dissoc({
        {"00-a", 10},
        {"10-a", 20},
        {"20-a", 30},
        {"01-a", 40},
        {"11-a", 50},
        {"21-a", 60},
    }, {
        "00-a",
        "10-a",
        "20-a",
        "01-a",
        "11-a",
        "21-a",
    });
}

TEST_F(persistent_hash_map_test, dissoc_root_array_level_2_array)
{
    test_dissoc({
        {"000-a", 10},
        {"100-a", 20},
        {"200-a", 30},
        {"300-a", 40},
        {"400-a", 50},
        {"500-a", 60},
        {"600-a", 70},
        {"700-a", 80},
        {"800-a", 90},
        {"900-a", 100},
        {"a00-a", 110},
        {"b00-a", 120},
        {"c00-a", 130},
        {"d00-a", 140},
        {"e00-a", 150},
        {"f00-a", 160},
        {"g00-a", 170},
        {"h00-a", 180},
        {"i00-a", 190},
        {"j00-a", 200},
        {"k00-a", 210},
        {"l00-a", 220},
        {"m00-a", 230},
        {"n00-a", 240},
        {"o00-a", 250},
        {"p00-a", 260},
        {"q00-a", 270},
        {"r00-a", 280},
        {"s00-a", 290},
        {"t00-a", 300},
        {"u00-a", 310},
        {"v00-a", 320},
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

    test_dissoc({
        {"000-a", 10},
        {"000-b", 11},
        {"000-c", 12},
        {"100-a", 20},
        {"200-a", 30},

        {"00-a", 110},
        {"00-b", 111},
        {"00-c", 112},
        {"10-a", 120},
        {"20-a", 130},
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

TEST_F(persistent_hash_map_permutation_test, dissoc_order)
{
    test_dissoc_permutations({
        {"0310-a", 10},
        {"3210-a", 20},
        {"3210-b", 30},
        {"4210-a", 40},
        {"4210-b", 50},
    }, {
        "0310-a",
        "3210-a",
        "3210-b",
        "4210-a",
        "4210-b",
    });
    test_dissoc_permutations({
        {"1-a", 10},
        {"100-a", 30},
        {"10000-a", 50},
        {"1000000-a", 60},
        {"1000000-b", 70},
    }, {
        "1-a",
        "100-a",
        "10000-a",
        "1000000-a",
        "1000000-b",
    });
}

TEST_F(persistent_hash_map_test, equality)
{
    test_equality({
        {},
        {{"7-a", 3}},
        {{"7-a", 8}},
        {{"5-a", 3}},
        {{"7-b", 3}},
        {
            {"0-a", 10},
            {"0-b", 20},
            {"0-c", 30},
        }, {
            {"1-a", 10},
            {"1-b", 20},
            {"1-c", 30},
        }, {
            {"0-a", 10},
            {"0-b", 20},
            {"0-d", 30},
        }, {
            {"0-a", 10},
            {"0-b", 20},
            {"0-c", 30},
            {"0-d", 40},
        }, {
            {"0-a", 10},
            {"0-b", 22},
            {"0-c", 30},
        }, {
            {"0-c", 30},
            {"0-a", 10},
            {"0-b", 20},
        }, {
            {"0-a", 10},
            {"1-a", 20},
            {"2-a", 30},
            {"3-a", 40},
            {"4-a", 50},
            {"5-a", 60},
            {"6-a", 70},
            {"7-a", 80},
            {"8-a", 90},
            {"9-a", 100},
            {"a-a", 110},
            {"b-a", 120},
            {"c-a", 130},
            {"d-a", 140},
            {"e-a", 150},
            {"f-a", 160},
            {"g-a", 170},
            {"h-a", 180},
            {"i-a", 190},
            {"j-a", 200},
            {"k-a", 210},
            {"l-a", 220},
            {"m-a", 230},
            {"n-a", 240},
            {"o-a", 250},
            {"p-a", 260},
            {"q-a", 270},
            {"r-a", 280},
            {"s-a", 290},
            {"t-a", 300},
            {"u-a", 310},
            {"v-a", 320},
        }, {
            {"0-a", 10},
            {"1-a", 20},
            {"2-a", 30},
            {"3-a", 40},
            {"4-a", 50},
            {"5-a", 60},
            {"6-a", 70},
            {"7-a", 80},
            {"8-a", 90},
            {"9-a", 100},
            {"a-a", 110},
            {"c-a", 130},
            {"d-a", 140},
            {"e-a", 150},
            {"f-a", 160},
            {"g-a", 170},
            {"h-a", 180},
            {"i-a", 190},
            {"j-a", 200},
            {"k-a", 210},
            {"l-a", 220},
            {"m-a", 230},
            {"n-a", 240},
            {"o-a", 250},
            {"p-a", 260},
            {"q-a", 270},
            {"r-a", 280},
            {"s-a", 290},
            {"t-a", 300},
            {"u-a", 310},
            {"v-a", 320},
        }, {
            {"0-b", 10}, // different key
            {"1-a", 20},
            {"2-a", 30},
            {"3-a", 40},
            {"4-a", 50},
            {"5-a", 60},
            {"6-a", 70},
            {"7-a", 80},
            {"8-a", 90},
            {"9-a", 100},
            {"a-a", 110},
            {"b-a", 120},
            {"c-a", 130},
            {"d-a", 140},
            {"e-a", 150},
            {"f-a", 160},
            {"g-a", 170},
            {"h-a", 180},
            {"i-a", 190},
            {"j-a", 200},
            {"k-a", 210},
            {"l-a", 220},
            {"m-a", 230},
            {"n-a", 240},
            {"o-a", 250},
            {"p-a", 260},
            {"q-a", 270},
            {"r-a", 280},
            {"s-a", 290},
            {"t-a", 300},
            {"u-a", 310},
            {"v-a", 320},
        }, {
            {"0-a", 10},
            {"1-a", 20},
            {"2-a", 30},
            {"3-a", 40},
            {"4-a", 50},
            {"5-a", 60},
            {"6-a", 70},
            {"7-a", 80},
            {"8-a", 90},
            {"9-a", 102}, // different value
            {"a-a", 110},
            {"b-a", 120},
            {"c-a", 130},
            {"d-a", 140},
            {"e-a", 150},
            {"f-a", 160},
            {"g-a", 170},
            {"h-a", 180},
            {"i-a", 190},
            {"j-a", 200},
            {"k-a", 210},
            {"l-a", 220},
            {"m-a", 230},
            {"n-a", 240},
            {"o-a", 250},
            {"p-a", 260},
            {"q-a", 270},
            {"r-a", 280},
            {"s-a", 290},
            {"t-a", 300},
            {"u-a", 310},
            {"v-a", 320},
        }, {
            {"0-a", 10},
            {"1-a", 20},
            {"2-a", 30},
            {"3-a", 40},
            {"4-a", 50},
            {"5-a", 60},
            {"6-a", 70},
            {"7-a", 80},
            {"8-a", 90},
            {"9-a", 100},
            {"a-a", 110},
            {"b-a", 120},
            {"c-a", 130},
            {"d-a", 140},
            {"e-a", 150},
            {"f-a", 160},
            {"g-a", 170},
            {"h-a", 180},
            {"i-a", 190},
            {"j-a", 200},
            {"k-a", 210},
            {"l-a", 220},
            {"m-a", 230},
            {"n-a", 240},
            {"o-a", 250},
            {"p-a", 260},
            {"q-a", 270},
            {"r-a", 280},
            {"s-a", 290},
            {"t-a", 300},
            {"u-a", 310},
            {"v-a", 322}, // different value
        }, {
            {"0-a", 10},
            {"1-a", 20},
            {"1-b", 21},
            {"1-c", 22},
            {"0-b", 11},
            {"0-c", 12},
            {"0-d", 13},
            {"2-a", 30},
            {"3-a", 40},
            {"4-a", 50},
            {"5-a", 60},
            {"6-a", 70},
            {"7-a", 80},
            {"8-a", 90},
            {"9-a", 100},
            {"a-a", 110},
            {"b-a", 120},
            {"c-a", 130},
            {"d-a", 140},
            {"e-a", 150},
            {"f-a", 160},
            {"g-a", 170},
            {"h-a", 180},
            {"i-a", 190},
            {"j-a", 200},
            {"k-a", 210},
            {"l-a", 220},
            {"m-a", 230},
            {"n-a", 240},
            {"o-a", 250},
            {"p-a", 260},
            {"q-a", 270},
            {"r-a", 280},
            {"s-a", 290},
            {"t-a", 300},
            {"u-a", 310},
            {"v-a", 320},
            {"v-b", 321},
            {"v-c", 322},
            {"v-d", 323},
        }, {
            {"0-a", 10},
            {"1-a", 20},
            {"1-b", 21},
            {"1-d", 22}, // different key
            {"0-b", 11},
            {"0-c", 12},
            {"0-d", 13},
            {"2-a", 30},
            {"3-a", 40},
            {"4-a", 50},
            {"5-a", 60},
            {"6-a", 70},
            {"7-a", 80},
            {"8-a", 90},
            {"9-a", 100},
            {"a-a", 110},
            {"b-a", 120},
            {"c-a", 130},
            {"d-a", 140},
            {"e-a", 150},
            {"f-a", 160},
            {"g-a", 170},
            {"h-a", 180},
            {"i-a", 190},
            {"j-a", 200},
            {"k-a", 210},
            {"l-a", 220},
            {"m-a", 230},
            {"n-a", 240},
            {"o-a", 250},
            {"p-a", 260},
            {"q-a", 270},
            {"r-a", 280},
            {"s-a", 290},
            {"t-a", 300},
            {"u-a", 310},
            {"v-a", 320},
            {"v-b", 321},
            {"v-c", 322},
            {"v-d", 323},
        }, {
            {"0-a", 10},
            {"1-a", 20},
            {"1-b", 21},
            {"1-c", 22},
            {"0-b", 11},
            {"0-c", 12},
            {"0-d", 13},
            {"2-a", 30},
            {"3-a", 40},
            {"4-a", 50},
            {"5-a", 60},
            {"6-a", 70},
            {"7-a", 80},
            {"8-a", 90},
            {"9-a", 100},
            {"a-a", 110},
            {"b-a", 120},
            {"c-a", 130},
            {"d-a", 140},
            {"e-a", 150},
            {"f-a", 160},
            {"g-a", 170},
            {"h-a", 180},
            {"i-a", 190},
            {"j-a", 200},
            {"k-a", 210},
            {"l-a", 220},
            {"m-a", 230},
            {"n-a", 240},
            {"o-a", 250},
            {"p-a", 260},
            {"q-a", 270},
            {"r-a", 280},
            {"s-a", 290},
            {"t-a", 300},
            {"u-a", 310},
            {"v-a", 320},
            {"v-b", 321},
            {"v-c", 322},
            {"v-d", 329}, // different value
        }, {
            {"00-a", 10},
            {"10-a", 20},
            {"20-a", 30},
            {"30-a", 40},
            {"40-a", 50},
            {"50-a", 60},
            {"60-a", 70},
            {"70-a", 80},
            {"80-a", 90},
            {"90-a", 100},
            {"a0-a", 110},
            {"b0-a", 120},
            {"c0-a", 130},
            {"d0-a", 140},
            {"e0-a", 150},
            {"f0-a", 160},
            {"g0-a", 170},
            {"h0-a", 180},
            {"i0-a", 190},
            {"j0-a", 200},
            {"k0-a", 210},
            {"l0-a", 220},
            {"m0-a", 230},
            {"n0-a", 240},
            {"o0-a", 250},
            {"p0-a", 260},
            {"q0-a", 270},
            {"r0-a", 280},
            {"s0-a", 290},
            {"t0-a", 300},
            {"u0-a", 310},
            {"v0-a", 320},
        }, {
            {"00-a", 11}, // different value
            {"10-a", 20},
            {"20-a", 30},
            {"30-a", 40},
            {"40-a", 50},
            {"50-a", 60},
            {"60-a", 70},
            {"70-a", 80},
            {"80-a", 90},
            {"90-a", 100},
            {"a0-a", 110},
            {"b0-a", 120},
            {"c0-a", 130},
            {"d0-a", 140},
            {"e0-a", 150},
            {"f0-a", 160},
            {"g0-a", 170},
            {"h0-a", 180},
            {"i0-a", 190},
            {"j0-a", 200},
            {"k0-a", 210},
            {"l0-a", 220},
            {"m0-a", 230},
            {"n0-a", 240},
            {"o0-a", 250},
            {"p0-a", 260},
            {"q0-a", 270},
            {"r0-a", 280},
            {"s0-a", 290},
            {"t0-a", 300},
            {"u0-a", 310},
            {"v0-a", 320},
        }, {
            {"00-a", 10},
            {"10-a", 20},
            {"20-a", 30},
            {"30-a", 40},
            {"40-a", 50},
            {"50-a", 60},
            {"60-a", 70},
            {"70-a", 80},
            {"80-a", 90},
            {"90-a", 100},
            {"a0-a", 110},
            {"b0-a", 120},
            {"c0-a", 130},
            {"d0-a", 140},
            {"e0-a", 150},
            {"f0-a", 160},
            {"g0-a", 170},
            {"h0-a", 180},
            {"i0-a", 190},
            {"j0-a", 200},
            {"k0-a", 210},
            {"l0-a", 220},
            {"m0-a", 230},
            {"n0-a", 240},
            {"o0-a", 250},
            {"p0-a", 260},
            {"q0-a", 270},
            {"r0-a", 280},
            {"s0-a", 290},
            {"t0-a", 300},
            {"u0-a", 310},
            {"v0-a", 321}, // different value
        }, {
            {"00-a", 10},
            {"10-a", 20},
            {"20-a", 30},
            {"30-a", 40},
            {"40-a", 50},
            {"50-a", 60},
            {"60-a", 70},
            {"70-a", 80},
            {"80-a", 90},
            {"90-a", 100},
            {"a0-a", 110},
            {"b0-a", 120},
            {"c0-a", 130},
            {"d0-a", 140},
            {"e0-a", 150},
            {"f0-a", 160},
            {"g0-a", 170},
            {"h0-a", 180},
            {"i0-a", 190},
            {"j0-a", 200},
            {"k0-a", 210},
            {"l0-a", 220},
            {"m0-a", 230},
            {"n0-a", 240},
            {"o0-a", 250},
            {"p0-a", 260},
            {"q0-a", 270},
            {"r0-a", 280},
            {"s0-a", 290},
            {"t0-a", 300},
            {"u0-a", 310},
            {"v0-b", 320}, // different key
        }, {
            {"000-a", 10},
            {"000-b", 11},
            {"000-c", 12},
            {"100-a", 20},
            {"200-a", 30},
            {"00-a", 110},
            {"00-b", 111},
            {"00-c", 112},
            {"10-a", 120},
            {"20-a", 130},
        }, {
            {"000-a", 10},
            {"000-b", 11},
            {"000-d", 12}, // different key
            {"100-a", 20},
            {"200-a", 30},
            {"00-a", 110},
            {"00-b", 111},
            {"00-c", 112},
            {"10-a", 120},
            {"20-a", 130},
        }
    });
}

TEST_F(persistent_hash_map_test, sequence)
{
    check_sequence({});
    check_sequence({{"7-a", 3}});
    check_sequence({
        {"7-a", 3},
        {"7-b", 4},
    });
    check_sequence({
        {"7-a", 3},
        {"7-b", 4},
        {"7-c", 5},
        {"7-d", 6},
    });
    check_sequence({
        {"0-a", 10},
        {"1-a", 20},
        {"2-a", 30},
        {"3-a", 40},
        {"4-a", 50},
        {"c-a", 130},
        {"d-a", 140},
        {"e-a", 150},
        {"f-a", 160},
        {"g-a", 170},
        {"n-a", 240},
        {"o-a", 250},
        {"p-a", 260},
        {"q-a", 270},
        {"r-a", 280},
        {"s-a", 290},
        {"t-a", 300},
        {"u-a", 310},
        {"v-a", 320},
    });
    check_sequence({
        {"2-a", 30},
        {"3-a", 40},
        {"4-a", 50},
        {"1-a", 20},
        {"1-b", 21},
        {"1-c", 22},
        {"0-a", 10},
        {"0-b", 11},
        {"0-c", 12},
        {"0-d", 13},
    });
    check_sequence({
        {"2-a", 30},
        {"2-b", 31},
        {"2-c", 32},
        {"1-a", 20},
        {"1-b", 21},
        {"1-c", 22},
    });
    check_sequence({
        {"000-a", 10},
        {"100-a", 20},
        {"200-a", 30},
        {"300-a", 40},
        {"400-a", 50},
        {"500-a", 60},
        {"600-a", 70},
        {"700-a", 80},
    });
    check_sequence({
        {"000-a", 10},
        {"100-a", 20},
        {"200-a", 30},
        {"300-a", 40},
        {"400-a", 50},
        {"500-a", 60},
        {"600-a", 70},
        {"700-a", 80},
    });
    check_sequence({
        {"2-a", 90},
        {"010-a", 100},
        {"110-a", 110},
        {"210-a", 120},
        {"000-a", 10},
        {"100-a", 20},
        {"200-a", 30},
        {"300-a", 40},
        {"400-a", 50},
        {"500-a", 60},
        {"600-a", 70},
        {"700-a", 80},
    });
    check_sequence({
        {"021-a", 10},
        {"121-a", 20},
        {"221-a", 30},
        {"011-a", 40},
        {"111-a", 50},
        {"211-a", 60},
        {"001-a", 70},
        {"101-a", 80},
        {"201-a", 90},
        {"020-a", 100},
        {"120-a", 110},
        {"220-a", 120},
        {"010-a", 130},
        {"110-a", 140},
        {"210-a", 150},
        {"000-a", 160},
        {"100-a", 170},
        {"200-a", 180},
    });
    check_sequence({
        {"21-a", 10},
        {"21-b", 20},
        {"21-c", 30},
        {"11-a", 40},
        {"11-b", 50},
        {"11-c", 60},
        {"01-a", 70},
        {"01-b", 80},
        {"01-c", 90},
        {"20-a", 100},
        {"20-b", 110},
        {"20-c", 120},
        {"10-a", 130},
        {"10-b", 140},
        {"10-c", 150},
        {"00-a", 160},
        {"00-b", 170},
        {"00-c", 180},
    });
}

}
}
