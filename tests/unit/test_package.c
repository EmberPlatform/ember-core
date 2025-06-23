#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include "ember.h"
#include "../../src/runtime/package/package.h"
#include "test_ember_internal.h"

// Macro to mark variables as intentionally unused
#define UNUSED(x) ((void)(x))

// Test helper to create temporary directory
static char temp_dir[512];
static int temp_dir_created = 0;

static void setup_temp_dir(void) {
    if (!temp_dir_created) {
        snprintf(temp_dir, sizeof(temp_dir), "/tmp/ember_test_packages_%d", getpid());
        ember_package_create_directory_recursive(temp_dir);
        temp_dir_created = 1;
    }
}

static void cleanup_temp_dir(void) {
    if (temp_dir_created) {
        char cleanup_cmd[600];
        snprintf(cleanup_cmd, sizeof(cleanup_cmd), "rm -rf %s", temp_dir);
        int result = system(cleanup_cmd);
        (void)result; // Ignore result for cleanup
        temp_dir_created = 0;
    }
}

// Test package registry operations
static void test_package_registry(void) {
    printf("Testing package registry operations...\n");
    
    // Test registry initialization
    EmberPackageRegistry* registry = ember_package_registry_init();
    assert(registry != NULL);
    assert(registry->packages != NULL);
    assert(registry->count == 0);
    assert(registry->capacity == 16);
    printf("  ✓ Package registry initialization test passed\n");
    
    // Test adding a package
    EmberPackage test_package;
    memset(&test_package, 0, sizeof(EmberPackage));
    strncpy(test_package.name, "test_package", EMBER_PACKAGE_MAX_NAME_LEN - 1);
    strncpy(test_package.version, "1.0.0", EMBER_PACKAGE_MAX_VERSION_LEN - 1);
    test_package.verified = true;
    test_package.loaded = false;
    
    bool add_result = ember_package_registry_add(registry, &test_package);

    
    UNUSED(add_result);

    
    UNUSED(add_result);
    assert(add_result == true);
    assert(registry->count == 1);
    printf("  ✓ Package registry add test passed\n");
    
    // Test finding a package
    EmberPackage* found_package = ember_package_registry_find(registry, "test_package");

    UNUSED(found_package);

    UNUSED(found_package);
    assert(found_package != NULL);
    assert(strcmp(found_package->name, "test_package") == 0);
    assert(strcmp(found_package->version, "1.0.0") == 0);
    printf("  ✓ Package registry find test passed\n");
    
    // Test finding nonexistent package
    EmberPackage* nonexistent = ember_package_registry_find(registry, "nonexistent_package");

    UNUSED(nonexistent);

    UNUSED(nonexistent);
    assert(nonexistent == NULL);
    printf("  ✓ Package registry nonexistent find test passed\n");
    
    // Test updating existing package
    EmberPackage updated_package = test_package;
    strncpy(updated_package.version, "2.0.0", EMBER_PACKAGE_MAX_VERSION_LEN - 1);
    bool update_result = ember_package_registry_add(registry, &updated_package);

    UNUSED(update_result);

    UNUSED(update_result);
    assert(update_result == true);
    assert(registry->count == 1); // Should still be 1
    
    EmberPackage* updated_found = ember_package_registry_find(registry, "test_package");

    
    UNUSED(updated_found);

    
    UNUSED(updated_found);
    assert(strcmp(updated_found->version, "2.0.0") == 0);
    printf("  ✓ Package registry update test passed\n");
    
    // Test registry capacity expansion
    for (int i = 0; i < 20; i++) {
        EmberPackage new_package;
        memset(&new_package, 0, sizeof(EmberPackage));
        snprintf(new_package.name, EMBER_PACKAGE_MAX_NAME_LEN, "package_%d", i);
        strncpy(new_package.version, "1.0.0", EMBER_PACKAGE_MAX_VERSION_LEN - 1);
        ember_package_registry_add(registry, &new_package);
    }
    assert(registry->count == 21); // 1 original + 20 new
    assert(registry->capacity > 16); // Should have expanded
    printf("  ✓ Package registry expansion test passed\n");
    
    // Test error cases
    assert(ember_package_registry_add(NULL, &test_package) == false);
    assert(ember_package_registry_add(registry, NULL) == false);
    assert(ember_package_registry_find(NULL, "test") == NULL);
    assert(ember_package_registry_find(registry, NULL) == NULL);
    printf("  ✓ Package registry error handling test passed\n");
    
    // Cleanup
    ember_package_registry_cleanup(registry);
    printf("  ✓ Package registry cleanup test passed\n");
    
    printf("Package registry tests completed successfully!\n\n");
}

// Test package discovery
static void test_package_discovery(void) {
    printf("Testing package discovery...\n");
    
    setup_temp_dir();
    
    // Test basic package discovery
    EmberPackage discovered_package;
    bool discovery_result = ember_package_discover("valid_package", &discovered_package);

    UNUSED(discovery_result);

    UNUSED(discovery_result);
    assert(discovery_result == true);
    assert(strcmp(discovered_package.name, "valid_package") == 0);
    assert(strcmp(discovered_package.version, "latest") == 0);
    assert(discovered_package.verified == false);
    assert(discovered_package.loaded == false);
    printf("  ✓ Basic package discovery test passed\n");
    
    // Test package discovery with invalid names
    EmberPackage invalid_package;
    bool invalid_result1 = ember_package_discover("../invalid", &invalid_package);

    UNUSED(invalid_result1);

    UNUSED(invalid_result1);
    assert(invalid_result1 == false);
    
    bool invalid_result2 = ember_package_discover("invalid/name", &invalid_package);

    
    UNUSED(invalid_result2);

    
    UNUSED(invalid_result2);
    assert(invalid_result2 == false);
    
    bool invalid_result3 = ember_package_discover("invalid<name", &invalid_package);

    
    UNUSED(invalid_result3);

    
    UNUSED(invalid_result3);
    assert(invalid_result3 == false);
    printf("  ✓ Invalid package name discovery test passed\n");
    
    // Test NULL parameter handling
    bool null_result1 = ember_package_discover(NULL, &discovered_package);

    UNUSED(null_result1);

    UNUSED(null_result1);
    assert(null_result1 == false);
    
    bool null_result2 = ember_package_discover("valid", NULL);

    
    UNUSED(null_result2);

    
    UNUSED(null_result2);
    assert(null_result2 == false);
    printf("  ✓ Package discovery NULL handling test passed\n");
    
    printf("Package discovery tests completed successfully!\n\n");
}

// Test package validation functions
static void test_package_validation(void) {
    printf("Testing package validation...\n");
    
    // Test valid package names
    assert(ember_package_validate_name("valid_package") == 0);
    assert(ember_package_validate_name("another-valid-package") == 0);
    assert(ember_package_validate_name("package123") == 0);
    printf("  ✓ Valid package name validation test passed\n");
    
    // Test invalid package names
    assert(ember_package_validate_name("../invalid") == -1);
    assert(ember_package_validate_name("invalid/name") == -1);
    assert(ember_package_validate_name("invalid\\name") == -1);
    assert(ember_package_validate_name("invalid<name") == -1);
    assert(ember_package_validate_name("invalid>name") == -1);
    assert(ember_package_validate_name("invalid|name") == -1);
    assert(ember_package_validate_name("invalid&name") == -1);
    assert(ember_package_validate_name("invalid;name") == -1);
    assert(ember_package_validate_name("invalid$name") == -1);
    assert(ember_package_validate_name("invalid`name") == -1);
    printf("  ✓ Invalid package name validation test passed\n");
    
    // Test edge cases
    assert(ember_package_validate_name("") == -1);
    assert(ember_package_validate_name(NULL) == -1);
    
    // Test very long package name
    char long_name[100];
    memset(long_name, 'a', 99);
    long_name[99] = '\0';
    assert(ember_package_validate_name(long_name) == -1);
    printf("  ✓ Package name edge cases validation test passed\n");
    
    // Test directory creation
    setup_temp_dir();
    
    char test_dir[600];
    snprintf(test_dir, sizeof(test_dir), "%s/test_create_dir", temp_dir);
    assert(ember_package_create_directory_recursive(test_dir) == 0);
    
    // Verify directory was created
    struct stat st;
    assert(stat(test_dir, &st) == 0);
    assert(S_ISDIR(st.st_mode));
    (void)st; // Silence compiler warning - st is used in assert
    printf("  ✓ Directory creation test passed\n");
    
    // Test creating nested directories
    char nested_dir[600];
    snprintf(nested_dir, sizeof(nested_dir), "%s/nested/deep/directory", temp_dir);
    assert(ember_package_create_directory_recursive(nested_dir) == 0);
    assert(stat(nested_dir, &st) == 0);
    assert(S_ISDIR(st.st_mode));
    printf("  ✓ Nested directory creation test passed\n");
    
    // Test invalid directory paths
    assert(ember_package_create_directory_recursive("../invalid") == -1);
    assert(ember_package_create_directory_recursive("invalid<path") == -1);
    assert(ember_package_create_directory_recursive(NULL) == -1);
    assert(ember_package_create_directory_recursive("") == -1);
    
    // Test very long path
    char long_path[600];
    memset(long_path, 'a', 599);
    long_path[599] = '\0';
    assert(ember_package_create_directory_recursive(long_path) == -1);
    printf("  ✓ Directory creation error handling test passed\n");
    
    printf("Package validation tests completed successfully!\n\n");
}

// Test package download and loading
static void test_package_download_load(void) {
    printf("Testing package download and loading...\n");
    
    setup_temp_dir();
    
    // Test package download
    EmberPackage download_package;
    memset(&download_package, 0, sizeof(EmberPackage));
    strncpy(download_package.name, "download_test", EMBER_PACKAGE_MAX_NAME_LEN - 1);
    strncpy(download_package.version, "1.0.0", EMBER_PACKAGE_MAX_VERSION_LEN - 1);
    
    bool download_result = ember_package_download(&download_package);

    
    UNUSED(download_result);

    
    UNUSED(download_result);
    assert(download_result == true);
    assert(download_package.verified == true);
    assert(strlen(download_package.local_path) > 0);
    printf("  ✓ Package download test passed\n");
    
    // Verify package file was created
    char package_file[700];
    snprintf(package_file, sizeof(package_file), "%s/package.ember", download_package.local_path);
    assert(access(package_file, F_OK) == 0);
    printf("  ✓ Package file creation test passed\n");
    
    // Test package loading
    bool load_result = ember_package_load(&download_package);

    UNUSED(load_result);

    UNUSED(load_result);
    assert(load_result == true);
    assert(download_package.loaded == true);
    printf("  ✓ Package loading test passed\n");
    
    // Test package unloading
    bool unload_result = ember_package_unload(&download_package);

    UNUSED(unload_result);

    UNUSED(unload_result);
    assert(unload_result == true);
    assert(download_package.loaded == false);
    printf("  ✓ Package unloading test passed\n");
    
    // Test error cases
    assert(ember_package_download(NULL) == false);
    assert(ember_package_load(NULL) == false);
    assert(ember_package_unload(NULL) == false);
    
    // Test download with invalid package name
    EmberPackage invalid_download;
    memset(&invalid_download, 0, sizeof(EmberPackage));
    strncpy(invalid_download.name, "../invalid", EMBER_PACKAGE_MAX_NAME_LEN - 1);
    assert(ember_package_download(&invalid_download) == false);
    printf("  ✓ Package download/load error handling test passed\n");
    
    printf("Package download and loading tests completed successfully!\n\n");
}

// Test semantic versioning functions
static void test_semantic_versioning(void) {
    printf("Testing semantic versioning...\n");
    
    // Test version satisfaction
    assert(ember_package_version_satisfies("1.0.0", "*") == true);
    assert(ember_package_version_satisfies("1.0.0", "latest") == true);
    assert(ember_package_version_satisfies("1.0.0", "1.0.0") == true);
    assert(ember_package_version_satisfies("1.0.0", "1.0.x") == true);
    assert(ember_package_version_satisfies("1.0.0", "2.0.0") == false);
    printf("  ✓ Version satisfaction test passed\n");
    
    // Test version comparison
    assert(ember_package_version_compare("1.0.0", "1.0.0") == 0);
    assert(ember_package_version_compare("1.0.0", "2.0.0") < 0);
    assert(ember_package_version_compare("2.0.0", "1.0.0") > 0);
    printf("  ✓ Version comparison test passed\n");
    
    // Test NULL handling
    assert(ember_package_version_satisfies(NULL, "1.0.0") == false);
    assert(ember_package_version_satisfies("1.0.0", NULL) == false);
    assert(ember_package_version_compare(NULL, "1.0.0") == 0);
    assert(ember_package_version_compare("1.0.0", NULL) == 0);
    printf("  ✓ Version function NULL handling test passed\n");
    
    printf("Semantic versioning tests completed successfully!\n\n");
}

// Test project management
static void test_project_management(void) {
    printf("Testing project management...\n");
    
    // Test project initialization
    EmberProject* project = ember_project_init("test_project", "1.0.0");
    assert(project != NULL);
    assert(strcmp(project->name, "test_project") == 0);
    assert(strcmp(project->version, "1.0.0") == 0);
    assert(project->dependencies != NULL);
    assert(project->dependency_count == 0);
    assert(project->dependency_capacity == 8);
    printf("  ✓ Project initialization test passed\n");
    
    // Test adding dependencies
    bool add_dep1 = ember_project_add_dependency(project, "dep1", "1.0.0");

    UNUSED(add_dep1);

    UNUSED(add_dep1);
    assert(add_dep1 == true);
    assert(project->dependency_count == 1);
    assert(strcmp(project->dependencies[0].name, "dep1") == 0);
    assert(strcmp(project->dependencies[0].version, "1.0.0") == 0);
    printf("  ✓ Project add dependency test passed\n");
    
    // Test updating existing dependency
    bool update_dep = ember_project_add_dependency(project, "dep1", "2.0.0");

    UNUSED(update_dep);

    UNUSED(update_dep);
    assert(update_dep == true);
    assert(project->dependency_count == 1); // Should still be 1
    assert(strcmp(project->dependencies[0].version, "2.0.0") == 0);
    printf("  ✓ Project update dependency test passed\n");
    
    // Test adding multiple dependencies
    for (int i = 2; i <= 10; i++) {
        char dep_name[32];
        snprintf(dep_name, sizeof(dep_name), "dep%d", i);
        ember_project_add_dependency(project, dep_name, "1.0.0");
    }
    assert(project->dependency_count == 10);
    assert(project->dependency_capacity > 8); // Should have expanded
    printf("  ✓ Project multiple dependencies test passed\n");
    
    // Test project save/load
    setup_temp_dir();
    char project_file[600];
    snprintf(project_file, sizeof(project_file), "%s/test_project.toml", temp_dir);
    
    strncpy(project->description, "Test project description", 255);
    strncpy(project->author, "Test Author", 127);
    
    bool save_result = ember_project_save_to_file(project, project_file);

    
    UNUSED(save_result);

    
    UNUSED(save_result);
    assert(save_result == true);
    assert(access(project_file, F_OK) == 0);
    printf("  ✓ Project save test passed\n");
    
    // Test project loading
    EmberProject* loaded_project = NULL;
    bool load_result = ember_project_load_from_file(project_file, &loaded_project);

    UNUSED(load_result);

    UNUSED(load_result);
    assert(load_result == true);
    assert(loaded_project != NULL);
    assert(strcmp(loaded_project->name, "test_project") == 0);
    assert(strcmp(loaded_project->version, "1.0.0") == 0);
    assert(strcmp(loaded_project->description, "Test project description") == 0);
    assert(strcmp(loaded_project->author, "Test Author") == 0);
    assert(loaded_project->dependency_count == 10);
    printf("  ✓ Project load test passed\n");
    
    // Test default project generation
    char default_dir[600];
    snprintf(default_dir, sizeof(default_dir), "%s/default_project", temp_dir);
    ember_package_create_directory_recursive(default_dir);
    
    bool default_result = ember_project_generate_default(default_dir);

    
    UNUSED(default_result);

    
    UNUSED(default_result);
    assert(default_result == true);
    
    char default_file[700];
    snprintf(default_file, sizeof(default_file), "%s/ember.toml", default_dir);
    assert(access(default_file, F_OK) == 0);
    printf("  ✓ Project default generation test passed\n");
    
    // Test error cases
    assert(ember_project_init(NULL, "1.0.0") == NULL);
    assert(ember_project_init("test", NULL) == NULL);
    assert(ember_project_add_dependency(NULL, "dep", "1.0.0") == false);
    assert(ember_project_add_dependency(project, NULL, "1.0.0") == false);
    assert(ember_project_add_dependency(project, "dep", NULL) == false);
    assert(ember_project_save_to_file(NULL, project_file) == false);
    assert(ember_project_save_to_file(project, NULL) == false);
    assert(ember_project_load_from_file(NULL, &loaded_project) == false);
    assert(ember_project_load_from_file(project_file, NULL) == false);
    printf("  ✓ Project management error handling test passed\n");
    
    // Cleanup
    ember_project_cleanup(project);
    ember_project_cleanup(loaded_project);
    printf("  ✓ Project cleanup test passed\n");
    
    printf("Project management tests completed successfully!\n\n");
}

// Test global package system
static void test_global_package_system(void) {
    printf("Testing global package system...\n");
    
    // Test system initialization
    assert(ember_package_system_init() == true);
    EmberPackageRegistry* global_registry = ember_package_get_global_registry();
    assert(global_registry != NULL);
    printf("  ✓ Global package system initialization test passed\n");
    
    // Test double initialization
    assert(ember_package_system_init() == true); // Should succeed (already initialized)
    printf("  ✓ Global package system double initialization test passed\n");
    
    // Test adding to global registry
    EmberPackage global_package;
    memset(&global_package, 0, sizeof(EmberPackage));
    strncpy(global_package.name, "global_test", EMBER_PACKAGE_MAX_NAME_LEN - 1);
    strncpy(global_package.version, "1.0.0", EMBER_PACKAGE_MAX_VERSION_LEN - 1);
    
    bool global_add = ember_package_registry_add(global_registry, &global_package);

    
    UNUSED(global_add);

    
    UNUSED(global_add);
    assert(global_add == true);
    assert(global_registry->count >= 1);
    printf("  ✓ Global registry package add test passed\n");
    
    // Test finding in global registry
    EmberPackage* found_global = ember_package_registry_find(global_registry, "global_test");

    UNUSED(found_global);

    UNUSED(found_global);
    assert(found_global != NULL);
    assert(strcmp(found_global->name, "global_test") == 0);
    printf("  ✓ Global registry package find test passed\n");
    
    // Test system cleanup
    ember_package_system_cleanup();
    assert(ember_package_get_global_registry() == NULL);
    printf("  ✓ Global package system cleanup test passed\n");
    
    // Test cleanup of already cleaned system
    ember_package_system_cleanup(); // Should be safe
    printf("  ✓ Global package system double cleanup test passed\n");
    
    printf("Global package system tests completed successfully!\n\n");
}

// Test package structure validation
static void test_package_structure_validation(void) {
    printf("Testing package structure validation...\n");
    
    setup_temp_dir();
    
    // Create a valid package structure
    char valid_package_dir[600];
    snprintf(valid_package_dir, sizeof(valid_package_dir), "%s/valid_package", temp_dir);
    ember_package_create_directory_recursive(valid_package_dir);
    
    char package_file[700];
    snprintf(package_file, sizeof(package_file), "%s/package.ember", valid_package_dir);
    FILE* f = fopen(package_file, "w");
    assert(f != NULL);
    fprintf(f, "# Valid package file\nprint(\"Hello from package\")\n");
    fclose(f);
    
    // Test valid package structure
    bool valid_result = ember_package_validate_structure(valid_package_dir);

    UNUSED(valid_result);

    UNUSED(valid_result);
    assert(valid_result == true);
    printf("  ✓ Valid package structure validation test passed\n");
    
    // Test invalid package structure (missing package.ember)
    char invalid_package_dir[600];
    snprintf(invalid_package_dir, sizeof(invalid_package_dir), "%s/invalid_package", temp_dir);
    ember_package_create_directory_recursive(invalid_package_dir);
    
    bool invalid_result = ember_package_validate_structure(invalid_package_dir);

    
    UNUSED(invalid_result);

    
    UNUSED(invalid_result);
    assert(invalid_result == false);
    printf("  ✓ Invalid package structure validation test passed\n");
    
    // Test manifest validation (stub)
    bool manifest_result = ember_package_validate_manifest("{\"name\": \"test\"}");

    UNUSED(manifest_result);

    UNUSED(manifest_result);
    assert(manifest_result == true); // Currently always returns true
    printf("  ✓ Package manifest validation test passed\n");
    
    // Test error cases
    assert(ember_package_validate_structure(NULL) == false);
    assert(ember_package_validate_structure("") == false);
    assert(ember_package_validate_structure("/nonexistent/path") == false);
    assert(ember_package_validate_manifest(NULL) == false);
    printf("  ✓ Package structure validation error handling test passed\n");
    
    printf("Package structure validation tests completed successfully!\n\n");
}

// Test repository functions (stubs)
static void test_repository_functions(void) {
    printf("Testing repository functions...\n");
    
    // Test fetch from repository (stub)
    bool fetch_result = ember_package_fetch_from_repository("test_package", "1.0.0", "https://example.com");

    UNUSED(fetch_result);

    UNUSED(fetch_result);
    assert(fetch_result == true); // Currently always returns true
    printf("  ✓ Package fetch from repository test passed\n");
    
    // Test publish to repository (stub)
    EmberPackage publish_package;
    memset(&publish_package, 0, sizeof(EmberPackage));
    strncpy(publish_package.name, "publish_test", EMBER_PACKAGE_MAX_NAME_LEN - 1);
    strncpy(publish_package.version, "1.0.0", EMBER_PACKAGE_MAX_VERSION_LEN - 1);
    
    bool publish_result = ember_package_publish_to_repository(&publish_package, "https://example.com");

    
    UNUSED(publish_result);

    
    UNUSED(publish_result);
    assert(publish_result == true); // Currently always returns true
    printf("  ✓ Package publish to repository test passed\n");
    
    // Test error cases
    assert(ember_package_fetch_from_repository(NULL, "1.0.0", "https://example.com") == false);
    assert(ember_package_fetch_from_repository("test", NULL, "https://example.com") == false);
    assert(ember_package_fetch_from_repository("test", "1.0.0", NULL) == false);
    assert(ember_package_publish_to_repository(NULL, "https://example.com") == false);
    assert(ember_package_publish_to_repository(&publish_package, NULL) == false);
    printf("  ✓ Repository functions error handling test passed\n");
    
    printf("Repository functions tests completed successfully!\n\n");
}

// Test import scanning and dependency installation
static void test_import_scanning(void) {
    printf("Testing import scanning and dependency installation...\n");
    
    setup_temp_dir();
    
    // Create a test script with import statements
    char script_file[600];
    snprintf(script_file, sizeof(script_file), "%s/test_script.ember", temp_dir);
    FILE* f = fopen(script_file, "w");
    assert(f != NULL);
    fprintf(f, "import math\n");
    fprintf(f, "import string@1.0.0\n");
    fprintf(f, "import json@latest # comment\n");
    fprintf(f, "# import comment_only\n");
    fprintf(f, "print(\"Hello, World!\")\n");
    fclose(f);
    
    // Test import scanning
    EmberProject* scan_project = ember_project_init("scan_test", "1.0.0");
    bool scan_result = ember_project_scan_imports(script_file, scan_project);

    UNUSED(scan_result);

    UNUSED(scan_result);
    assert(scan_result == true);
    assert(scan_project->dependency_count == 3);
    
    // Check discovered dependencies
    bool found_math = false, found_string = false, found_json = false;

    for (size_t i = 0; i < scan_project->dependency_count; i++) {
        if (strcmp(scan_project->dependencies[i].name, "math") == 0) {
            found_math = true;
            assert(strcmp(scan_project->dependencies[i].version, "latest") == 0);
        } else if (strcmp(scan_project->dependencies[i].name, "string") == 0) {
            found_string = true;
            assert(strcmp(scan_project->dependencies[i].version, "1.0.0") == 0);
        } else if (strcmp(scan_project->dependencies[i].name, "json") == 0) {
            found_json = true;
            assert(strcmp(scan_project->dependencies[i].version, "latest") == 0);
        }
    }
    assert(found_math && found_string && found_json);
    // Explicitly use variables to silence compiler warnings
    (void)found_math;
    (void)found_string;
    (void)found_json;
    printf("  ✓ Import scanning test passed\n");
    
    // Test dependency installation
    bool install_result = ember_project_install_dependencies(scan_project);

    UNUSED(install_result);

    UNUSED(install_result);
    assert(install_result == true);
    
    // Check that packages were loaded
    for (size_t i = 0; i < scan_project->dependency_count; i++) {
        assert(scan_project->dependencies[i].loaded == true);
    }
    printf("  ✓ Dependency installation test passed\n");
    
    // Test with empty project
    EmberProject* empty_project = ember_project_init("empty", "1.0.0");
    bool empty_install = ember_project_install_dependencies(empty_project);

    UNUSED(empty_install);

    UNUSED(empty_install);
    assert(empty_install == true); // Should succeed with no dependencies
    printf("  ✓ Empty project dependency installation test passed\n");
    
    // Test error cases
    assert(ember_project_scan_imports(NULL, scan_project) == false);
    assert(ember_project_scan_imports(script_file, NULL) == false);
    assert(ember_project_scan_imports("/nonexistent/file", scan_project) == false);
    assert(ember_project_install_dependencies(NULL) == true); // Returns true for NULL
    printf("  ✓ Import scanning error handling test passed\n");
    
    // Cleanup
    ember_package_system_cleanup();
    ember_project_cleanup(scan_project);
    ember_project_cleanup(empty_project);
    
    printf("Import scanning and dependency installation tests completed successfully!\n\n");
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember package management tests...\n\n");
    
    // Run all tests
    test_package_registry();
    test_package_discovery();
    test_package_validation();
    test_package_download_load();
    test_semantic_versioning();
    test_project_management();
    test_global_package_system();
    test_package_structure_validation();
    test_repository_functions();
    test_import_scanning();
    
    // Cleanup temporary directory
    cleanup_temp_dir();
    
    printf("All package management tests passed successfully!\n");
    return 0;
}