/**
 * Enhanced file system operations for Ember runtime
 * Provides advanced file and directory management capabilities
 */

#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>

#define MAX_PATH_LENGTH 4096
#define MAX_FILE_SIZE (100 * 1024 * 1024) // 100MB limit

// Security validation for file paths
static int validate_file_path(const char* path) {
    if (!path || strlen(path) == 0 || strlen(path) >= MAX_PATH_LENGTH) {
        return 0;
    }
    
    // Check for null bytes
    if (strlen(path) != strlen(path)) {
        return 0;
    }
    
    // Check for directory traversal attempts
    if (strstr(path, "..") || strstr(path, "/.") || path[0] == '/') {
        return 0;
    }
    
    return 1;
}

// Create directory
ember_value ember_native_mkdir(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_bool(0);
    }
    
    int result = mkdir(path_str->chars, 0755);
    return ember_make_bool(result == 0);
}

// Remove directory (empty only)
ember_value ember_native_rmdir(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_bool(0);
    }
    
    int result = rmdir(path_str->chars);
    return ember_make_bool(result == 0);
}

// List directory contents
ember_value ember_native_listdir(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_nil();
    }
    
    DIR* dir = opendir(path_str->chars);
    if (!dir) {
        return ember_make_nil();
    }
    
    // Build comma-separated list of filenames
    char* result_buffer = malloc(4096);
    if (!result_buffer) {
        closedir(dir);
        return ember_make_nil();
    }
    
    result_buffer[0] = '\0';
    int first = 1;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        if (!first) {
            strcat(result_buffer, ",");
        }
        strcat(result_buffer, entry->d_name);
        first = 0;
    }
    
    closedir(dir);
    
    ember_value result = ember_make_string_gc(vm, result_buffer);
    free(result_buffer);
    
    return result;
}

// Check if path is a directory
ember_value ember_native_isdir(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_bool(0);
    }
    
    struct stat st;
    if (stat(path_str->chars, &st) == 0) {
        return ember_make_bool(S_ISDIR(st.st_mode));
    }
    
    return ember_make_bool(0);
}

// Check if path is a regular file
ember_value ember_native_isfile(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_bool(0);
    }
    
    struct stat st;
    if (stat(path_str->chars, &st) == 0) {
        return ember_make_bool(S_ISREG(st.st_mode));
    }
    
    return ember_make_bool(0);
}

// Get file size
ember_value ember_native_filesize(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_number(-1);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_number(-1);
    }
    
    struct stat st;
    if (stat(path_str->chars, &st) == 0 && S_ISREG(st.st_mode)) {
        return ember_make_number((double)st.st_size);
    }
    
    return ember_make_number(-1);
}

// Get file modification time (Unix timestamp)
ember_value ember_native_file_mtime(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_number(-1);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_number(-1);
    }
    
    struct stat st;
    if (stat(path_str->chars, &st) == 0) {
        return ember_make_number((double)st.st_mtime);
    }
    
    return ember_make_number(-1);
}

// Delete file
ember_value ember_native_unlink(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_bool(0);
    }
    
    int result = unlink(path_str->chars);
    return ember_make_bool(result == 0);
}

// Rename/move file
ember_value ember_native_rename(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* old_path = AS_STRING(argv[0]);
    ember_string* new_path = AS_STRING(argv[1]);
    
    if (!validate_file_path(old_path->chars) || !validate_file_path(new_path->chars)) {
        return ember_make_bool(0);
    }
    
    int result = rename(old_path->chars, new_path->chars);
    return ember_make_bool(result == 0);
}

// Copy file (simple implementation)
ember_value ember_native_copy_file(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* src_path = AS_STRING(argv[0]);
    ember_string* dst_path = AS_STRING(argv[1]);
    
    if (!validate_file_path(src_path->chars) || !validate_file_path(dst_path->chars)) {
        return ember_make_bool(0);
    }
    
    // Check source file size
    struct stat st;
    if (stat(src_path->chars, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size > MAX_FILE_SIZE) {
        return ember_make_bool(0);
    }
    
    FILE* src = fopen(src_path->chars, "rb");
    if (!src) {
        return ember_make_bool(0);
    }
    
    FILE* dst = fopen(dst_path->chars, "wb");
    if (!dst) {
        fclose(src);
        return ember_make_bool(0);
    }
    
    char buffer[8192];
    size_t bytes_copied = 0;
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        if (fwrite(buffer, 1, bytes_read, dst) != bytes_read) {
            fclose(src);
            fclose(dst);
            unlink(dst_path->chars); // Remove partial file
            return ember_make_bool(0);
        }
        bytes_copied += bytes_read;
    }
    
    fclose(src);
    fclose(dst);
    
    return ember_make_bool(bytes_copied == (size_t)st.st_size);
}

// Get current working directory
ember_value ember_native_getcwd(ember_vm* vm, int argc, ember_value* argv) {
    (void)argc;
    (void)argv;
    
    char* cwd = getcwd(NULL, 0);
    if (!cwd) {
        return ember_make_nil();
    }
    
    ember_value result = ember_make_string_gc(vm, cwd);
    free(cwd);
    
    return result;
}

// Create temporary file
ember_value ember_native_mktemp(ember_vm* vm, int argc, ember_value* argv) {
    (void)argc;
    (void)argv;
    
    char template[] = "/tmp/ember_XXXXXX";
    int fd = mkstemp(template);
    
    if (fd == -1) {
        return ember_make_nil();
    }
    
    close(fd); // Close the file descriptor, but keep the file
    return ember_make_string_gc(vm, template);
}

// Check file permissions (simplified)
ember_value ember_native_access(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    ember_string* mode_str = AS_STRING(argv[1]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_bool(0);
    }
    
    int mode = F_OK; // Default to existence check
    
    if (strcmp(mode_str->chars, "r") == 0) {
        mode = R_OK;
    } else if (strcmp(mode_str->chars, "w") == 0) {
        mode = W_OK;
    } else if (strcmp(mode_str->chars, "x") == 0) {
        mode = X_OK;
    } else if (strcmp(mode_str->chars, "rw") == 0) {
        mode = R_OK | W_OK;
    }
    
    int result = access(path_str->chars, mode);
    return ember_make_bool(result == 0);
}

// Read directory with file types (enhanced version)
ember_value ember_native_listdir_detailed(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    
    if (!validate_file_path(path_str->chars)) {
        return ember_make_nil();
    }
    
    DIR* dir = opendir(path_str->chars);
    if (!dir) {
        return ember_make_nil();
    }
    
    // Build JSON-like string with file details
    char* result_buffer = malloc(8192);
    if (!result_buffer) {
        closedir(dir);
        return ember_make_nil();
    }
    
    strcpy(result_buffer, "{");
    int first = 1;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Get full path for stat
        char full_path[MAX_PATH_LENGTH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path_str->chars, entry->d_name);
        
        struct stat st;
        char type = '?';
        long size = -1;
        
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                type = 'd';
            } else if (S_ISREG(st.st_mode)) {
                type = 'f';
                size = st.st_size;
            } else if (S_ISLNK(st.st_mode)) {
                type = 'l';
            }
        }
        
        if (!first) {
            strcat(result_buffer, ",");
        }
        
        char entry_info[512];
        snprintf(entry_info, sizeof(entry_info), "\"%s\":{\"type\":\"%c\",\"size\":%ld}", 
                entry->d_name, type, size);
        strcat(result_buffer, entry_info);
        first = 0;
    }
    
    strcat(result_buffer, "}");
    closedir(dir);
    
    ember_value result = ember_make_string_gc(vm, result_buffer);
    free(result_buffer);
    
    return result;
}