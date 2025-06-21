#define _GNU_SOURCE
#include "ember.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stddef.h>

// Initialize VFS with default mount (current working directory to /app)
void ember_vfs_init(ember_vm* vm) {
    if (!vm) return;
    
    // Initialize VFS
    vm->mounts = NULL;
    vm->mount_count = 0;
    vm->mount_capacity = 0;
    
    // Add default mount: current working directory -> /app
    char* cwd = getcwd(NULL, 0);
    if (cwd) {
        ember_vfs_mount(vm, "/app", cwd, EMBER_MOUNT_RW);
        free(cwd);
    }
    
    // Add /tmp mount from environment or system default
    const char* tmp_path = getenv("TMPDIR");
    if (!tmp_path) tmp_path = "/tmp";
    ember_vfs_mount(vm, "/tmp", tmp_path, EMBER_MOUNT_RW);
    
    // Process EMBER_MOUNTS environment variable
    const char* env_mounts = getenv("EMBER_MOUNTS");
    if (env_mounts) {
        size_t env_len = strlen(env_mounts);
        char* mounts_copy = malloc(env_len + 1);
        if (!mounts_copy) {
            fprintf(stderr, "[SECURITY] Memory allocation failed for EMBER_MOUNTS processing\n");
            return;
        }
        memcpy(mounts_copy, env_mounts, env_len + 1);
        
        char* mount = strtok(mounts_copy, ",");
        while (mount != NULL) {
            // Parse mount format: "/virtual:/host" or "/virtual:/host:ro"
            char* colon1 = strchr(mount, ':');
            if (colon1) {
                *colon1 = '\0';
                char* virtual_path = mount;
                char* host_part = colon1 + 1;
                
                char* colon2 = strchr(host_part, ':');
                int flags = EMBER_MOUNT_RW; // default
                if (colon2) {
                    *colon2 = '\0';
                    if (strcmp(colon2 + 1, "ro") == 0) {
                        flags = EMBER_MOUNT_RO;
                    }
                }
                
                ember_vfs_mount(vm, virtual_path, host_part, flags);
            }
            mount = strtok(NULL, ",");
        }
        
        free(mounts_copy);
    }
}

// Mount a host path to a virtual path
int ember_vfs_mount(ember_vm* vm, const char* virtual_path, const char* host_path, int flags) {
    if (!vm || !virtual_path || !host_path) return -1;
    
    // Validate that host path exists
    struct stat st;
    if (stat(host_path, &st) != 0) {
        return -1; // Host path doesn't exist
    }
    
    // Resolve host path to absolute path
    char resolved_host[PATH_MAX];
    if (realpath(host_path, resolved_host) == NULL) {
        return -1; // Cannot resolve host path
    }
    
    // Ensure virtual path starts with /
    if (virtual_path[0] != '/') {
        return -1; // Virtual paths must be absolute
    }
    
    // Check if we need to expand mount array
    if (vm->mount_count >= vm->mount_capacity) {
        int new_capacity = vm->mount_capacity == 0 ? 8 : vm->mount_capacity * 2;
        if (new_capacity > EMBER_MAX_MOUNTS) {
            return -1; // Too many mounts
        }
        
        ember_mount_point* new_mounts = realloc(vm->mounts, 
                                               new_capacity * sizeof(ember_mount_point));
        if (!new_mounts) return -1;
        
        vm->mounts = new_mounts;
        vm->mount_capacity = new_capacity;
    }
    
    // Check if virtual path already mounted (replace it)
    for (int i = 0; i < vm->mount_count; i++) {
        if (strcmp(vm->mounts[i].virtual_path, virtual_path) == 0) {
            size_t vpath_len = strlen(virtual_path);
            size_t hpath_len = strlen(resolved_host);
            
            char* new_vpath = malloc(vpath_len + 1);
            char* new_hpath = malloc(hpath_len + 1);
            
            if (!new_vpath || !new_hpath) {
                fprintf(stderr, "[SECURITY] Memory allocation failed for mount replacement\n");
                free(new_vpath);
                free(new_hpath);
                return -1;
            }
            
            free(vm->mounts[i].virtual_path);
            free(vm->mounts[i].host_path);
            
            memcpy(new_vpath, virtual_path, vpath_len + 1);
            memcpy(new_hpath, resolved_host, hpath_len + 1);
            
            vm->mounts[i].virtual_path = new_vpath;
            vm->mounts[i].host_path = new_hpath;
            vm->mounts[i].flags = flags;
            return 0;
        }
    }
    
    // Add new mount
    ember_mount_point* mount = &vm->mounts[vm->mount_count];
    
    size_t vpath_len = strlen(virtual_path);
    size_t hpath_len = strlen(resolved_host);
    
    mount->virtual_path = malloc(vpath_len + 1);
    mount->host_path = malloc(hpath_len + 1);
    
    if (!mount->virtual_path || !mount->host_path) {
        fprintf(stderr, "[SECURITY] Memory allocation failed for new mount\n");
        free(mount->virtual_path);
        free(mount->host_path);
        mount->virtual_path = NULL;
        mount->host_path = NULL;
        return -1;
    }
    
    memcpy(mount->virtual_path, virtual_path, vpath_len + 1);
    memcpy(mount->host_path, resolved_host, hpath_len + 1);
    mount->flags = flags;
    vm->mount_count++;
    
    return 0;
}

// Unmount a virtual path
int ember_vfs_unmount(ember_vm* vm, const char* virtual_path) {
    if (!vm || !virtual_path) return -1;
    
    for (int i = 0; i < vm->mount_count; i++) {
        if (strcmp(vm->mounts[i].virtual_path, virtual_path) == 0) {
            free(vm->mounts[i].virtual_path);
            free(vm->mounts[i].host_path);
            
            // Move last mount to this position
            if (i < vm->mount_count - 1) {
                vm->mounts[i] = vm->mounts[vm->mount_count - 1];
            }
            vm->mount_count--;
            return 0;
        }
    }
    return -1; // Mount not found
}

// Validate that a path component doesn't contain dangerous patterns  
static int is_safe_path_component(const char* component, size_t len) {
    // Check for empty component or excessive length
    if (len == 0 || len > PATH_MAX) return 0;
    
    // Prevent integer overflow in component access
    if (component == NULL) return 0;
    
    // Check for . or ..
    if (len == 1 && component[0] == '.') return 0;
    if (len == 2 && component[0] == '.' && component[1] == '.') return 0;
    
    // Check for null bytes and other dangerous characters
    for (size_t i = 0; i < len; i++) {
        char c = component[i];
        if (c == '\0' || c == '\n' || c == '\r') return 0;
        // Block control characters that could be used in attacks
        if ((unsigned char)c < 32 && c != '\t') return 0;
    }
    
    return 1;
}

// Resolve a virtual path to a host path
char* ember_vfs_resolve(ember_vm* vm, const char* virtual_path) {
    if (!vm || !virtual_path) return NULL;
    
    // Validate input path length to prevent buffer overflows
    size_t vpath_len = strlen(virtual_path);
    if (vpath_len == 0 || vpath_len >= PATH_MAX) {
        fprintf(stderr, "[SECURITY] Invalid path length: %zu\n", vpath_len);
        return NULL;
    }
    
    // Virtual paths must be absolute
    if (virtual_path[0] != '/') return NULL;
    
    // Validate path components to prevent traversal
    const char* component = virtual_path + 1;  // Skip initial /
    const char* next_slash;
    
    while (*component) {
        next_slash = strchr(component, '/');
        
        // Prevent integer overflow in pointer arithmetic
        if (next_slash && next_slash < component) {
            fprintf(stderr, "[SECURITY] Invalid pointer arithmetic detected\n");
            return NULL;
        }
        
        size_t component_len;
        if (next_slash) {
            // Safe pointer subtraction with overflow check
            ptrdiff_t diff = next_slash - component;
            if (diff < 0 || diff > PATH_MAX) {
                fprintf(stderr, "[SECURITY] Component length overflow detected\n");
                return NULL;
            }
            component_len = (size_t)diff;
        } else {
            component_len = strlen(component);
            if (component_len > PATH_MAX) {
                fprintf(stderr, "[SECURITY] Component too long: %zu\n", component_len);
                return NULL;
            }
        }
        
        if (!is_safe_path_component(component, component_len)) {
            fprintf(stderr, "[SECURITY] Path traversal attempt blocked: %s\n", virtual_path);
            return NULL;
        }
        
        component = next_slash ? next_slash + 1 : component + component_len;
        
        // Prevent infinite loops from malformed paths
        if (component > virtual_path + vpath_len) {
            fprintf(stderr, "[SECURITY] Path parsing error detected\n");
            return NULL;
        }
    }
    
    // Find the best matching mount (longest prefix match)
    ember_mount_point* best_mount = NULL;
    size_t best_len = 0;
    
    // Ensure mount_count is within bounds to prevent out-of-bounds access
    if (vm->mount_count < 0 || vm->mount_count > EMBER_MAX_MOUNTS) {
        fprintf(stderr, "[SECURITY] Invalid mount count: %d\n", vm->mount_count);
        return NULL;
    }
    
    for (int i = 0; i < vm->mount_count; i++) {
        // Validate mount structure integrity
        if (!vm->mounts[i].virtual_path || !vm->mounts[i].host_path) {
            fprintf(stderr, "[SECURITY] Invalid mount at index %d\n", i);
            continue;
        }
        
        size_t mount_len = strlen(vm->mounts[i].virtual_path);
        
        // Prevent buffer overread with mount_len check
        if (mount_len == 0 || mount_len > vpath_len) continue;
        
        // Check if virtual path starts with mount's virtual path
        if (strncmp(virtual_path, vm->mounts[i].virtual_path, mount_len) == 0) {
            // Ensure it's a proper path boundary with safe array access
            if (virtual_path[mount_len] == '\0' || 
                virtual_path[mount_len] == '/' ||
                (mount_len > 0 && vm->mounts[i].virtual_path[mount_len - 1] == '/')) {
                
                if (mount_len > best_len) {
                    best_mount = &vm->mounts[i];
                    best_len = mount_len;
                }
            }
        }
    }
    
    if (!best_mount) {
        fprintf(stderr, "[SECURITY] No valid mount found for path: %s\n", virtual_path);
        return NULL;
    }
    
    // Build the resolved path
    const char* relative_part = virtual_path + best_len;
    if (relative_part[0] == '/') relative_part++; // Skip leading slash
    
    size_t host_len = strlen(best_mount->host_path);
    size_t relative_len = strlen(relative_part);
    
    // Enhanced integer overflow checks
    if (host_len > PATH_MAX || relative_len > PATH_MAX) {
        fprintf(stderr, "[SECURITY] Path component too long\n");
        return NULL;
    }
    
    if (host_len > SIZE_MAX - relative_len - 2) {
        fprintf(stderr, "[SECURITY] Path length overflow prevented\n");
        return NULL;
    }
    
    size_t total_len = host_len + relative_len + 2; // +2 for '/' and '\0'
    
    // Additional bounds check
    if (total_len > PATH_MAX) {
        fprintf(stderr, "[SECURITY] Total path length exceeds maximum: %zu\n", total_len);
        return NULL;
    }
    
    char* resolved = malloc(total_len);
    if (!resolved) {
        fprintf(stderr, "[SECURITY] Memory allocation failed for path resolution\n");
        return NULL;
    }
    
    // Initialize buffer to prevent uninitialized memory access
    memset(resolved, 0, total_len);
    
    // Safely build the path with bounds checking
    if (host_len > 0) {
        memcpy(resolved, best_mount->host_path, host_len);
    }
    size_t pos = host_len;
    
    if (relative_len > 0) {
        if (host_len > 0 && pos > 0 && resolved[pos - 1] != '/') {
            if (pos >= total_len - 1) {
                fprintf(stderr, "[SECURITY] Buffer overflow prevented in path building\n");
                free(resolved);
                return NULL;
            }
            resolved[pos++] = '/';
        }
        
        if (pos + relative_len >= total_len) {
            fprintf(stderr, "[SECURITY] Buffer overflow prevented in relative path copy\n");
            free(resolved);
            return NULL;
        }
        
        memcpy(resolved + pos, relative_part, relative_len);
        pos += relative_len;
    }
    
    if (pos >= total_len) {
        fprintf(stderr, "[SECURITY] Buffer overflow prevented in null termination\n");
        free(resolved);
        return NULL;
    }
    
    resolved[pos] = '\0';
    
    // Final validation: ensure the resolved path is within the mount's host path
    char final_path[PATH_MAX];
    char* realpath_result = realpath(resolved, final_path);
    
    // CRITICAL FIX: Don't free resolved until after realpath check
    if (realpath_result == NULL) {
        // realpath failed - this could be because file doesn't exist, which is OK
        // But we still need to validate the path doesn't escape the mount
        
        // Use the constructed path for validation instead
        size_t host_path_len = strlen(best_mount->host_path);
        if (strncmp(resolved, best_mount->host_path, host_path_len) != 0) {
            fprintf(stderr, "[SECURITY] Path escape attempt blocked (realpath failed): %s -> %s\n", virtual_path, resolved);
            free(resolved);
            return NULL;
        }
    } else {
        // realpath succeeded - validate against resolved real path
        size_t host_path_len = strlen(best_mount->host_path);
        if (strncmp(final_path, best_mount->host_path, host_path_len) != 0) {
            fprintf(stderr, "[SECURITY] Path escape attempt blocked: %s -> %s\n", virtual_path, final_path);
            free(resolved);
            return NULL;
        }
        
        // Update resolved to use the canonical path
        free(resolved);
        resolved = malloc(strlen(final_path) + 1);
        if (!resolved) {
            fprintf(stderr, "[SECURITY] Memory allocation failed for canonical path\n");
            return NULL;
        }
        strcpy(resolved, final_path);
    }
    
    return resolved;
}

// Check if access is allowed to a virtual path
int ember_vfs_check_access(ember_vm* vm, const char* virtual_path, int write_access) {
    if (!vm || !virtual_path) {
        fprintf(stderr, "[SECURITY] Invalid parameters in access check\n");
        return 0;
    }
    
    // Validate input path
    size_t path_len = strlen(virtual_path);
    if (path_len == 0 || path_len >= PATH_MAX) {
        fprintf(stderr, "[SECURITY] Invalid path length in access check: %zu\n", path_len);
        return 0;
    }
    
    // Ensure mount_count is within bounds
    if (vm->mount_count < 0 || vm->mount_count > EMBER_MAX_MOUNTS) {
        fprintf(stderr, "[SECURITY] Invalid mount count in access check: %d\n", vm->mount_count);
        return 0;
    }
    
    // Find the matching mount
    for (int i = 0; i < vm->mount_count; i++) {
        // Validate mount structure
        if (!vm->mounts[i].virtual_path || !vm->mounts[i].host_path) {
            fprintf(stderr, "[SECURITY] Invalid mount structure at index %d\n", i);
            continue;
        }
        
        size_t mount_len = strlen(vm->mounts[i].virtual_path);
        
        // Prevent buffer overread
        if (mount_len == 0 || mount_len > path_len) continue;
        
        if (strncmp(virtual_path, vm->mounts[i].virtual_path, mount_len) == 0) {
            if (virtual_path[mount_len] == '\0' || 
                virtual_path[mount_len] == '/' ||
                (mount_len > 0 && vm->mounts[i].virtual_path[mount_len - 1] == '/')) {
                
                // Check permissions
                if (write_access && (vm->mounts[i].flags & EMBER_MOUNT_RO)) {
                    fprintf(stderr, "[SECURITY] Write access denied on read-only mount: %s\n", virtual_path);
                    return 0; // Write access denied on read-only mount
                }
                return 1; // Access allowed
            }
        }
    }
    
    fprintf(stderr, "[SECURITY] No mount found for path: %s\n", virtual_path);
    return 0; // No mount found - access denied
}

// Clean up VFS resources
void ember_vfs_cleanup(ember_vm* vm) {
    if (!vm) return;
    
    for (int i = 0; i < vm->mount_count; i++) {
        free(vm->mounts[i].virtual_path);
        free(vm->mounts[i].host_path);
    }
    
    free(vm->mounts);
    vm->mounts = NULL;
    vm->mount_count = 0;
    vm->mount_capacity = 0;
}