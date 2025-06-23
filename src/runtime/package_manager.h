/**
 * Ember Package Manager
 * Client-side package management for Ember applications
 */

#ifndef EMBER_PACKAGE_MANAGER_H
#define EMBER_PACKAGE_MANAGER_H

#include "../ember.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Package management configuration
#define EMBER_PKG_MAX_NAME_LEN 128
#define EMBER_PKG_MAX_VERSION_LEN 32
#define EMBER_PKG_MAX_DESCRIPTION_LEN 512
#define EMBER_PKG_MAX_DEPENDENCIES 64
#define EMBER_PKG_MAX_PACKAGES 256

// Package status
typedef enum {
    EMBER_PKG_STATUS_NOT_INSTALLED,
    EMBER_PKG_STATUS_INSTALLED,
    EMBER_PKG_STATUS_LOADING,
    EMBER_PKG_STATUS_LOADED,
    EMBER_PKG_STATUS_ERROR
} ember_package_status;

// Package dependency
typedef struct {
    char name[EMBER_PKG_MAX_NAME_LEN];
    char version[EMBER_PKG_MAX_VERSION_LEN];
    bool optional;              // Whether dependency is optional
} ember_package_dependency;

// Package metadata
typedef struct {
    char name[EMBER_PKG_MAX_NAME_LEN];
    char version[EMBER_PKG_MAX_VERSION_LEN];
    char description[EMBER_PKG_MAX_DESCRIPTION_LEN];
    char author[128];
    char license[64];
    char homepage[256];
    
    // Dependencies
    ember_package_dependency dependencies[EMBER_PKG_MAX_DEPENDENCIES];
    int dependency_count;
    
    // Installation info
    ember_package_status status;
    char install_path[512];
    time_t install_time;
    uint64_t package_size;
    
    // Registry info
    char registry_url[256];
    char checksum[128];
    
} ember_package_info;

// Package manager configuration
typedef struct {
    char registry_url[256];      // Default registry URL
    char cache_dir[512];        // Package cache directory
    char install_dir[512];      // Package installation directory
    bool auto_update;           // Automatic package updates
    bool verify_checksums;      // Verify package checksums
    int max_concurrent_downloads; // Maximum concurrent downloads
    int connection_timeout;     // HTTP connection timeout (seconds)
} ember_package_config;

// Package manager context
typedef struct {
    ember_vm* vm;               // Associated VM
    ember_package_config config; // Configuration
    
    // Installed packages
    ember_package_info packages[EMBER_PKG_MAX_PACKAGES];
    int package_count;
    
    // Download progress tracking
    struct {
        char current_package[EMBER_PKG_MAX_NAME_LEN];
        uint64_t bytes_downloaded;
        uint64_t total_bytes;
        int download_speed;     // KB/s
    } download_progress;
    
    // Error handling
    char last_error[512];
    
} ember_package_manager;

// Package manager API

// Lifecycle
ember_package_manager* ember_package_manager_create(ember_vm* vm);
void ember_package_manager_destroy(ember_package_manager* pm);

// Configuration
bool ember_package_manager_configure(ember_package_manager* pm, ember_package_config config);
bool ember_package_manager_load_config(ember_package_manager* pm, const char* config_file);
bool ember_package_manager_save_config(ember_package_manager* pm, const char* config_file);

// Registry operations
bool ember_package_manager_set_registry(ember_package_manager* pm, const char* registry_url);
bool ember_package_manager_update_registry_cache(ember_package_manager* pm);
char** ember_package_manager_search_packages(ember_package_manager* pm, const char* query, int* result_count);

// Package operations
bool ember_package_manager_install_package(ember_package_manager* pm, const char* package_name, const char* version);
bool ember_package_manager_uninstall_package(ember_package_manager* pm, const char* package_name);
bool ember_package_manager_update_package(ember_package_manager* pm, const char* package_name);
bool ember_package_manager_update_all_packages(ember_package_manager* pm);

// Package information
ember_package_info* ember_package_manager_get_package_info(ember_package_manager* pm, const char* package_name);
bool ember_package_manager_is_package_installed(ember_package_manager* pm, const char* package_name);
char** ember_package_manager_list_installed_packages(ember_package_manager* pm, int* package_count);

// Dependency management
bool ember_package_manager_resolve_dependencies(ember_package_manager* pm, const char* package_name);
bool ember_package_manager_install_dependencies(ember_package_manager* pm, ember_package_info* package);
char** ember_package_manager_get_dependency_tree(ember_package_manager* pm, const char* package_name, int* dependency_count);

// Package loading and execution
bool ember_package_manager_load_package(ember_package_manager* pm, const char* package_name);
bool ember_package_manager_import_package(ember_package_manager* pm, const char* package_name, const char* alias);
ember_value ember_package_manager_call_package_function(ember_package_manager* pm, const char* package_name, const char* function_name, int argc, ember_value* argv);

// Cache management
bool ember_package_manager_clear_cache(ember_package_manager* pm);
bool ember_package_manager_clean_cache(ember_package_manager* pm); // Remove unused packages
uint64_t ember_package_manager_get_cache_size(ember_package_manager* pm);

// Error handling and logging
const char* ember_package_manager_get_last_error(ember_package_manager* pm);
void ember_package_manager_set_verbose(ember_package_manager* pm, bool verbose);

// Progress callbacks
typedef void (*ember_package_progress_callback)(const char* package_name, uint64_t bytes_downloaded, uint64_t total_bytes, void* user_data);
void ember_package_manager_set_progress_callback(ember_package_manager* pm, ember_package_progress_callback callback, void* user_data);

// Ember native functions for package management

// Package management functions callable from Ember scripts
ember_value ember_native_package_install(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_package_uninstall(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_package_list(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_package_info(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_package_search(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_package_update(ember_vm* vm, int argc, ember_value* argv);

// Import function for loading packages
ember_value ember_native_import(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_require(ember_vm* vm, int argc, ember_value* argv);

// Registry functions
ember_value ember_native_registry_set_url(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_registry_update(ember_vm* vm, int argc, ember_value* argv);

// Utility functions
bool ember_package_manager_validate_package_name(const char* name);
bool ember_package_manager_validate_version(const char* version);
int ember_package_manager_compare_versions(const char* version1, const char* version2);
char* ember_package_manager_format_package_size(uint64_t bytes);

// Package manager registration
void ember_package_manager_register_natives(ember_vm* vm);

// Configuration constants
extern const char* EMBER_PACKAGE_DEFAULT_REGISTRY;
extern const char* EMBER_PACKAGE_DEFAULT_CACHE_DIR;
extern const char* EMBER_PACKAGE_DEFAULT_INSTALL_DIR;
extern const char* EMBER_PACKAGE_CONFIG_FILE;

#ifdef __cplusplus
}
#endif

#endif // EMBER_PACKAGE_MANAGER_H