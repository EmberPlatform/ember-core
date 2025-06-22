#include "testing_framework.h"

// Test Set functionality
TEST_DEFINE(set_creation) {
    ember_value set = ember_make_set(vm);
    TEST_ASSERT_NOT_NULL(set, "Set should be created successfully");
    TEST_ASSERT(set.type == EMBER_VAL_SET, "Value should be of type SET");
}

TEST_DEFINE(set_operations) {
    ember_value set = ember_make_set(vm);
    ember_set* set_obj = AS_SET(set);
    
    // Test adding elements
    ember_value elem1 = ember_make_number(42);
    ember_value elem2 = ember_make_string_gc(vm, "hello");
    
    int result1 = set_add(set_obj, elem1);
    TEST_ASSERT(result1 == 1, "Adding first element should succeed");
    TEST_ASSERT(set_obj->size == 1, "Set size should be 1 after adding element");
    
    int result2 = set_add(set_obj, elem2);
    TEST_ASSERT(result2 == 1, "Adding second element should succeed");
    TEST_ASSERT(set_obj->size == 2, "Set size should be 2 after adding second element");
    
    // Test has operation
    TEST_ASSERT(set_has(set_obj, elem1), "Set should contain first element");
    TEST_ASSERT(set_has(set_obj, elem2), "Set should contain second element");
    
    ember_value elem3 = ember_make_number(99);
    TEST_ASSERT(!set_has(set_obj, elem3), "Set should not contain non-added element");
    
    // Test delete operation
    int delete_result = set_delete(set_obj, elem1);
    TEST_ASSERT(delete_result == 1, "Deleting existing element should succeed");
    TEST_ASSERT(set_obj->size == 1, "Set size should be 1 after deletion");
    TEST_ASSERT(!set_has(set_obj, elem1), "Set should not contain deleted element");
    
    // Test clear operation
    set_clear(set_obj);
    TEST_ASSERT(set_obj->size == 0, "Set should be empty after clear");
    TEST_ASSERT(!set_has(set_obj, elem2), "Set should not contain any elements after clear");
}

// Test Map functionality
TEST_DEFINE(map_creation) {
    ember_value map = ember_make_map(vm);
    TEST_ASSERT_NOT_NULL(map, "Map should be created successfully");
    TEST_ASSERT(map.type == EMBER_VAL_MAP, "Value should be of type MAP");
}

TEST_DEFINE(map_operations) {
    ember_value map = ember_make_map(vm);
    ember_map* map_obj = AS_MAP(map);
    
    // Test setting key-value pairs
    ember_value key1 = ember_make_string_gc(vm, "name");
    ember_value val1 = ember_make_string_gc(vm, "Ember");
    ember_value key2 = ember_make_number(42);
    ember_value val2 = ember_make_string_gc(vm, "answer");
    
    int result1 = map_set(map_obj, key1, val1);
    TEST_ASSERT(result1 == 1, "Setting first key-value pair should succeed");
    TEST_ASSERT(map_obj->size == 1, "Map size should be 1 after adding pair");
    
    int result2 = map_set(map_obj, key2, val2);
    TEST_ASSERT(result2 == 1, "Setting second key-value pair should succeed");
    TEST_ASSERT(map_obj->size == 2, "Map size should be 2 after adding second pair");
    
    // Test getting values
    ember_value retrieved1 = map_get(map_obj, key1);
    TEST_ASSERT_EQ(val1, retrieved1, "Retrieved value should match set value");
    
    ember_value retrieved2 = map_get(map_obj, key2);
    TEST_ASSERT_EQ(val2, retrieved2, "Retrieved second value should match set value");
    
    // Test has operation
    TEST_ASSERT(map_has(map_obj, key1), "Map should contain first key");
    TEST_ASSERT(map_has(map_obj, key2), "Map should contain second key");
    
    ember_value non_existent_key = ember_make_string_gc(vm, "not_there");
    TEST_ASSERT(!map_has(map_obj, non_existent_key), "Map should not contain non-existent key");
    
    // Test delete operation
    int delete_result = map_delete(map_obj, key1);
    TEST_ASSERT(delete_result == 1, "Deleting existing key should succeed");
    TEST_ASSERT(map_obj->size == 1, "Map size should be 1 after deletion");
    TEST_ASSERT(!map_has(map_obj, key1), "Map should not contain deleted key");
    
    // Test clear operation
    map_clear(map_obj);
    TEST_ASSERT(map_obj->size == 0, "Map should be empty after clear");
    TEST_ASSERT(!map_has(map_obj, key2), "Map should not contain any keys after clear");
}

// Test collections equality
TEST_DEFINE(collections_equality) {
    // Test Set equality
    ember_value set1 = ember_make_set(vm);
    ember_value set2 = ember_make_set(vm);
    
    ember_set* set1_obj = AS_SET(set1);
    ember_set* set2_obj = AS_SET(set2);
    
    // Empty sets should be equal
    TEST_ASSERT_EQ(set1, set2, "Empty sets should be equal");
    
    // Add same elements to both sets
    ember_value elem = ember_make_number(123);
    set_add(set1_obj, elem);
    set_add(set2_obj, elem);
    
    TEST_ASSERT_EQ(set1, set2, "Sets with same elements should be equal");
    
    // Add different element to one set
    ember_value elem2 = ember_make_string_gc(vm, "different");
    set_add(set1_obj, elem2);
    
    TEST_ASSERT_NEQ(set1, set2, "Sets with different elements should not be equal");
    
    // Test Map equality
    ember_value map1 = ember_make_map(vm);
    ember_value map2 = ember_make_map(vm);
    
    ember_map* map1_obj = AS_MAP(map1);
    ember_map* map2_obj = AS_MAP(map2);
    
    // Empty maps should be equal
    TEST_ASSERT_EQ(map1, map2, "Empty maps should be equal");
    
    // Add same key-value pairs to both maps
    ember_value key = ember_make_string_gc(vm, "key");
    ember_value value = ember_make_number(456);
    map_set(map1_obj, key, value);
    map_set(map2_obj, key, value);
    
    TEST_ASSERT_EQ(map1, map2, "Maps with same key-value pairs should be equal");
    
    // Add different pair to one map
    ember_value key2 = ember_make_string_gc(vm, "key2");
    ember_value value2 = ember_make_number(789);
    map_set(map1_obj, key2, value2);
    
    TEST_ASSERT_NEQ(map1, map2, "Maps with different key-value pairs should not be equal");
}

// Test suite runner
TEST_SUITE(collections) {
    ember_vm* vm = ember_new_vm();
    test_suite* suite = test_runner_add_suite(runner, "Collections Framework");
    
    test_suite_add_test(suite, "Set Creation", test_set_creation, vm);
    test_suite_add_test(suite, "Set Operations", test_set_operations, vm);
    test_suite_add_test(suite, "Map Creation", test_map_creation, vm);
    test_suite_add_test(suite, "Map Operations", test_map_operations, vm);
    test_suite_add_test(suite, "Collections Equality", test_collections_equality, vm);
    
    ember_free_vm(vm);
}