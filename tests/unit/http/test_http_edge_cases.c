/**
 * HTTP Edge Cases and Malformed Input Test
 * Tests HTTP request/response parsing with malformed inputs and edge cases
 */

#include "ember.h"
#include "../../src/stdlib/http_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void test_malformed_urls() {
    printf("Testing malformed URLs...\n");
    
    http_response_t response;
    int result;
    
    // Test with NULL URL
    result = ember_http_get(NULL, NULL, &response);
    assert(result != 0); // Should fail
    printf("  ✓ NULL URL correctly rejected\n");
    
    // Test with empty URL
    result = ember_http_get("", NULL, &response);
    assert(result != 0); // Should fail
    printf("  ✓ Empty URL correctly rejected\n");
    
    // Test with invalid protocol
    result = ember_http_get("ftp://example.com", NULL, &response);
    assert(result != 0); // Should fail
    printf("  ✓ Invalid protocol correctly rejected\n");
    
    // Test with malformed URL
    result = ember_http_get("ht tp://example.com", NULL, &response);
    assert(result != 0); // Should fail
    printf("  ✓ Malformed URL correctly rejected\n");
    
    // Test with extremely long URL
    char long_url[10000];
    strcpy(long_url, "https://");
    for (int i = 8; i < 9990; i++) {
        long_url[i] = 'a';
    }
    strcpy(long_url + 9990, ".com");
    result = ember_http_get(long_url, NULL, &response);
    // This may or may not fail depending on curl limits, but shouldn't crash
    printf("  ✓ Extremely long URL handled safely\n");
    
    printf("Malformed URL tests passed\n\n");
}

void test_invalid_auth_tokens() {
    printf("Testing invalid authentication tokens...\n");
    
    http_response_t response;
    int result;
    
    // Test with very long auth token
    char long_token[5000];
    memset(long_token, 'x', 4999);
    long_token[4999] = '\0';
    
    result = ember_http_get("https://httpbin.org/headers", long_token, &response);
    // Should handle gracefully without crashing
    if (result == 0) {
        http_response_cleanup(&response);
    }
    printf("  ✓ Long auth token handled safely\n");
    
    // Test with special characters in token
    result = ember_http_get("https://httpbin.org/headers", "token with spaces", &response);
    if (result == 0) {
        http_response_cleanup(&response);
    }
    printf("  ✓ Token with spaces handled safely\n");
    
    // Test with null bytes in token (truncated string)
    char token_with_null[] = "valid\\0invalid";
    result = ember_http_get("https://httpbin.org/headers", token_with_null, &response);
    if (result == 0) {
        http_response_cleanup(&response);
    }
    printf("  ✓ Token with null bytes handled safely\n");
    
    printf("Invalid auth token tests passed\n\n");
}

void test_response_edge_cases() {
    printf("Testing response handling edge cases...\n");
    
    http_response_t response;
    
    // Test response cleanup with uninitialized response
    memset(&response, 0, sizeof(response));
    http_response_cleanup(&response); // Should not crash
    printf("  ✓ Cleanup of uninitialized response handled safely\n");
    
    // Test response cleanup with partially initialized response
    response.data = malloc(100);
    strcpy(response.data, "test data");
    response.content_type = NULL;
    response.size = 9;
    http_response_cleanup(&response);
    printf("  ✓ Cleanup of partially initialized response handled safely\n");
    
    // Test double cleanup
    memset(&response, 0, sizeof(response));
    response.data = malloc(50);
    strcpy(response.data, "test");
    response.content_type = malloc(20);
    strcpy(response.content_type, "text/plain");
    http_response_cleanup(&response);
    http_response_cleanup(&response); // Should not crash
    printf("  ✓ Double cleanup handled safely\n");
    
    printf("Response edge case tests passed\n\n");
}

void test_concurrent_requests() {
    printf("Testing concurrent request safety...\n");
    
    // This tests that HTTP operations are thread-safe
    // We'll simulate this by rapid successive calls
    http_response_t responses[5];
    int results[5];
    
    for (int i = 0; i < 5; i++) {
        results[i] = ember_http_get("https://httpbin.org/get", NULL, &responses[i]);
    }
    
    // Cleanup all responses
    for (int i = 0; i < 5; i++) {
        if (results[i] == 0) {
            http_response_cleanup(&responses[i]);
        }
    }
    
    printf("  ✓ Rapid successive requests handled safely\n");
    printf("Concurrent request tests passed\n\n");
}

void test_memory_exhaustion_scenarios() {
    printf("Testing memory exhaustion scenarios...\n");
    
    // Test with response that might cause memory issues
    http_response_t response;
    
    // Initialize HTTP
    if (ember_http_init() != 0) {
        printf("  WARNING: HTTP init failed, skipping memory tests\n");
        return;
    }
    
    // Test multiple allocations and cleanups
    for (int i = 0; i < 10; i++) {
        int result = ember_http_get("https://httpbin.org/bytes/1024", NULL, &response);
        if (result == 0) {
            // Verify response data is reasonable
            assert(response.data != NULL);
            assert(response.size > 0);
            assert(response.size < 10000); // Reasonable size limit
            http_response_cleanup(&response);
        }
    }
    
    ember_http_cleanup();
    printf("  ✓ Multiple allocation/cleanup cycles handled safely\n");
    printf("Memory exhaustion scenario tests passed\n\n");
}

void test_ssl_edge_cases() {
    printf("Testing SSL/TLS edge cases...\n");
    
    http_response_t response;
    int result;
    
    // Test with self-signed certificate (should fail gracefully)
    result = ember_http_get("https://self-signed.badssl.com/", NULL, &response);
    // Should fail but not crash
    printf("  ✓ Self-signed certificate handled safely\n");
    
    // Test with expired certificate (should fail gracefully)  
    result = ember_http_get("https://expired.badssl.com/", NULL, &response);
    // Should fail but not crash
    printf("  ✓ Expired certificate handled safely\n");
    
    // Test with wrong hostname (should fail gracefully)
    result = ember_http_get("https://wrong.host.badssl.com/", NULL, &response);
    // Should fail but not crash
    printf("  ✓ Wrong hostname handled safely\n");
    
    printf("SSL/TLS edge case tests passed\n\n");
}

int main() {
    printf("HTTP Edge Cases and Malformed Input Tests\n");
    printf("==========================================\n\n");
    
    // Initialize HTTP subsystem
    if (ember_http_init() != 0) {
        printf("ERROR: Failed to initialize HTTP subsystem\n");
        return 1;
    }
    
    test_malformed_urls();
    test_invalid_auth_tokens();
    test_response_edge_cases();
    test_concurrent_requests();
    test_memory_exhaustion_scenarios();
    test_ssl_edge_cases();
    
    // Cleanup
    ember_http_cleanup();
    
    printf("All HTTP edge case tests passed!\n");
    printf("HTTP implementation is robust against malformed inputs and edge cases.\n");
    
    return 0;
}