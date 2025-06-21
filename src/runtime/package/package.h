/**
 * Ember Package Management System
 * Container-native package management with cryptographic security
 */

#ifndef EMBER_PACKAGE_H
#define EMBER_PACKAGE_H

#include "../value/value.h"
#include <stdbool.h>
#include <stdint.h>

#define EMBER_PACKAGE_MAX_NAME_LEN 128
#define EMBER_PACKAGE_MAX_VERSION_LEN 64
#define EMBER_PACKAGE_MAX_PATH_LEN 512
#define EMBER_PACKAGE_SIGNATURE_LEN 64

// Package management structures
typedef struct {
    char name[EMBER_PACKAGE_MAX_NAME_LEN];
    char version[EMBER_PACKAGE_MAX_VERSION_LEN];
    char local_path[EMBER_PACKAGE_MAX_PATH_LEN];
    char repository_url[EMBER_PACKAGE_MAX_PATH_LEN];
    bool verified;
    bool loaded;
    void* handle;  // Module handle
} EmberPackage;

// Project control file structure (ember.toml equivalent)
typedef struct {
    char name[EMBER_PACKAGE_MAX_NAME_LEN];
    char version[EMBER_PACKAGE_MAX_VERSION_LEN];
    char description[256];
    char author[128];
    EmberPackage* dependencies;
    size_t dependency_count;
    size_t dependency_capacity;
} EmberProject;

typedef struct {
    EmberPackage* packages;
    size_t count;
    size_t capacity;
} EmberPackageRegistry;

// Package operations
bool ember_package_discover(const char* package_name, EmberPackage* package);
bool ember_package_download(EmberPackage* package);
bool ember_package_load(EmberPackage* package);
bool ember_package_unload(EmberPackage* package);

// Module system integration
bool ember_package_register_functions(EmberPackage* package, ember_vm* target_vm);
bool ember_package_import_into_vm(EmberPackage* package, ember_vm* target_vm);
bool ember_package_get_exports(EmberPackage* package, char** function_names, int* count, int max_functions);

// Registry management
EmberPackageRegistry* ember_package_registry_init(void);
void ember_package_registry_cleanup(EmberPackageRegistry* registry);
bool ember_package_registry_add(EmberPackageRegistry* registry, const EmberPackage* package);
EmberPackage* ember_package_registry_find(EmberPackageRegistry* registry, const char* name);

// Semantic versioning
bool ember_package_version_satisfies(const char* version, const char* constraint);
int ember_package_version_compare(const char* version1, const char* version2);

// Package repository integration (HTTP-based)
bool ember_package_fetch_from_repository(const char* package_name, const char* version, const char* repo_url);
bool ember_package_publish_to_repository(const EmberPackage* package, const char* repo_url);

// Package validation
bool ember_package_validate_structure(const char* package_path);
bool ember_package_validate_manifest(const char* manifest_json);

// Global package system management
bool ember_package_system_init(void);
void ember_package_system_cleanup(void);
EmberPackageRegistry* ember_package_get_global_registry(void);

// Project control file management (ember.toml)
EmberProject* ember_project_init(const char* name, const char* version);
void ember_project_cleanup(EmberProject* project);
bool ember_project_add_dependency(EmberProject* project, const char* name, const char* version);
bool ember_project_load_from_file(const char* filepath, EmberProject** project);
bool ember_project_save_to_file(const EmberProject* project, const char* filepath);
bool ember_project_generate_default(const char* directory);

// Automatic dependency detection
bool ember_project_scan_imports(const char* script_path, EmberProject* project);
bool ember_project_install_dependencies(EmberProject* project);

// Security functions
int ember_package_validate_name(const char* package_name);
int ember_package_create_directory_recursive(const char* path);

#endif // EMBER_PACKAGE_H