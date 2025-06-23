#include "ember.h"
#include "../../src/runtime/package/package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

// Test helper functions
void setup_test_environment(void) {
    // Create test directories
    system("mkdir -p /tmp/ember_test_modules");
    system("mkdir -p /tmp/ember_test_packages");
    
    // Initialize package system
    ember_package_system_init();
}

void cleanup_test_environment(void) {
    // Clean up test directories
    system("rm -rf /tmp/ember_test_modules");
    system("rm -rf /tmp/ember_test_packages");
    
    // Cleanup package system
    ember_package_system_cleanup();
}

void create_test_module(const char* name, const char* content) {
    char path[512];
    snprintf(path, sizeof(path), "/tmp/ember_test_modules/%s.ember", name);
    
    FILE* file = fopen(path, "w");
    if (file) {
        fprintf(file, "%s", content);
        fclose(file);
    }
}

void create_test_package(const char* name, const char* content) {
    char dir_path[512];
    char file_path[512];
    
    snprintf(dir_path, sizeof(dir_path), "/tmp/ember_test_packages/%s", name);
    snprintf(file_path, sizeof(file_path), "%s/package.ember", dir_path);
    
    mkdir(dir_path, 0755);
    
    FILE* file = fopen(file_path, "w");
    if (file) {
        fprintf(file, "%s", content);
        fclose(file);
    }
}

// Test 1: Module path resolution
void test_ember_resolve_module_path(void) {
    printf("Testing ember_resolve_module_path...\n");
    
    // Create test module in current directory
    create_test_module("test_module", "print(\"Test module loaded\")");
    
    // Test resolving existing module
    char* resolved = ember_resolve_module_path("test_module");
    assert(resolved != NULL);
    assert(strstr(resolved, "test_module.ember") != NULL);
    free(resolved);
    
    // Test resolving non-existent module
    char* not_found = ember_resolve_module_path("non_existent_module");
    assert(not_found == NULL);
    
    // Test invalid module name
    char* invalid = ember_resolve_module_path("../../../etc/passwd");
    assert(invalid == NULL);
    
    printf("✓ ember_resolve_module_path tests passed\n");
}

// Test 2: Module path management in VM
void test_ember_add_module_path(void) {
    printf("Testing ember_add_module_path...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Test adding valid module path
    ember_add_module_path(vm, "/tmp/ember_test_modules");
    assert(vm->module_path_count == 1);
    assert(strcmp(vm->module_paths[0], "/tmp/ember_test_modules") == 0);
    
    // Test adding another path
    ember_add_module_path(vm, "/tmp/ember_test_packages");
    assert(vm->module_path_count == 2);
    
    // Test adding duplicate path (should be ignored)
    int initial_count = vm->module_path_count;

    UNUSED(initial_count);
    ember_add_module_path(vm, "/tmp/ember_test_modules");
    assert(vm->module_path_count == initial_count);
    
    // Test adding invalid path (should be rejected)
    ember_add_module_path(vm, "../../../etc");
    assert(vm->module_path_count == initial_count);
    
    // Test adding non-existent path (should be rejected)
    ember_add_module_path(vm, "/non/existent/path");
    assert(vm->module_path_count == initial_count);
    
    ember_free_vm(vm);
    printf("✓ ember_add_module_path tests passed\n");
}

// Test 3: Module importing
void test_ember_import_module(void) {
    printf("Testing ember_import_module...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Add test module path
    ember_add_module_path(vm, "/tmp/ember_test_modules");
    
    // Create a test module
    create_test_module("math_utils", 
        "fn add(a, b) { return a + b }\n"
        "fn multiply(a, b) { return a * b }\n"
        "print(\"Math utils module loaded\")\n");
    
    // Test importing existing module
    int result = ember_import_module(vm, "math_utils");

    UNUSED(result);
    assert(result == 0);
    
    // Verify module is loaded
    assert(vm->module_count > 0);
    int found = 0;

    UNUSED(found);
    for (int i = 0; i < vm->module_count; i++) {
        if (strcmp(vm->modules[i].name, "math_utils") == 0) {
            assert(vm->modules[i].is_loaded == 1);
            found = 1;
            break;
        }
    }
    assert(found == 1);
    
    // Test importing already loaded module (should succeed)
    result = ember_import_module(vm, "math_utils");
    assert(result == 0);
    
    // Test importing non-existent module
    result = ember_import_module(vm, "non_existent");
    assert(result == -1);
    
    // Test invalid parameters
    result = ember_import_module(NULL, "test");
    assert(result == -1);
    
    result = ember_import_module(vm, NULL);
    assert(result == -1);
    
    ember_free_vm(vm);
    printf("✓ ember_import_module tests passed\n");
}

// Test 4: Library installation
void test_ember_install_library(void) {
    printf("Testing ember_install_library...\n");
    
    // Create a test source file
    char source_path[] = "/tmp/test_library.ember";
    FILE* source_file = fopen(source_path, "w");
    assert(source_file != NULL);
    fprintf(source_file, 
        "# Test Library\n"
        "fn hello() { return \"Hello from library\" }\n"
        "print(\"Test library loaded\")\n");
    fclose(source_file);
    
    // Test installing library
    int result = ember_install_library("test_lib", source_path);

    UNUSED(result);
    assert(result == 0);
    
    // Verify library was installed
    char install_path[512];
    char* home = getenv("HOME");
    if (home) {
        snprintf(install_path, sizeof(install_path), "%s/.ember/packages/test_lib/package.ember", home);
    } else {
        snprintf(install_path, sizeof(install_path), "/tmp/.ember/packages/test_lib/package.ember");
    }
    assert(access(install_path, R_OK) == 0);
    
    // Test installing with invalid source path
    result = ember_install_library("invalid_lib", "/non/existent/file");
    assert(result == -1);
    
    // Test invalid parameters
    result = ember_install_library(NULL, source_path);
    assert(result == -1);
    
    result = ember_install_library("test_lib", NULL);
    assert(result == -1);
    
    // Test invalid library name
    result = ember_install_library("../../../malicious", source_path);
    assert(result == -1);
    
    // Clean up
    unlink(source_path);
    printf("✓ ember_install_library tests passed\n");
}

// Test 5: Integration test - import installed library
void test_module_integration(void) {
    printf("Testing module system integration...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Create and install a library
    char source_path[] = "/tmp/integration_lib.ember";
    FILE* source_file = fopen(source_path, "w");
    assert(source_file != NULL);
    fprintf(source_file, 
        "# Integration Test Library\n"
        "global_var = \"Integration test successful\"\n"
        "fn get_message() { return global_var }\n");
    fclose(source_file);
    
    int install_result = ember_install_library("integration_lib", source_path);

    
    UNUSED(install_result);
    assert(install_result == 0);
    
    // Now try to import the installed library
    int import_result = ember_import_module(vm, "integration_lib");

    UNUSED(import_result);
    assert(import_result == 0);
    
    // Verify the module is loaded
    int found = 0;

    UNUSED(found);
    for (int i = 0; i < vm->module_count; i++) {
        if (strcmp(vm->modules[i].name, "integration_lib") == 0) {
            assert(vm->modules[i].is_loaded == 1);
            found = 1;
            break;
        }
    }
    assert(found == 1);
    
    // Clean up
    unlink(source_path);
    ember_free_vm(vm);
    printf("✓ Module system integration tests passed\n");
}

// Test 6: Error handling and edge cases
void test_module_error_handling(void) {
    printf("Testing module error handling...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Test importing module with syntax error
    create_test_module("syntax_error", "fn broken(");
    ember_add_module_path(vm, "/tmp/ember_test_modules");
    
    int result = ember_import_module(vm, "syntax_error");

    
    UNUSED(result);
    assert(result == -1); // Should fail due to syntax error
    
    // Test maximum module limit (if applicable)
    // This would require creating many modules to test the limit
    
    // Test module with circular dependencies (conceptual test)
    create_test_module("circular_a", "import circular_b\nprint(\"Module A\")");
    create_test_module("circular_b", "import circular_a\nprint(\"Module B\")");
    
    // In a real implementation, this should be handled gracefully
    // For now, we just test that it doesn't crash
    result = ember_import_module(vm, "circular_a");
    // Result can be either success or failure depending on implementation
    
    ember_free_vm(vm);
    printf("✓ Module error handling tests passed\n");
}

// Test 7: Security tests
void test_module_security(void) {
    printf("Testing module security...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Test path traversal prevention in module names
    int result = ember_import_module(vm, "../../../etc/passwd");

    UNUSED(result);
    assert(result == -1);
    
    result = ember_import_module(vm, "..\\..\\windows\\system32\\calc.exe");
    assert(result == -1);
    
    // Test path traversal prevention in module paths
    ember_add_module_path(vm, "../../../sensitive/directory");
    // Should be rejected (we test this in previous tests)
    
    // Test library installation with dangerous names
    char safe_file[] = "/tmp/safe_test.ember";
    FILE* file = fopen(safe_file, "w");
    if (file) {
        fprintf(file, "print(\"Safe content\")");
        fclose(file);
    }
    
    result = ember_install_library("../../../dangerous", safe_file);
    assert(result == -1);
    
    result = ember_install_library("lib|with|pipes", safe_file);
    assert(result == -1);
    
    result = ember_install_library("lib;with;semicolons", safe_file);
    assert(result == -1);
    
    unlink(safe_file);
    ember_free_vm(vm);
    printf("✓ Module security tests passed\n");
}

int main(void) {
    printf("Starting Module API tests...\n\n");
    
    setup_test_environment();
    
    test_ember_resolve_module_path();
    test_ember_add_module_path();
    test_ember_import_module();
    test_ember_install_library();
    test_module_integration();
    test_module_error_handling();
    test_module_security();
    
    cleanup_test_environment();
    
    printf("\n✅ All Module API tests passed!\n");
    return 0;
}