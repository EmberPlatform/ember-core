/**
 * Comprehensive Package Loading Test
 * Tests the core package management functionality in Ember
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "ember.h"
#include "../../src/runtime/package/package.h"

// Test utilities
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("❌ FAIL: %s\n", message); \
            return 0; \
        } \
        printf("✅ PASS: %s\n", message); \
    } while(0)

#define TEST_SECTION(name) \
    printf("\n🔍 Testing %s\n", name); \
    printf("================================\n")

// Create test package directory and files
static int create_test_package() {
    // Create test directory structure
    if (mkdir("test_package", 0755) != 0) {
        // Directory might already exist, try to continue
    }
    
    // Create package.ember file
    FILE* package_file = fopen("test_package/package.ember", "w");
    if (!package_file) {
        printf("Failed to create test package.ember file\n");
        return 0;
    }
    
    fprintf(package_file, "# Test Package Implementation\n");
    fprintf(package_file, "print(\"Hello from test package!\")\n");
    fprintf(package_file, "\n");
    fprintf(package_file, "fn greet(name) {\n");
    fprintf(package_file, "    print(\"Hello, \" + name + \"!\")\n");
    fprintf(package_file, "}\n");
    fprintf(package_file, "\n");
    fprintf(package_file, "fn add(a, b) {\n");
    fprintf(package_file, "    return a + b\n");
    fprintf(package_file, "}\n");
    fprintf(package_file, "\n");
    fprintf(package_file, "# Export functions for other modules\n");
    fprintf(package_file, "print(\"Test package loaded successfully\")\n");
    fclose(package_file);
    
    // Create package.toml manifest
    FILE* manifest_file = fopen("test_package/package.toml", "w");
    if (!manifest_file) {
        printf("Failed to create test package.toml file\n");
        return 0;
    }
    
    fprintf(manifest_file, "[package]\n");
    fprintf(manifest_file, "name = \"test-package\"\n");
    fprintf(manifest_file, "version = \"1.0.0\"\n");
    fprintf(manifest_file, "description = \"A test package for Ember\"\n");
    fprintf(manifest_file, "author = \"Test Author\"\n");
    fprintf(manifest_file, "\n");
    fprintf(manifest_file, "[dependencies]\n");
    fprintf(manifest_file, "# No dependencies for this test\n");
    fclose(manifest_file);
    
    return 1;
}

// Test semantic versioning functionality
static int test_semantic_versioning() {
    TEST_SECTION("Semantic Versioning");
    
    // Test version comparison
    TEST_ASSERT(ember_package_version_compare("1.0.0", "1.0.0") == 0, 
                "Version comparison: equal versions");
    TEST_ASSERT(ember_package_version_compare("1.0.1", "1.0.0") > 0, 
                "Version comparison: patch version greater");
    TEST_ASSERT(ember_package_version_compare("1.1.0", "1.0.0") > 0, 
                "Version comparison: minor version greater");
    TEST_ASSERT(ember_package_version_compare("2.0.0", "1.9.9") > 0, 
                "Version comparison: major version greater");
    
    // Test version constraints
    TEST_ASSERT(ember_package_version_satisfies("1.2.3", "^1.2.0"), 
                "Caret constraint: compatible version");
    TEST_ASSERT(!ember_package_version_satisfies("2.0.0", "^1.2.0"), 
                "Caret constraint: incompatible major version");
    TEST_ASSERT(ember_package_version_satisfies("1.2.5", "~1.2.3"), 
                "Tilde constraint: compatible patch version");
    TEST_ASSERT(!ember_package_version_satisfies("1.3.0", "~1.2.3"), 
                "Tilde constraint: incompatible minor version");
    TEST_ASSERT(ember_package_version_satisfies("1.5.0", ">=1.2.0"), 
                "Greater than or equal constraint");
    TEST_ASSERT(ember_package_version_satisfies("1.0.0", "1.x"), 
                "Wildcard constraint");
    
    return 1;
}

// Test package structure validation
static int test_package_validation() {
    TEST_SECTION("Package Structure Validation");
    
    // Test package name validation
    TEST_ASSERT(ember_package_validate_name("valid-package") == 0, 
                "Valid package name accepted");
    TEST_ASSERT(ember_package_validate_name("../invalid") != 0, 
                "Invalid package name with path traversal rejected");
    TEST_ASSERT(ember_package_validate_name("valid123") == 0, 
                "Package name with numbers accepted");
    TEST_ASSERT(ember_package_validate_name("") != 0, 
                "Empty package name rejected");
    
    // Test package structure validation
    TEST_ASSERT(ember_package_validate_structure("test_package"), 
                "Valid package structure accepted");
    TEST_ASSERT(!ember_package_validate_structure("nonexistent_package"), 
                "Nonexistent package structure rejected");
    
    return 1;
}

// Test package discovery and loading
static int test_package_discovery_and_loading() {
    TEST_SECTION("Package Discovery and Loading");
    
    EmberPackage package;
    
    // Test package discovery
    TEST_ASSERT(ember_package_discover("test-package", &package), 
                "Package discovery succeeds");
    TEST_ASSERT(strcmp(package.name, "test-package") == 0, 
                "Package name correctly set");
    
    // Set local path for our test package
    strcpy(package.local_path, "test_package");
    
    // Test package loading
    TEST_ASSERT(ember_package_load(&package), 
                "Package loading succeeds");
    TEST_ASSERT(package.loaded, 
                "Package marked as loaded");
    TEST_ASSERT(package.handle != NULL, 
                "Package VM handle created");
    
    // Test package unloading
    TEST_ASSERT(ember_package_unload(&package), 
                "Package unloading succeeds");
    TEST_ASSERT(!package.loaded, 
                "Package marked as unloaded");
    
    return 1;
}

// Test package registry functionality
static int test_package_registry() {
    TEST_SECTION("Package Registry");
    
    // Initialize package registry
    EmberPackageRegistry* registry = ember_package_registry_init();
    TEST_ASSERT(registry != NULL, 
                "Package registry initialization");
    TEST_ASSERT(registry->count == 0, 
                "Registry starts empty");
    
    // Create and add a test package
    EmberPackage package;
    ember_package_discover("test-package", &package);
    strcpy(package.local_path, "test_package");
    strcpy(package.version, "1.0.0");
    package.verified = true;
    
    TEST_ASSERT(ember_package_registry_add(registry, &package), 
                "Package added to registry");
    TEST_ASSERT(registry->count == 1, 
                "Registry count updated");
    
    // Test package finding
    EmberPackage* found = ember_package_registry_find(registry, "test-package");

    UNUSED(found);
    TEST_ASSERT(found != NULL, 
                "Package found in registry");
    TEST_ASSERT(strcmp(found->name, "test-package") == 0, 
                "Correct package found");
    
    // Test duplicate handling
    TEST_ASSERT(ember_package_registry_add(registry, &package), 
                "Duplicate package handled gracefully");
    TEST_ASSERT(registry->count == 1, 
                "Registry count remains same for duplicate");
    
    // Cleanup registry
    ember_package_registry_cleanup(registry);
    
    return 1;
}

// Test project management functionality
static int test_project_management() {
    TEST_SECTION("Project Management");
    
    // Test project initialization
    EmberProject* project = ember_project_init("test-project", "1.0.0");
    TEST_ASSERT(project != NULL, 
                "Project initialization");
    TEST_ASSERT(strcmp(project->name, "test-project") == 0, 
                "Project name set correctly");
    TEST_ASSERT(project->dependency_count == 0, 
                "Project starts with no dependencies");
    
    // Test adding dependencies
    TEST_ASSERT(ember_project_add_dependency(project, "json-parser", "^2.1.0"), 
                "Dependency added successfully");
    TEST_ASSERT(project->dependency_count == 1, 
                "Dependency count updated");
    
    // Test adding another dependency
    TEST_ASSERT(ember_project_add_dependency(project, "http-client", "~1.5.0"), 
                "Second dependency added");
    TEST_ASSERT(project->dependency_count == 2, 
                "Dependency count updated to 2");
    
    // Test updating existing dependency
    TEST_ASSERT(ember_project_add_dependency(project, "json-parser", "^2.2.0"), 
                "Existing dependency updated");
    TEST_ASSERT(project->dependency_count == 2, 
                "Dependency count remains same for update");
    
    // Cleanup project
    ember_project_cleanup(project);
    
    return 1;
}

// Test global package system
static int test_global_package_system() {
    TEST_SECTION("Global Package System");
    
    // Test system initialization
    TEST_ASSERT(ember_package_system_init(), 
                "Global package system initialization");
    
    // Test getting global registry
    EmberPackageRegistry* registry = ember_package_get_global_registry();
    TEST_ASSERT(registry != NULL, 
                "Global registry accessible");
    
    // Test multiple initializations (should be safe)
    TEST_ASSERT(ember_package_system_init(), 
                "Multiple initializations handled safely");
    
    // Cleanup system
    ember_package_system_cleanup();
    
    return 1;
}

// Test manifest validation
static int test_manifest_validation() {
    TEST_SECTION("Manifest Validation");
    
    // Test valid manifest
    const char* valid_manifest = 
        "[package]\n"
        "name = \"test-package\"\n"
        "version = \"1.0.0\"\n"
        "description = \"A test package\"\n";

    UNUSED(valid_manifest);
    
    TEST_ASSERT(ember_package_validate_manifest(valid_manifest), 
                "Valid manifest accepted");
    
    // Test invalid manifest (missing name)
    const char* invalid_manifest = 
        "[package]\n"
        "version = \"1.0.0\"\n"
        "description = \"A test package\"\n";

    UNUSED(invalid_manifest);
    
    TEST_ASSERT(!ember_package_validate_manifest(invalid_manifest), 
                "Invalid manifest (missing name) rejected");
    
    // Test manifest with dangerous content
    const char* dangerous_manifest = 
        "[package]\n"
        "name = \"test-package\"\n"
        "version = \"1.0.0\"\n"
        "path = \"../../../etc/passwd\"\n";

    UNUSED(dangerous_manifest);
    
    TEST_ASSERT(!ember_package_validate_manifest(dangerous_manifest), 
                "Dangerous manifest rejected");
    
    return 1;
}

// Cleanup test files
static void cleanup_test_files() {
    printf("\n🧹 Cleaning up test files...\n");
    unlink("test_package/package.ember");
    unlink("test_package/package.toml");
    rmdir("test_package");
}

// Main test runner
int main() {
    printf("🚀 Ember Package Management System Test Suite\n");
    printf("=============================================\n");
    
    int tests_passed = 0;

    
    UNUSED(tests_passed);
    int total_tests = 8;

    UNUSED(total_tests);
    
    // Create test package first
    if (!create_test_package()) {
        printf("❌ Failed to create test package files\n");
        return 1;
    }
    
    // Run all tests
    if (test_semantic_versioning()) tests_passed++;
    if (test_package_validation()) tests_passed++;
    if (test_package_discovery_and_loading()) tests_passed++;
    if (test_package_registry()) tests_passed++;
    if (test_project_management()) tests_passed++;
    if (test_global_package_system()) tests_passed++;
    if (test_manifest_validation()) tests_passed++;
    
    // Cleanup
    cleanup_test_files();
    
    // Print results
    printf("\n📊 Test Results\n");
    printf("===============\n");
    printf("Tests Passed: %d/%d\n", tests_passed, total_tests);
    
    if (tests_passed == total_tests) {
        printf("🎉 All tests passed! Package management system is working correctly.\n");
        return 0;
    } else {
        printf("❌ %d tests failed. Please check the implementation.\n", total_tests - tests_passed);
        return 1;
    }
}