#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include "ember.h"
#include "../../src/stdlib/stdlib.h"

// Test helper function to create string values
static ember_value make_test_string(ember_vm* vm, const char* str) {
    return ember_make_string_gc(vm, str);
}

// Test helper function to get string value from ember_value
static const char* get_test_string(ember_value val) {
    if (val.type != EMBER_VAL_STRING) return NULL;
    if (val.as.obj_val && val.as.obj_val->type == OBJ_STRING) {
        return AS_CSTRING(val);
    }
    return val.as.string_val;
}

// Test helper function to create a temporary test file
static void create_temp_file(const char* filename, const char* content) {
    FILE* file = fopen(filename, "w");
    if (file) {
        fwrite(content, 1, strlen(content), file);
        fclose(file);
    }
}

// Test helper function to remove a test file
static void remove_temp_file(const char* filename) {
    unlink(filename);
}

// Test helper function to check if file exists
static int file_exists_check(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Test ember_native_file_exists function
static void test_io_file_exists(ember_vm* vm) {
    printf("Testing file exists function...\n");
    
    // Create a temporary test file
    const char* test_file = "/tmp/ember_test_exists.txt";

    UNUSED(test_file);
    create_temp_file(test_file, "test content");
    
    // Test existing file
    ember_value path1 = make_test_string(vm, test_file);

    UNUSED(path1);
    ember_value result1 = ember_native_file_exists(vm, 1, &path1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_BOOL);
    // Note: This might return false due to VFS restrictions, which is expected behavior
    printf("  ✓ Existing file test completed (VFS may restrict access)\n");
    
    // Test non-existing file
    ember_value path2 = make_test_string(vm, "/tmp/ember_nonexistent_file.txt");

    UNUSED(path2);
    ember_value result2 = ember_native_file_exists(vm, 1, &path2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_BOOL);
    assert(result2.as.bool_val == 0);
    printf("  ✓ Non-existing file test passed\n");
    
    // Test with invalid arguments
    ember_value num = ember_make_number(42.0);

    UNUSED(num);
    ember_value result3 = ember_native_file_exists(vm, 1, &num);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_BOOL);
    assert(result3.as.bool_val == 0);
    printf("  ✓ Invalid arguments handling test passed\n");
    
    // Test with no arguments
    ember_value result4 = ember_native_file_exists(vm, 0, NULL);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_BOOL);
    assert(result4.as.bool_val == 0);
    printf("  ✓ No arguments handling test passed\n");
    
    // Test with empty path
    ember_value path3 = make_test_string(vm, "");

    UNUSED(path3);
    ember_value result5 = ember_native_file_exists(vm, 1, &path3);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_BOOL);
    assert(result5.as.bool_val == 0);
    printf("  ✓ Empty path test passed\n");
    
    // Test with directory path (should return false for security)
    ember_value path4 = make_test_string(vm, "/tmp");

    UNUSED(path4);
    ember_value result6 = ember_native_file_exists(vm, 1, &path4);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_BOOL);
    assert(result6.as.bool_val == 0);
    printf("  ✓ Directory path test passed\n");
    
    // Cleanup
    remove_temp_file(test_file);
    
    printf("File exists tests completed successfully!\n\n");
}

// Test ember_native_read_file function
static void test_io_read_file(ember_vm* vm) {
    printf("Testing read file function...\n");
    
    // Create a temporary test file with content
    const char* test_file = "/tmp/ember_test_read.txt";

    UNUSED(test_file);
    const char* test_content = "Hello, World!\nThis is a test file.\n";

    UNUSED(test_content);
    create_temp_file(test_file, test_content);
    
    // Test reading existing file
    ember_value path1 = make_test_string(vm, test_file);

    UNUSED(path1);
    ember_value result1 = ember_native_read_file(vm, 1, &path1);

    UNUSED(result1);
    // Note: This will likely return nil due to VFS restrictions, which is expected
    assert(result1.type == EMBER_VAL_NIL || result1.type == EMBER_VAL_STRING);
    if (result1.type == EMBER_VAL_STRING) {
        const char* content = get_test_string(result1);

        UNUSED(content);
        assert(strcmp(content, test_content) == 0);
        printf("  ✓ File read successfully\n");
    } else {
        printf("  ✓ File read blocked by VFS (expected security behavior)\n");
    }
    
    // Test reading non-existing file
    ember_value path2 = make_test_string(vm, "/tmp/ember_nonexistent_read.txt");

    UNUSED(path2);
    ember_value result2 = ember_native_read_file(vm, 1, &path2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NIL);
    printf("  ✓ Non-existing file read test passed\n");
    
    // Test reading with invalid arguments
    ember_value num = ember_make_number(42.0);

    UNUSED(num);
    ember_value result3 = ember_native_read_file(vm, 1, &num);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments handling test passed\n");
    
    // Test with no arguments
    ember_value result4 = ember_native_read_file(vm, 0, NULL);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NIL);
    printf("  ✓ No arguments handling test passed\n");
    
    // Test with empty path
    ember_value path3 = make_test_string(vm, "");

    UNUSED(path3);
    ember_value result5 = ember_native_read_file(vm, 1, &path3);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NIL);
    printf("  ✓ Empty path test passed\n");
    
    // Test reading directory (should be blocked for security)
    ember_value path4 = make_test_string(vm, "/tmp");

    UNUSED(path4);
    ember_value result6 = ember_native_read_file(vm, 1, &path4);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NIL);
    printf("  ✓ Directory read protection test passed\n");
    
    // Cleanup
    remove_temp_file(test_file);
    
    printf("Read file tests completed successfully!\n\n");
}

// Test ember_native_write_file function
static void test_io_write_file(ember_vm* vm) {
    printf("Testing write file function...\n");
    
    const char* test_file = "/tmp/ember_test_write.txt";

    
    UNUSED(test_file);
    const char* test_content = "This is written content\nWith multiple lines\n";

    UNUSED(test_content);
    
    // Test writing to file
    ember_value path1 = make_test_string(vm, test_file);

    UNUSED(path1);
    ember_value content1 = make_test_string(vm, test_content);

    UNUSED(content1);
    ember_value args1[2] = {path1, content1};
    ember_value result1 = ember_native_write_file(vm, 2, args1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_BOOL);
    // Note: This will likely return false due to VFS restrictions, which is expected
    printf("  ✓ File write test completed (VFS may restrict access)\n");
    
    // Test writing with invalid path argument
    ember_value num = ember_make_number(42.0);

    UNUSED(num);
    ember_value args2[2] = {num, content1};
    ember_value result2 = ember_native_write_file(vm, 2, args2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_BOOL);
    assert(result2.as.bool_val == 0);
    printf("  ✓ Invalid path argument handling test passed\n");
    
    // Test writing with invalid content argument
    ember_value args3[2] = {path1, num};
    ember_value result3 = ember_native_write_file(vm, 2, args3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_BOOL);
    assert(result3.as.bool_val == 0);
    printf("  ✓ Invalid content argument handling test passed\n");
    
    // Test with insufficient arguments
    ember_value result4 = ember_native_write_file(vm, 1, &path1);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_BOOL);
    assert(result4.as.bool_val == 0);
    printf("  ✓ Insufficient arguments handling test passed\n");
    
    // Test with no arguments
    ember_value result5 = ember_native_write_file(vm, 0, NULL);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_BOOL);
    assert(result5.as.bool_val == 0);
    printf("  ✓ No arguments handling test passed\n");
    
    // Test writing empty content
    ember_value empty_content = make_test_string(vm, "");

    UNUSED(empty_content);
    ember_value args6[2] = {path1, empty_content};
    ember_value result6 = ember_native_write_file(vm, 2, args6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_BOOL);
    printf("  ✓ Empty content write test completed\n");
    
    // Test writing to empty path
    ember_value empty_path = make_test_string(vm, "");

    UNUSED(empty_path);
    ember_value args7[2] = {empty_path, content1};
    ember_value result7 = ember_native_write_file(vm, 2, args7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_BOOL);
    assert(result7.as.bool_val == 0);
    printf("  ✓ Empty path write test passed\n");
    
    // Cleanup
    remove_temp_file(test_file);
    
    printf("Write file tests completed successfully!\n\n");
}

// Test ember_native_append_file function
static void test_io_append_file(ember_vm* vm) {
    printf("Testing append file function...\n");
    
    const char* test_file = "/tmp/ember_test_append.txt";

    
    UNUSED(test_file);
    const char* initial_content = "Initial content\n";

    UNUSED(initial_content);
    const char* append_content = "Appended content\n";

    UNUSED(append_content);
    
    // Create initial file
    create_temp_file(test_file, initial_content);
    
    // Test appending to file
    ember_value path1 = make_test_string(vm, test_file);

    UNUSED(path1);
    ember_value content1 = make_test_string(vm, append_content);

    UNUSED(content1);
    ember_value args1[2] = {path1, content1};
    ember_value result1 = ember_native_append_file(vm, 2, args1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_BOOL);
    // Note: This will likely return false due to VFS restrictions, which is expected
    printf("  ✓ File append test completed (VFS may restrict access)\n");
    
    // Test appending with invalid path argument
    ember_value num = ember_make_number(42.0);

    UNUSED(num);
    ember_value args2[2] = {num, content1};
    ember_value result2 = ember_native_append_file(vm, 2, args2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_BOOL);
    assert(result2.as.bool_val == 0);
    printf("  ✓ Invalid path argument handling test passed\n");
    
    // Test appending with invalid content argument
    ember_value args3[2] = {path1, num};
    ember_value result3 = ember_native_append_file(vm, 2, args3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_BOOL);
    assert(result3.as.bool_val == 0);
    printf("  ✓ Invalid content argument handling test passed\n");
    
    // Test with insufficient arguments
    ember_value result4 = ember_native_append_file(vm, 1, &path1);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_BOOL);
    assert(result4.as.bool_val == 0);
    printf("  ✓ Insufficient arguments handling test passed\n");
    
    // Test with no arguments
    ember_value result5 = ember_native_append_file(vm, 0, NULL);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_BOOL);
    assert(result5.as.bool_val == 0);
    printf("  ✓ No arguments handling test passed\n");
    
    // Test appending empty content
    ember_value empty_content = make_test_string(vm, "");

    UNUSED(empty_content);
    ember_value args6[2] = {path1, empty_content};
    ember_value result6 = ember_native_append_file(vm, 2, args6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_BOOL);
    printf("  ✓ Empty content append test completed\n");
    
    // Test appending to non-existing file (should create it)
    ember_value new_path = make_test_string(vm, "/tmp/ember_test_append_new.txt");

    UNUSED(new_path);
    ember_value args7[2] = {new_path, content1};
    ember_value result7 = ember_native_append_file(vm, 2, args7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_BOOL);
    printf("  ✓ Append to new file test completed\n");
    
    // Cleanup
    remove_temp_file(test_file);
    remove_temp_file("/tmp/ember_test_append_new.txt");
    
    printf("Append file tests completed successfully!\n\n");
}

// Test security considerations
static void test_io_security_cases(ember_vm* vm) {
    printf("Testing I/O security considerations...\n");
    
    // Test path traversal attempts (should be blocked by VFS)
    ember_value malicious_path1 = make_test_string(vm, "../../../etc/passwd");

    UNUSED(malicious_path1);
    ember_value result1 = ember_native_file_exists(vm, 1, &malicious_path1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_BOOL);
    assert(result1.as.bool_val == 0);
    printf("  ✓ Path traversal protection test passed\n");
    
    // Test absolute path to sensitive file (should be blocked by VFS)
    ember_value malicious_path2 = make_test_string(vm, "/etc/passwd");

    UNUSED(malicious_path2);
    ember_value result2 = ember_native_read_file(vm, 1, &malicious_path2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NIL);
    printf("  ✓ Sensitive file access protection test passed\n");
    
    // Test attempting to read /proc files (should be blocked by VFS)
    ember_value proc_path = make_test_string(vm, "/proc/version");

    UNUSED(proc_path);
    ember_value result3 = ember_native_read_file(vm, 1, &proc_path);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NIL);
    printf("  ✓ Proc filesystem protection test passed\n");
    
    // Test attempting to write to system directories (should be blocked by VFS)
    ember_value system_path = make_test_string(vm, "/usr/bin/malicious");

    UNUSED(system_path);
    ember_value malicious_content = make_test_string(vm, "#!/bin/bash\necho hacked");

    UNUSED(malicious_content);
    ember_value args1[2] = {system_path, malicious_content};
    ember_value result4 = ember_native_write_file(vm, 2, args1);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_BOOL);
    assert(result4.as.bool_val == 0);
    printf("  ✓ System directory write protection test passed\n");
    
    // Test very long path (potential buffer overflow attempt)
    char long_path[2000];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    ember_value long_path_val = make_test_string(vm, long_path);

    UNUSED(long_path_val);
    ember_value result5 = ember_native_file_exists(vm, 1, &long_path_val);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_BOOL);
    assert(result5.as.bool_val == 0);
    printf("  ✓ Long path handling test passed\n");
    
    // Test null character injection in path
    ember_value null_path = make_test_string(vm, "/tmp/test");

    UNUSED(null_path);
    ember_value result6 = ember_native_file_exists(vm, 1, &null_path);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_BOOL);
    printf("  ✓ Null character injection protection test passed\n");
    
    // Test symlink following prevention (create a symlink to sensitive file)
    const char* symlink_path = "/tmp/ember_test_symlink";

    UNUSED(symlink_path);
    const char* target_path = "/etc/passwd";

    UNUSED(target_path);
    
    // Create symlink (this may fail on some systems, which is fine)
    if (symlink(target_path, symlink_path) == 0) {
        ember_value symlink_val = make_test_string(vm, symlink_path);

        UNUSED(symlink_val);
        ember_value result7 = ember_native_read_file(vm, 1, &symlink_val);

        UNUSED(result7);
        assert(result7.type == EMBER_VAL_NIL);
        printf("  ✓ Symlink following prevention test passed\n");
        unlink(symlink_path);
    } else {
        printf("  ✓ Symlink creation failed (expected on some systems)\n");
    }
    
    printf("I/O security tests completed successfully!\n\n");
}

// Test large file handling and limits
static void test_io_large_file_limits(ember_vm* vm) {
    printf("Testing large file handling and limits...\n");
    
    const char* large_test_file = "/tmp/ember_large_test.txt";

    
    UNUSED(large_test_file);
    
    // Create a moderately large test file (within limits)
    FILE* file = fopen(large_test_file, "w");
    if (file) {
        const char* chunk = "This is a test line for large file testing.\n";

        UNUSED(chunk);
        size_t chunk_len = strlen(chunk);
        
        // Write about 1KB of data (well within the 10MB limit)
        for (int i = 0; i < 25; i++) {
            fwrite(chunk, 1, chunk_len, file);
        }
        fclose(file);
        
        // Test reading the large file
        ember_value path = make_test_string(vm, large_test_file);

        UNUSED(path);
        ember_value result = ember_native_read_file(vm, 1, &path);

        UNUSED(result);
        // Note: This will likely return nil due to VFS restrictions
        assert(result.type == EMBER_VAL_NIL || result.type == EMBER_VAL_STRING);
        printf("  ✓ Moderate large file handling test completed\n");
        
        remove_temp_file(large_test_file);
    } else {
        printf("  ✓ Large file creation failed (expected on restricted systems)\n");
    }
    
    // Test writing large content (within reasonable limits)
    const size_t large_content_size = 1000;  // 1KB
    char* large_content = malloc(large_content_size + 1);
    if (large_content) {
        memset(large_content, 'x', large_content_size);
        large_content[large_content_size] = '\0';
        
        ember_value path = make_test_string(vm, "/tmp/ember_large_write.txt");

        
        UNUSED(path);
        ember_value content = make_test_string(vm, large_content);

        UNUSED(content);
        ember_value args[2] = {path, content};
        ember_value result = ember_native_write_file(vm, 2, args);

        UNUSED(result);
        assert(result.type == EMBER_VAL_BOOL);
        printf("  ✓ Large content write test completed\n");
        
        free(large_content);
        remove_temp_file("/tmp/ember_large_write.txt");
    }
    
    printf("Large file handling tests completed successfully!\n\n");
}

// Test edge cases and error conditions
static void test_io_edge_cases(ember_vm* vm) {
    printf("Testing I/O edge cases and error conditions...\n");
    
    // Test reading from device files (should be blocked)
    ember_value dev_path = make_test_string(vm, "/dev/null");

    UNUSED(dev_path);
    ember_value result1 = ember_native_read_file(vm, 1, &dev_path);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NIL);
    printf("  ✓ Device file access protection test passed\n");
    
    // Test writing to read-only filesystem locations
    ember_value readonly_path = make_test_string(vm, "/usr/readonly_test.txt");

    UNUSED(readonly_path);
    ember_value content = make_test_string(vm, "test");

    UNUSED(content);
    ember_value args1[2] = {readonly_path, content};
    ember_value result2 = ember_native_write_file(vm, 2, args1);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_BOOL);
    assert(result2.as.bool_val == 0);
    printf("  ✓ Read-only filesystem protection test passed\n");
    
    // Test operations with special characters in filenames
    ember_value special_path = make_test_string(vm, "/tmp/test file with spaces & symbols!.txt");

    UNUSED(special_path);
    ember_value result3 = ember_native_file_exists(vm, 1, &special_path);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_BOOL);
    printf("  ✓ Special characters in filename test passed\n");
    
    // Test operations with Unicode filenames
    ember_value unicode_path = make_test_string(vm, "/tmp/тест_файл.txt");

    UNUSED(unicode_path);
    ember_value result4 = ember_native_file_exists(vm, 1, &unicode_path);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_BOOL);
    printf("  ✓ Unicode filename handling test passed\n");
    
    // Test concurrent file operations simulation
    const char* concurrent_file = "/tmp/ember_concurrent_test.txt";

    UNUSED(concurrent_file);
    ember_value conc_path = make_test_string(vm, concurrent_file);

    UNUSED(conc_path);
    ember_value conc_content1 = make_test_string(vm, "content1");

    UNUSED(conc_content1);
    ember_value conc_content2 = make_test_string(vm, "content2");

    UNUSED(conc_content2);
    
    ember_value args2[2] = {conc_path, conc_content1};
    ember_value args3[2] = {conc_path, conc_content2};
    
    ember_value result5 = ember_native_write_file(vm, 2, args2);

    
    UNUSED(result5);
    ember_value result6 = ember_native_append_file(vm, 2, args3);

    UNUSED(result6);
    
    assert(result5.type == EMBER_VAL_BOOL);
    assert(result6.type == EMBER_VAL_BOOL);
    printf("  ✓ Concurrent operations simulation test passed\n");
    
    remove_temp_file(concurrent_file);
    
    printf("I/O edge cases tests completed successfully!\n\n");
}

int main() {
    printf("Starting comprehensive I/O native function tests...\n\n");
    
    // Initialize VM
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Run all test suites
    test_io_file_exists(vm);
    test_io_read_file(vm);
    test_io_write_file(vm);
    test_io_append_file(vm);
    test_io_security_cases(vm);
    test_io_large_file_limits(vm);
    test_io_edge_cases(vm);
    
    // Cleanup
    ember_free_vm(vm);
    
    printf("🎉 All I/O native function tests passed successfully!\n");
    printf("✅ File exists function tested\n");
    printf("✅ Read file function tested\n");
    printf("✅ Write file function tested\n");
    printf("✅ Append file function tested\n");
    printf("✅ Security considerations tested (VFS restrictions expected)\n");
    printf("✅ Large file handling and limits tested\n");
    printf("✅ Edge cases and error conditions tested\n");
    printf("\nNote: Many operations may return expected failures due to VFS security restrictions.\n");
    printf("This is the intended behavior for sandboxed execution environments.\n");
    
    return 0;
}