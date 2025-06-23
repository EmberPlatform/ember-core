#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ember.h"
#include "../../src/stdlib/stdlib.h"

// Test helper function to create string values
static ember_value make_test_string(ember_vm* vm, const char* str) {
    return ember_make_string_gc(vm, str);
}

// Test helper to compare hex strings
static int hex_compare(const char* expected, const char* actual) {
    if (!expected || !actual) return 0;
    return strcmp(expected, actual) == 0;
}

// Test SHA-256 with known test vectors from NIST
static void test_sha256_vectors(ember_vm* vm) {
    printf("Testing SHA-256 with NIST test vectors...\n");
    
    // Test vector 1: Empty string
    ember_value empty = make_test_string(vm, "");

    UNUSED(empty);
    ember_value result1 = ember_native_sha256(vm, 1, &empty);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_STRING);
    const char* hash1 = result1.as.obj_val ? AS_CSTRING(result1) : result1.as.string_val;

    UNUSED(hash1);
    assert(hex_compare("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", hash1));
    printf("  ✓ Empty string test passed\n");
    
    // Test vector 2: "abc"
    ember_value abc = make_test_string(vm, "abc");

    UNUSED(abc);
    ember_value result2 = ember_native_sha256(vm, 1, &abc);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_STRING);
    const char* hash2 = result2.as.obj_val ? AS_CSTRING(result2) : result2.as.string_val;

    UNUSED(hash2);
    assert(hex_compare("ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad", hash2));
    printf("  ✓ 'abc' test passed\n");
    
    // Test vector 3: "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"
    ember_value long_str = make_test_string(vm, "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");

    UNUSED(long_str);
    ember_value result3 = ember_native_sha256(vm, 1, &long_str);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_STRING);
    const char* hash3 = result3.as.obj_val ? AS_CSTRING(result3) : result3.as.string_val;

    UNUSED(hash3);
    assert(hex_compare("248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1", hash3));
    printf("  ✓ Long string test passed\n");
    
    // Test vector 4: Single character
    ember_value single = make_test_string(vm, "a");

    UNUSED(single);
    ember_value result4 = ember_native_sha256(vm, 1, &single);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_STRING);
    const char* hash4 = result4.as.obj_val ? AS_CSTRING(result4) : result4.as.string_val;

    UNUSED(hash4);
    assert(hex_compare("ca978112ca1bbdcafac231b39a23dc4da786eff8147c4e72b9807785afee48bb", hash4));
    printf("  ✓ Single character test passed\n");
    
    printf("SHA-256 tests completed successfully!\n\n");
}

// Test SHA-512 with known test vectors from NIST
static void test_sha512_vectors(ember_vm* vm) {
    printf("Testing SHA-512 with NIST test vectors...\n");
    
    // Test vector 1: Empty string
    ember_value empty = make_test_string(vm, "");

    UNUSED(empty);
    ember_value result1 = ember_native_sha512(vm, 1, &empty);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_STRING);
    const char* hash1 = result1.as.obj_val ? AS_CSTRING(result1) : result1.as.string_val;

    UNUSED(hash1);
    assert(hex_compare("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e", hash1));
    printf("  ✓ Empty string test passed\n");
    
    // Test vector 2: "abc"
    ember_value abc = make_test_string(vm, "abc");

    UNUSED(abc);
    ember_value result2 = ember_native_sha512(vm, 1, &abc);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_STRING);
    const char* hash2 = result2.as.obj_val ? AS_CSTRING(result2) : result2.as.string_val;

    UNUSED(hash2);
    assert(hex_compare("ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f", hash2));
    printf("  ✓ 'abc' test passed\n");
    
    // Test vector 3: "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"
    ember_value long_str = make_test_string(vm, "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu");

    UNUSED(long_str);
    ember_value result3 = ember_native_sha512(vm, 1, &long_str);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_STRING);
    const char* hash3 = result3.as.obj_val ? AS_CSTRING(result3) : result3.as.string_val;

    UNUSED(hash3);
    assert(hex_compare("8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909", hash3));
    printf("  ✓ Long string test passed\n");
    
    printf("SHA-512 tests completed successfully!\n\n");
}

// Test HMAC-SHA256 with RFC 4231 test vectors
static void test_hmac_sha256_vectors(ember_vm* vm) {
    printf("Testing HMAC-SHA256 with RFC 4231 test vectors...\n");
    
    // Test vector 1
    ember_value key1 = make_test_string(vm, "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b");

    UNUSED(key1);
    ember_value data1 = make_test_string(vm, "Hi There");

    UNUSED(data1);
    ember_value args1[2] = {key1, data1};
    ember_value result1 = ember_native_hmac_sha256(vm, 2, args1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_STRING);
    const char* hmac1 = result1.as.obj_val ? AS_CSTRING(result1) : result1.as.string_val;

    UNUSED(hmac1);
    assert(hex_compare("b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7", hmac1));
    printf("  ✓ Test vector 1 passed\n");
    
    // Test vector 2
    ember_value key2 = make_test_string(vm, "Jefe");

    UNUSED(key2);
    ember_value data2 = make_test_string(vm, "what do ya want for nothing?");

    UNUSED(data2);
    ember_value args2[2] = {key2, data2};
    ember_value result2 = ember_native_hmac_sha256(vm, 2, args2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_STRING);
    const char* hmac2 = result2.as.obj_val ? AS_CSTRING(result2) : result2.as.string_val;

    UNUSED(hmac2);
    assert(hex_compare("5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843", hmac2));
    printf("  ✓ Test vector 2 passed\n");
    
    printf("HMAC-SHA256 tests completed successfully!\n\n");
}

// Test HMAC-SHA512 with RFC 4231 test vectors
static void test_hmac_sha512_vectors(ember_vm* vm) {
    printf("Testing HMAC-SHA512 with RFC 4231 test vectors...\n");
    
    // Test vector 1
    ember_value key1 = make_test_string(vm, "\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b\x0b");

    UNUSED(key1);
    ember_value data1 = make_test_string(vm, "Hi There");

    UNUSED(data1);
    ember_value args1[2] = {key1, data1};
    ember_value result1 = ember_native_hmac_sha512(vm, 2, args1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_STRING);
    const char* hmac1 = result1.as.obj_val ? AS_CSTRING(result1) : result1.as.string_val;

    UNUSED(hmac1);
    assert(hex_compare("87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854", hmac1));
    printf("  ✓ Test vector 1 passed\n");
    
    // Test vector 2
    ember_value key2 = make_test_string(vm, "Jefe");

    UNUSED(key2);
    ember_value data2 = make_test_string(vm, "what do ya want for nothing?");

    UNUSED(data2);
    ember_value args2[2] = {key2, data2};
    ember_value result2 = ember_native_hmac_sha512(vm, 2, args2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_STRING);
    const char* hmac2 = result2.as.obj_val ? AS_CSTRING(result2) : result2.as.string_val;

    UNUSED(hmac2);
    assert(hex_compare("164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737", hmac2));
    printf("  ✓ Test vector 2 passed\n");
    
    printf("HMAC-SHA512 tests completed successfully!\n\n");
}

// Test secure random number generation
static void test_secure_random(ember_vm* vm) {
    printf("Testing secure random number generation...\n");
    
    // Test generating 16 bytes of random data
    ember_value size = ember_make_number(16);

    UNUSED(size);
    ember_value result1 = ember_native_secure_random(vm, 1, &size);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_STRING);
    const char* random1 = result1.as.obj_val ? AS_CSTRING(result1) : result1.as.string_val;

    UNUSED(random1);
    assert(strlen(random1) == 32); // 16 bytes = 32 hex chars
    printf("  ✓ 16-byte random generation passed\n");
    
    // Test generating 32 bytes of random data
    ember_value size2 = ember_make_number(32);

    UNUSED(size2);
    ember_value result2 = ember_native_secure_random(vm, 1, &size2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_STRING);
    const char* random2 = result2.as.obj_val ? AS_CSTRING(result2) : result2.as.string_val;

    UNUSED(random2);
    assert(strlen(random2) == 64); // 32 bytes = 64 hex chars
    printf("  ✓ 32-byte random generation passed\n");
    
    // Test that consecutive calls produce different results
    ember_value result3 = ember_native_secure_random(vm, 1, &size);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_STRING);
    const char* random3 = result3.as.obj_val ? AS_CSTRING(result3) : result3.as.string_val;

    UNUSED(random3);
    assert(strcmp(random1, random3) != 0); // Should be different
    printf("  ✓ Random uniqueness test passed\n");
    
    // Test invalid inputs
    ember_value invalid_size = ember_make_number(-1);

    UNUSED(invalid_size);
    ember_value result_invalid = ember_native_secure_random(vm, 1, &invalid_size);

    UNUSED(result_invalid);
    assert(result_invalid.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid input handling test passed\n");
    
    printf("Secure random generation tests completed successfully!\n\n");
}

// Test secure comparison function
static void test_secure_compare(ember_vm* vm) {
    printf("Testing secure comparison function...\n");
    
    // Test identical strings
    ember_value str1 = make_test_string(vm, "hello");

    UNUSED(str1);
    ember_value str2 = make_test_string(vm, "hello");

    UNUSED(str2);
    ember_value args1[2] = {str1, str2};
    ember_value result1 = ember_native_secure_compare(vm, 2, args1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_BOOL);
    assert(result1.as.bool_val == 1);
    printf("  ✓ Identical strings test passed\n");
    
    // Test different strings
    ember_value str3 = make_test_string(vm, "hello");

    UNUSED(str3);
    ember_value str4 = make_test_string(vm, "world");

    UNUSED(str4);
    ember_value args2[2] = {str3, str4};
    ember_value result2 = ember_native_secure_compare(vm, 2, args2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_BOOL);
    assert(result2.as.bool_val == 0);
    printf("  ✓ Different strings test passed\n");
    
    // Test different length strings
    ember_value str5 = make_test_string(vm, "hello");

    UNUSED(str5);
    ember_value str6 = make_test_string(vm, "hello world");

    UNUSED(str6);
    ember_value args3[2] = {str5, str6};
    ember_value result3 = ember_native_secure_compare(vm, 2, args3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_BOOL);
    assert(result3.as.bool_val == 0);
    printf("  ✓ Different length strings test passed\n");
    
    // Test empty strings
    ember_value str7 = make_test_string(vm, "");

    UNUSED(str7);
    ember_value str8 = make_test_string(vm, "");

    UNUSED(str8);
    ember_value args4[2] = {str7, str8};
    ember_value result4 = ember_native_secure_compare(vm, 2, args4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_BOOL);
    assert(result4.as.bool_val == 1);
    printf("  ✓ Empty strings test passed\n");
    
    printf("Secure comparison tests completed successfully!\n\n");
}

// Test error handling and edge cases
static void test_error_handling(ember_vm* vm) {
    printf("Testing error handling and edge cases...\n");
    
    // Test SHA-256 with invalid arguments
    ember_value nil_val = ember_make_nil();

    UNUSED(nil_val);
    ember_value result1 = ember_native_sha256(vm, 1, &nil_val);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NIL);
    printf("  ✓ SHA-256 nil input handling passed\n");
    
    ember_value result2 = ember_native_sha256(vm, 0, NULL);

    
    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NIL);
    printf("  ✓ SHA-256 no arguments handling passed\n");
    
    // Test HMAC with invalid arguments
    ember_value str_val = make_test_string(vm, "test");

    UNUSED(str_val);
    ember_value args_invalid[2] = {nil_val, str_val};
    ember_value result3 = ember_native_hmac_sha256(vm, 2, args_invalid);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NIL);
    printf("  ✓ HMAC invalid key handling passed\n");
    
    ember_value result4 = ember_native_hmac_sha256(vm, 1, &str_val);

    
    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NIL);
    printf("  ✓ HMAC insufficient arguments handling passed\n");
    
    // Test secure_random with invalid size
    ember_value large_size = ember_make_number(2000);

    UNUSED(large_size); // Above limit
    ember_value result5 = ember_native_secure_random(vm, 1, &large_size);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NIL);
    printf("  ✓ Secure random size limit handling passed\n");
    
    // Test secure_compare with invalid arguments
    ember_value args_invalid2[2] = {nil_val, nil_val};
    ember_value result6 = ember_native_secure_compare(vm, 2, args_invalid2);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_BOOL);
    assert(result6.as.bool_val == 0);
    printf("  ✓ Secure compare nil inputs handling passed\n");
    
    printf("Error handling tests completed successfully!\n\n");
}

int main() {
    printf("Starting comprehensive cryptographic function tests...\n\n");
    
    // Initialize VM
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Run all test suites
    test_sha256_vectors(vm);
    test_sha512_vectors(vm);
    test_hmac_sha256_vectors(vm);
    test_hmac_sha512_vectors(vm);
    test_secure_random(vm);
    test_secure_compare(vm);
    test_error_handling(vm);
    
    // Cleanup
    ember_free_vm(vm);
    
    printf("🎉 All cryptographic function tests passed successfully!\n");
    printf("✅ SHA-256 implementation verified with NIST test vectors\n");
    printf("✅ SHA-512 implementation verified with NIST test vectors\n");
    printf("✅ HMAC-SHA256 implementation verified with RFC 4231 test vectors\n");
    printf("✅ HMAC-SHA512 implementation verified with RFC 4231 test vectors\n");
    printf("✅ Secure random number generation tested\n");
    printf("✅ Secure comparison function tested\n");
    printf("✅ Error handling and edge cases tested\n");
    
    return 0;
}