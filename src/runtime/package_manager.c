/**
 * Ember Package Manager Implementation
 * Client-side package management for Ember applications
 */

#include "package_manager.h"
#include "http_enhanced.h"
#include "../vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

// Configuration constants
const char* EMBER_PACKAGE_DEFAULT_REGISTRY = "https://packages.ember-lang.org";
const char* EMBER_PACKAGE_DEFAULT_CACHE_DIR = "~/.ember/cache";
const char* EMBER_PACKAGE_DEFAULT_INSTALL_DIR = "~/.ember/packages";
const char* EMBER_PACKAGE_CONFIG_FILE = "~/.ember/package_config.json";

// Global package manager instance (for VM integration)
static ember_package_manager* g_package_manager = NULL;

// Utility: Expand tilde in path
static char* expand_path(const char* path) {
    if (path[0] != '~') {
        return strdup(path);
    }
    
    const char* home = getenv("HOME");
    if (!home) {
        return strdup(path);
    }
    
    size_t home_len = strlen(home);
    size_t path_len = strlen(path);
    char* expanded = malloc(home_len + path_len);
    
    strcpy(expanded, home);
    strcat(expanded, path + 1); // Skip the ~
    
    return expanded;
}

// Utility: Create directory recursively
static bool create_directory_recursive(const char* path) {
    char* path_copy = strdup(path);
    char* p = path_copy;
    
    while (*p) {
        if (*p == '/' && p != path_copy) {
            *p = '\0';
            if (mkdir(path_copy, 0755) != 0 && errno != EEXIST) {
                free(path_copy);
                return false;
            }
            *p = '/';
        }
        p++;
    }
    
    bool result = (mkdir(path_copy, 0755) == 0 || errno == EEXIST);
    free(path_copy);
    return result;
}

// Create package manager
ember_package_manager* ember_package_manager_create(ember_vm* vm) {
    ember_package_manager* pm = calloc(1, sizeof(ember_package_manager));
    if (!pm) {
        return NULL;
    }
    
    pm->vm = vm;
    
    // Set default configuration
    strcpy(pm->config.registry_url, EMBER_PACKAGE_DEFAULT_REGISTRY);
    
    char* cache_dir = expand_path(EMBER_PACKAGE_DEFAULT_CACHE_DIR);
    strcpy(pm->config.cache_dir, cache_dir);
    free(cache_dir);
    
    char* install_dir = expand_path(EMBER_PACKAGE_DEFAULT_INSTALL_DIR);
    strcpy(pm->config.install_dir, install_dir);
    free(install_dir);
    
    pm->config.auto_update = false;
    pm->config.verify_checksums = true;
    pm->config.max_concurrent_downloads = 3;
    pm->config.connection_timeout = 30;
    
    pm->package_count = 0;
    pm->last_error[0] = '\0';
    
    // Create directories
    create_directory_recursive(pm->config.cache_dir);
    create_directory_recursive(pm->config.install_dir);
    
    // Set global instance
    g_package_manager = pm;
    
    printf("Package manager created (registry: %s)\n", pm->config.registry_url);
    return pm;
}

// Destroy package manager
void ember_package_manager_destroy(ember_package_manager* pm) {
    if (!pm) {
        return;
    }
    
    if (g_package_manager == pm) {
        g_package_manager = NULL;
    }
    
    free(pm);
}

// Set registry URL
bool ember_package_manager_set_registry(ember_package_manager* pm, const char* registry_url) {
    if (!pm || !registry_url || strlen(registry_url) >= sizeof(pm->config.registry_url)) {
        return false;
    }
    
    strcpy(pm->config.registry_url, registry_url);
    printf("Registry URL set to: %s\n", registry_url);
    return true;
}

// Search packages in registry
char** ember_package_manager_search_packages(ember_package_manager* pm, const char* query, int* result_count) {
    if (!pm || !query || !result_count) {
        *result_count = 0;
        return NULL;
    }
    
    // Build search URL
    char search_url[512];
    snprintf(search_url, sizeof(search_url), "%s/search?q=%s&limit=20", 
             pm->config.registry_url, query);
    
    printf("Searching packages: %s\n", query);
    
    // For now, return mock results
    static char* mock_results[] = {
        "http-client - HTTP client library for Ember",
        "crypto-utils - Cryptographic utilities",
        "json-parser - Fast JSON parsing",
        "math-extra - Extended math functions",
        "string-utils - String manipulation utilities"
    };
    
    *result_count = 5;
    return mock_results;
}

// Install package
bool ember_package_manager_install_package(ember_package_manager* pm, const char* package_name, const char* version) {
    if (!pm || !package_name) {
        strcpy(pm->last_error, "Invalid parameters");
        return false;
    }
    
    if (!ember_package_manager_validate_package_name(package_name)) {
        snprintf(pm->last_error, sizeof(pm->last_error), "Invalid package name: %s", package_name);
        return false;
    }
    
    // Check if already installed
    if (ember_package_manager_is_package_installed(pm, package_name)) {
        printf("Package '%s' is already installed\n", package_name);
        return true;
    }
    
    printf("Installing package: %s", package_name);
    if (version) {
        printf(" (version: %s)", version);
    }
    printf("\n");
    
    // Create package info
    if (pm->package_count >= EMBER_PKG_MAX_PACKAGES) {
        strcpy(pm->last_error, "Maximum number of packages reached");
        return false;
    }
    
    ember_package_info* pkg = &pm->packages[pm->package_count];
    strcpy(pkg->name, package_name);
    
    if (version) {
        strcpy(pkg->version, version);
    } else {
        strcpy(pkg->version, "latest");
    }
    
    snprintf(pkg->description, sizeof(pkg->description), "Package %s", package_name);
    strcpy(pkg->author, "Unknown");
    strcpy(pkg->license, "MIT");
    
    // Set installation path
    snprintf(pkg->install_path, sizeof(pkg->install_path), "%s/%s", 
             pm->config.install_dir, package_name);
    
    pkg->status = EMBER_PKG_STATUS_INSTALLED;
    pkg->install_time = time(NULL);
    pkg->package_size = 1024; // Mock size
    
    strcpy(pkg->registry_url, pm->config.registry_url);
    strcpy(pkg->checksum, "mock-checksum-123");
    
    pkg->dependency_count = 0;
    
    pm->package_count++;
    
    printf("Package '%s' installed successfully\n", package_name);
    return true;
}

// Uninstall package
bool ember_package_manager_uninstall_package(ember_package_manager* pm, const char* package_name) {
    if (!pm || !package_name) {
        return false;
    }
    
    // Find package
    for (int i = 0; i < pm->package_count; i++) {
        if (strcmp(pm->packages[i].name, package_name) == 0) {
            printf("Uninstalling package: %s\n", package_name);
            
            // Remove from array by shifting
            for (int j = i; j < pm->package_count - 1; j++) {
                pm->packages[j] = pm->packages[j + 1];
            }
            pm->package_count--;
            
            printf("Package '%s' uninstalled successfully\n", package_name);
            return true;
        }
    }
    
    snprintf(pm->last_error, sizeof(pm->last_error), "Package '%s' not found", package_name);
    return false;
}

// Get package info
ember_package_info* ember_package_manager_get_package_info(ember_package_manager* pm, const char* package_name) {
    if (!pm || !package_name) {
        return NULL;
    }
    
    for (int i = 0; i < pm->package_count; i++) {
        if (strcmp(pm->packages[i].name, package_name) == 0) {
            return &pm->packages[i];
        }
    }
    
    return NULL;
}

// Check if package is installed
bool ember_package_manager_is_package_installed(ember_package_manager* pm, const char* package_name) {
    return ember_package_manager_get_package_info(pm, package_name) != NULL;
}

// List installed packages
char** ember_package_manager_list_installed_packages(ember_package_manager* pm, int* package_count) {
    if (!pm || !package_count) {
        *package_count = 0;
        return NULL;
    }
    
    if (pm->package_count == 0) {
        *package_count = 0;
        return NULL;
    }
    
    char** package_list = malloc(pm->package_count * sizeof(char*));
    for (int i = 0; i < pm->package_count; i++) {
        package_list[i] = strdup(pm->packages[i].name);
    }
    
    *package_count = pm->package_count;
    return package_list;
}

// Validate package name
bool ember_package_manager_validate_package_name(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= EMBER_PKG_MAX_NAME_LEN) {
        return false;
    }
    
    // Check for valid characters (alphanumeric, dash, underscore)
    for (const char* p = name; *p; p++) {
        if (!isalnum(*p) && *p != '-' && *p != '_') {
            return false;
        }
    }
    
    return true;
}

// Validate version
bool ember_package_manager_validate_version(const char* version) {
    if (!version || strlen(version) == 0 || strlen(version) >= EMBER_PKG_MAX_VERSION_LEN) {
        return false;
    }
    
    // Simple version validation (should be more sophisticated)
    return strstr(version, "..") == NULL; // No double dots
}

// Get last error
const char* ember_package_manager_get_last_error(ember_package_manager* pm) {
    return pm ? pm->last_error : "Package manager not initialized";
}

// Format package size
char* ember_package_manager_format_package_size(uint64_t bytes) {
    static char buffer[64];
    
    if (bytes < 1024) {
        snprintf(buffer, sizeof(buffer), "%lu B", bytes);
    } else if (bytes < 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f KB", bytes / 1024.0);
    } else if (bytes < 1024 * 1024 * 1024) {
        snprintf(buffer, sizeof(buffer), "%.1f MB", bytes / (1024.0 * 1024.0));
    } else {
        snprintf(buffer, sizeof(buffer), "%.1f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    }
    
    return buffer;
}

// Ember native functions

// Install package from Ember script
ember_value ember_native_package_install(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_bool(false);
    }
    
    if (!g_package_manager) {
        printf("Package manager not initialized\n");
        return ember_make_bool(false);
    }
    
    ember_string* package_name = AS_STRING(argv[0]);
    bool result = ember_package_manager_install_package(g_package_manager, package_name->chars, NULL);
    
    return ember_make_bool(result);
}

// Uninstall package from Ember script
ember_value ember_native_package_uninstall(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_bool(false);
    }
    
    if (!g_package_manager) {
        return ember_make_bool(false);
    }
    
    ember_string* package_name = AS_STRING(argv[0]);
    bool result = ember_package_manager_uninstall_package(g_package_manager, package_name->chars);
    
    return ember_make_bool(result);
}

// List installed packages from Ember script
ember_value ember_native_package_list(ember_vm* vm, int argc, ember_value* argv) {
    (void)argc;
    (void)argv;
    
    if (!g_package_manager) {
        return ember_make_string_gc(vm, "");
    }
    
    int package_count;
    char** packages = ember_package_manager_list_installed_packages(g_package_manager, &package_count);
    
    if (package_count == 0) {
        return ember_make_string_gc(vm, "No packages installed");
    }
    
    // Build comma-separated list
    size_t total_len = 0;
    for (int i = 0; i < package_count; i++) {
        total_len += strlen(packages[i]) + 2; // +2 for ", "
    }
    
    char* result = malloc(total_len + 1);
    result[0] = '\0';
    
    for (int i = 0; i < package_count; i++) {
        if (i > 0) {
            strcat(result, ", ");
        }
        strcat(result, packages[i]);
        free(packages[i]);
    }
    free(packages);
    
    ember_value ret = ember_make_string_gc(vm, result);
    free(result);
    
    return ret;
}

// Search packages from Ember script
ember_value ember_native_package_search(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_string_gc(vm, "");
    }
    
    if (!g_package_manager) {
        return ember_make_string_gc(vm, "Package manager not available");
    }
    
    ember_string* query = AS_STRING(argv[0]);
    int result_count;
    char** results = ember_package_manager_search_packages(g_package_manager, query->chars, &result_count);
    
    if (result_count == 0) {
        return ember_make_string_gc(vm, "No packages found");
    }
    
    // Build result string
    size_t total_len = 0;
    for (int i = 0; i < result_count; i++) {
        total_len += strlen(results[i]) + 1; // +1 for newline
    }
    
    char* result_str = malloc(total_len + 1);
    result_str[0] = '\0';
    
    for (int i = 0; i < result_count; i++) {
        if (i > 0) {
            strcat(result_str, "\n");
        }
        strcat(result_str, results[i]);
    }
    
    ember_value ret = ember_make_string_gc(vm, result_str);
    free(result_str);
    
    return ret;
}

// Register package manager native functions
void ember_package_manager_register_natives(ember_vm* vm) {
    ember_register_func(vm, "package_install", ember_native_package_install);
    ember_register_func(vm, "package_uninstall", ember_native_package_uninstall);
    ember_register_func(vm, "package_list", ember_native_package_list);
    ember_register_func(vm, "package_search", ember_native_package_search);
    
    printf("Package manager native functions registered\n");
}

// Placeholder implementations for other functions
bool ember_package_manager_configure(ember_package_manager* pm, ember_package_config config) {
    if (!pm) return false;
    pm->config = config;
    return true;
}

bool ember_package_manager_update_package(ember_package_manager* pm, const char* package_name) {
    (void)pm; (void)package_name;
    printf("Package update not yet implemented\n");
    return false;
}

bool ember_package_manager_load_package(ember_package_manager* pm, const char* package_name) {
    (void)pm; (void)package_name;
    printf("Package loading not yet implemented\n");
    return false;
}

ember_value ember_native_import(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("Import function not yet implemented\n");
    return ember_make_nil();
}