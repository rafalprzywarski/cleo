#include <cleo/persistent_hash_map.hpp>
#include <cleo/multimethod.hpp>
#include "util.hpp"

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

    persistent_hash_map_test() : Test("cleo.persistent_hash_map.test")
    {
        Root f{create_native_function1<string_value>()};
        define_method(HASH_OBJ, *HashString, *f);

        f = create_native_function2<are_hash_strings_equal>();
        Root args{svec(*HashString, *HashString)};
        define_method(OBJ_EQ, *args, *f);
    }

    Force create_key(const std::string& s)
    {
        Root k{create_string(s)};
        return create_object1(*HashString, *k);
    }

    void expect_in(const std::string& name, const std::pair<std::string, int>& step_kv, Value m, const std::string& key, int value)
    {
        Root ek{create_key(key)};
        Root ev{create_int64(value)};
        ASSERT_EQ_VALS(*ev, persistent_hash_map_get(m, *ek))
            << name << " [" << step_kv.first << " " << step_kv.second << "] key: " << key;
        ASSERT_EQ_REFS(TRUE, persistent_hash_map_contains(m, *ek))
            << name << " [" << step_kv.first << " " << step_kv.second << "] key: " << key;
    }

    void expect_not_in(const std::string& name, const std::pair<std::string, int>& step_kv, Value m, const std::string& key, Value k)
    {
        ASSERT_EQ_REFS(nil, persistent_hash_map_get(m, k))
            << name << " [" << step_kv.first << " " << step_kv.second << "] key: " << key;
        ASSERT_EQ_REFS(nil, persistent_hash_map_contains(m, k))
            << name << " [" << step_kv.first << " " << step_kv.second << "] key: " << key;
    }

    void expect_not_in(const std::string& name, const std::pair<std::string, int>& step_kv, Value m, const std::string& key)
    {
        Root k{create_key(key)};
        expect_not_in(name, step_kv, m, key, *k);
    }

    void expect_nil_not_in(const std::string& name, const std::pair<std::string, int>& step_kv, Value m)
    {
        expect_not_in(name, step_kv, m, "nil", nil);
    }

    void check_pm(const std::string& name, const std::pair<std::string, int>& step_kv, Value m, const std::unordered_map<std::string, int>& expected)
    {
        for (const auto& ekv : expected)
            expect_in(name, step_kv, m, ekv.first, ekv.second);
        ASSERT_EQ(get_persistent_hash_map_size(m), Int64(expected.size()))
            << name << " [" << step_kv.first << " " << step_kv.second << "]";
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

                pm = *new_pm;
                expected[kv.first] = kv.second;

                check_pm("new", kv, *pm, expected);

                expect_not_in("new", kv, *pm, bad_key);
                expect_nil_not_in("new", kv, *pm);
            });
    }
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

}
}
