/**
 * Simple functional file I/O implementations to replace stubs
 */

#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

ember_value ember_native_file_exists_working(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    struct stat st;
    int result = stat(path_str->chars, &st);
    
    return ember_make_bool(result == 0);
}

ember_value ember_native_read_file_working(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    FILE* file = fopen(path_str->chars, "r");
    if (!file) {
        return ember_make_nil();
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (size < 0 || size > 10 * 1024 * 1024) { // Limit to 10MB
        fclose(file);
        return ember_make_nil();
    }
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        return ember_make_nil();
    }
    
    size_t bytes_read = fread(buffer, 1, size, file);
    fclose(file);
    
    buffer[bytes_read] = '\0';
    ember_value result = ember_make_string_gc(vm, buffer);
    free(buffer);
    
    return result;
}

ember_value ember_native_write_file_working(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    ember_string* content_str = AS_STRING(argv[1]);
    
    FILE* file = fopen(path_str->chars, "w");
    if (!file) {
        return ember_make_bool(0);
    }
    
    size_t content_len = strlen(content_str->chars);
    size_t written = fwrite(content_str->chars, 1, content_len, file);
    fclose(file);
    
    return ember_make_bool(written == content_len);
}

ember_value ember_native_append_file_working(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* path_str = AS_STRING(argv[0]);
    ember_string* content_str = AS_STRING(argv[1]);
    
    FILE* file = fopen(path_str->chars, "a");
    if (!file) {
        return ember_make_bool(0);
    }
    
    size_t content_len = strlen(content_str->chars);
    size_t written = fwrite(content_str->chars, 1, content_len, file);
    fclose(file);
    
    return ember_make_bool(written == content_len);
}

// Function aliases for compatibility
ember_value ember_native_read_file(ember_vm* vm, int argc, ember_value* argv) {
    return ember_native_read_file_working(vm, argc, argv);
}

ember_value ember_native_write_file(ember_vm* vm, int argc, ember_value* argv) {
    return ember_native_write_file_working(vm, argc, argv);
}

ember_value ember_native_append_file(ember_vm* vm, int argc, ember_value* argv) {
    return ember_native_append_file_working(vm, argc, argv);
}

ember_value ember_native_file_exists(ember_vm* vm, int argc, ember_value* argv) {
    return ember_native_file_exists_working(vm, argc, argv);
}
