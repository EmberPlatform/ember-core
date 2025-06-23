/**
 * Enhanced Module System for Ember
 * Supports ES6-style imports, CommonJS-style requires, and dynamic imports
 */

#define _GNU_SOURCE

#include "../vm.h"
#include "value/value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Module cache to prevent multiple loads
typedef struct module_cache_entry {
    char* module_path;
    ember_value module_exports;
    bool is_loaded;
    struct module_cache_entry* next;
} module_cache_entry;

static module_cache_entry* g_module_cache = NULL;

// Standard library modules
typedef struct {
    const char* name;
    ember_value (*initializer)(ember_vm* vm);
} stdlib_module;

// Forward declarations
static ember_value init_math_module(ember_vm* vm);
static ember_value init_string_module(ember_vm* vm);
static ember_value init_crypto_module(ember_vm* vm);
static ember_value init_json_module(ember_vm* vm);
static ember_value init_io_module(ember_vm* vm);
static ember_value init_http_module(ember_vm* vm);

// Standard library module registry
static stdlib_module stdlib_modules[] = {
    {"math", init_math_module},
    {"string", init_string_module},
    {"crypto", init_crypto_module},
    {"json", init_json_module},
    {"io", init_io_module},
    {"http", init_http_module},
    {NULL, NULL}
};

// Path resolution for modules
static char* resolve_module_path(const char* module_name, const char* current_file) {
    char* resolved_path = malloc(512);
    
    // Check if it's a relative path
    if (module_name[0] == '.' && module_name[1] == '/') {
        // Relative to current file
        if (current_file) {
            // Get directory of current file
            char* dir = strrchr(current_file, '/');
            if (dir) {
                size_t dir_len = dir - current_file + 1;
                strncpy(resolved_path, current_file, dir_len);
                resolved_path[dir_len] = '\0';
                strcat(resolved_path, module_name + 2); // Skip "./"
            } else {
                strcpy(resolved_path, module_name + 2);
            }
        } else {
            strcpy(resolved_path, module_name + 2);
        }
    } else if (module_name[0] == '/') {
        // Absolute path
        strcpy(resolved_path, module_name);
    } else {
        // Standard library or installed package
        // Try different locations
        char candidates[4][512];
        snprintf(candidates[0], 512, "./%s.ember", module_name);
        snprintf(candidates[1], 512, "./lib/%s.ember", module_name);
        snprintf(candidates[2], 512, "/usr/lib/ember/%s.ember", module_name);
        snprintf(candidates[3], 512, "%s/.local/lib/ember/%s.ember", getenv("HOME") ?: ".", module_name);
        
        for (int i = 0; i < 4; i++) {
            struct stat st;
            if (stat(candidates[i], &st) == 0) {
                strcpy(resolved_path, candidates[i]);
                return resolved_path;
            }
        }
        
        // Default to current directory
        snprintf(resolved_path, 512, "./%s.ember", module_name);
    }
    
    // Add .ember extension if not present
    if (!strstr(resolved_path, ".ember")) {
        strcat(resolved_path, ".ember");
    }
    
    return resolved_path;
}

// Check module cache
static ember_value get_cached_module(const char* module_path) {
    module_cache_entry* entry = g_module_cache;
    while (entry) {
        if (strcmp(entry->module_path, module_path) == 0 && entry->is_loaded) {
            return entry->module_exports;
        }
        entry = entry->next;
    }
    return ember_make_nil();
}

// Cache module exports
static void cache_module(const char* module_path, ember_value exports) {
    // Check if already cached
    module_cache_entry* entry = g_module_cache;
    while (entry) {
        if (strcmp(entry->module_path, module_path) == 0) {
            entry->module_exports = exports;
            entry->is_loaded = true;
            return;
        }
        entry = entry->next;
    }
    
    // Add new entry
    entry = malloc(sizeof(module_cache_entry));
    entry->module_path = strdup(module_path);
    entry->module_exports = exports;
    entry->is_loaded = true;
    entry->next = g_module_cache;
    g_module_cache = entry;
}

// Load file module
static ember_value load_file_module(ember_vm* vm, const char* file_path) {
    // Check if file exists
    struct stat st;
    if (stat(file_path, &st) != 0) {
        printf("Module not found: %s\n", file_path);
        return ember_make_nil();
    }
    
    // Read file content
    FILE* file = fopen(file_path, "r");
    if (!file) {
        printf("Cannot open module: %s\n", file_path);
        return ember_make_nil();
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* source = malloc(file_size + 1);
    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);
    
    // Create module object (hash map for exports)
    ember_value module_exports = ember_make_hash_map(vm, 16);
    
    // Set up module context
    // TODO: Execute the module source in a separate context
    // For now, return empty exports
    printf("[MODULE] Loaded %s\n", file_path);
    
    free(source);
    return module_exports;
}

// Load standard library module
static ember_value load_stdlib_module(ember_vm* vm, const char* module_name) {
    for (int i = 0; stdlib_modules[i].name; i++) {
        if (strcmp(stdlib_modules[i].name, module_name) == 0) {
            return stdlib_modules[i].initializer(vm);
        }
    }
    return ember_make_nil();
}

// Main module loading function
ember_value ember_load_module(ember_vm* vm, const char* module_name, const char* current_file) {
    // First, check standard library
    ember_value stdlib_module = load_stdlib_module(vm, module_name);
    if (stdlib_module.type != EMBER_VAL_NIL) {
        return stdlib_module;
    }
    
    // Resolve file path
    char* module_path = resolve_module_path(module_name, current_file);
    
    // Check cache
    ember_value cached = get_cached_module(module_path);
    if (cached.type != EMBER_VAL_NIL) {
        free(module_path);
        return cached;
    }
    
    // Load from file
    ember_value module_exports = load_file_module(vm, module_path);
    
    // Cache the result
    if (module_exports.type != EMBER_VAL_NIL) {
        cache_module(module_path, module_exports);
    }
    
    free(module_path);
    return module_exports;
}

// Native function: import(module_name)
ember_value ember_native_import(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* module_name = AS_STRING(argv[0]);
    return ember_load_module(vm, module_name->chars, NULL);
}

// Native function: require(module_name) - CommonJS style
ember_value ember_native_require(ember_vm* vm, int argc, ember_value* argv) {
    return ember_native_import(vm, argc, argv);
}

// Standard library module initializers

static ember_value init_math_module(ember_vm* vm) {
    ember_value exports = ember_make_hash_map(vm, 16);
    ember_hash_map* map = AS_HASH_MAP(exports);
    
    // Add math constants
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "PI"), ember_make_number(3.14159265359));
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "E"), ember_make_number(2.71828182846));
    
    // For now, create simple message indicating module is loaded
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "loaded"), ember_make_bool(true));
    
    return exports;
}

static ember_value init_string_module(ember_vm* vm) {
    ember_value exports = ember_make_hash_map(vm, 16);
    ember_hash_map* map = AS_HASH_MAP(exports);
    
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "loaded"), ember_make_bool(true));
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "type"), ember_make_string_gc(vm, "string"));
    
    return exports;
}

static ember_value init_crypto_module(ember_vm* vm) {
    ember_value exports = ember_make_hash_map(vm, 16);
    ember_hash_map* map = AS_HASH_MAP(exports);
    
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "loaded"), ember_make_bool(true));
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "type"), ember_make_string_gc(vm, "crypto"));
    
    return exports;
}

static ember_value init_json_module(ember_vm* vm) {
    ember_value exports = ember_make_hash_map(vm, 16);
    ember_hash_map* map = AS_HASH_MAP(exports);
    
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "loaded"), ember_make_bool(true));
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "type"), ember_make_string_gc(vm, "json"));
    
    return exports;
}

static ember_value init_io_module(ember_vm* vm) {
    ember_value exports = ember_make_hash_map(vm, 16);
    ember_hash_map* map = AS_HASH_MAP(exports);
    
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "loaded"), ember_make_bool(true));
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "type"), ember_make_string_gc(vm, "io"));
    
    return exports;
}

static ember_value init_http_module(ember_vm* vm) {
    ember_value exports = ember_make_hash_map(vm, 16);
    ember_hash_map* map = AS_HASH_MAP(exports);
    
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "loaded"), ember_make_bool(true));
    hash_map_set_with_vm(vm, map, ember_make_string_gc(vm, "type"), ember_make_string_gc(vm, "http"));
    
    return exports;
}

// Register module system native functions
void ember_module_system_register_natives(ember_vm* vm) {
    ember_register_func(vm, "import", ember_native_import);
    ember_register_func(vm, "require", ember_native_require);
}

// Clean up module cache
void ember_module_system_cleanup(void) {
    module_cache_entry* entry = g_module_cache;
    while (entry) {
        module_cache_entry* next = entry->next;
        free(entry->module_path);
        free(entry);
        entry = next;
    }
    g_module_cache = NULL;
}