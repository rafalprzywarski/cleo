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
        EXPECT_EQ_REFS(TRUE, persistent_hash_map_contains(m, *ek))
            << name << " [" << step_kv.first << " " << step_kv.second << "] key: " << key;
        EXPECT_EQ_VALS(*ev, persistent_hash_map_get(m, *ek))
            << name << " [" << step_kv.first << " " << step_kv.second << "] key: " << key;
    }

    void expect_not_in(const std::string& name, const std::pair<std::string, int>& step_kv, Value m, const std::string& key, Value k)
    {
        EXPECT_EQ_REFS(nil, persistent_hash_map_contains(m, k))
            << name << " [" << step_kv.first << " " << step_kv.second << "] key: " << key;
        EXPECT_EQ_REFS(nil, persistent_hash_map_get(m, k))
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
        EXPECT_EQ(get_persistent_hash_map_size(m), Int64(expected.size()))
            << name << " [" << step_kv.first << " " << step_kv.second << "]";
        for (const auto& ekv : expected)
            expect_in(name, step_kv, m, ekv.first, ekv.second);
    }

    void test_assoc(std::vector<std::pair<std::string, int>> kvs)
    {
        Root pm{create_persistent_hash_map()};
        std::unordered_map<std::string, int> expected;

        for (auto const& kv : kvs)
        {
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
        }
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

}
}
