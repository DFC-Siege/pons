#include <unity.h>

#include "nvs_store.hpp"
#include "nvs_store_test.hpp"

static void test_init_succeeds() {
        const auto result = store::kv::NvsStore::init("test");
        TEST_ASSERT_FALSE(result.failed());
}

static void test_store_and_get_value() {
        auto store_result = store::kv::NvsStore::init("test");
        TEST_ASSERT_FALSE(store_result.failed());
        auto store = store_result.value();

        const auto set_result = store.store("key1", "value1");
        TEST_ASSERT_FALSE(set_result.failed());

        const auto get_result = store.get("key1");
        TEST_ASSERT_FALSE(get_result.failed());
        TEST_ASSERT_EQUAL_STRING("value1", get_result.value().c_str());
}

static void test_get_nonexistent_key_fails() {
        auto store_result = store::kv::NvsStore::init("test");
        TEST_ASSERT_FALSE(store_result.failed());
        auto store = store_result.value();

        const auto result = store.get("nonexistent_key_xyz");
        TEST_ASSERT_TRUE(result.failed());
}

static void test_overwrite_value() {
        auto store_result = store::kv::NvsStore::init("test");
        TEST_ASSERT_FALSE(store_result.failed());
        auto store = store_result.value();

        store.store("key2", "original");
        store.store("key2", "updated");

        const auto result = store.get("key2");
        TEST_ASSERT_FALSE(result.failed());
        TEST_ASSERT_EQUAL_STRING("updated", result.value().c_str());
}

static void test_store_empty_value() {
        auto store_result = store::kv::NvsStore::init("test");
        TEST_ASSERT_FALSE(store_result.failed());
        auto store = store_result.value();

        const auto set_result = store.store("key3", "");
        TEST_ASSERT_FALSE(set_result.failed());
}

static void test_different_namespaces_are_independent() {
        auto store1_result = store::kv::NvsStore::init("ns1");
        auto store2_result = store::kv::NvsStore::init("ns2");
        TEST_ASSERT_FALSE(store1_result.failed());
        TEST_ASSERT_FALSE(store2_result.failed());

        auto store1 = store1_result.value();
        auto store2 = store2_result.value();

        store1.store("shared_key", "from_ns1");
        store2.store("shared_key", "from_ns2");

        TEST_ASSERT_EQUAL_STRING("from_ns1",
                                 store1.get("shared_key").value().c_str());
        TEST_ASSERT_EQUAL_STRING("from_ns2",
                                 store2.get("shared_key").value().c_str());
}

void run_nvs_store_tests() {
        RUN_TEST(test_init_succeeds);
        RUN_TEST(test_store_and_get_value);
        RUN_TEST(test_get_nonexistent_key_fails);
        RUN_TEST(test_overwrite_value);
        RUN_TEST(test_store_empty_value);
        RUN_TEST(test_different_namespaces_are_independent);
}
