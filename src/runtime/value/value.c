#include "value.h"
#include "../../vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

ember_value ember_make_number(double num) {
    ember_value value;
    value.type = EMBER_VAL_NUMBER;
    value.as.number_val = num;
    return value;
}

ember_value ember_make_bool(int b) {
    ember_value value;
    value.type = EMBER_VAL_BOOL;
    value.as.bool_val = b;
    return value;
}

ember_value ember_make_string(const char* str) {
    ember_value value;
    value.type = EMBER_VAL_STRING;
    
    if (!str) {
        value.as.string_val = NULL;
        return value;
    }
    
    size_t len = strlen(str);
    value.as.string_val = malloc(len + 1);
    if (!value.as.string_val) {
        fprintf(stderr, "[SECURITY] Memory allocation failed for string of length %zu\n", len);
        value.type = EMBER_VAL_NIL;
        return value;
    }
    
    memcpy(value.as.string_val, str, len + 1);
    return value;
}

ember_value ember_make_nil(void) {
    ember_value value;
    value.type = EMBER_VAL_NIL;
    return value;
}

ember_object* allocate_object(ember_vm* vm, size_t size, ember_object_type type) {
    ember_object* object = (ember_object*)malloc(size);
    if (!object) {
        fprintf(stderr, "[SECURITY] Memory allocation failed for object of size %zu\n", size);
        return NULL;
    }
    
    object->type = type;
    object->is_marked = 0;
    object->next = vm->objects;
    vm->objects = object;
    
    vm->bytes_allocated += size;
    if (vm->bytes_allocated > vm->next_gc) {
        collect_garbage(vm);
    }
    
    return object;
}

ember_string* allocate_string(ember_vm* vm, char* chars, int length) {
    ember_string* string = (ember_string*)allocate_object(vm, sizeof(ember_string), OBJ_STRING);
    if (!string) {
        if (chars) free(chars);
        return NULL;
    }
    string->length = length;
    string->chars = chars;
    return string;
}

ember_string* copy_string(ember_vm* vm, const char* chars, int length) {
    if (!chars || length < 0) {
        return NULL;
    }
    
    char* heap_chars = malloc(length + 1);
    if (!heap_chars) {
        fprintf(stderr, "[SECURITY] Memory allocation failed for string of length %d\n", length);
        return NULL;
    }
    
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return allocate_string(vm, heap_chars, length);
}

ember_value ember_make_string_gc(ember_vm* vm, const char* str) {
    ember_value value;
    
    if (!str) {
        value.type = EMBER_VAL_NIL;
        return value;
    }
    
    ember_string* string = copy_string(vm, str, strlen(str));
    if (!string) {
        value.type = EMBER_VAL_NIL;
        return value;
    }
    
    value.type = EMBER_VAL_STRING;
    value.as.obj_val = (ember_object*)string;
    return value;
}

ember_array* allocate_array(ember_vm* vm, int capacity) {
    if (capacity < 0) {
        fprintf(stderr, "[SECURITY] Invalid array capacity: %d\n", capacity);
        return NULL;
    }
    
    ember_array* array = (ember_array*)allocate_object(vm, sizeof(ember_array), OBJ_ARRAY);
    if (!array) {
        return NULL;
    }
    
    array->length = 0;
    array->capacity = capacity;
    
    if (capacity > 0) {
        array->elements = malloc(sizeof(ember_value) * capacity);
        if (!array->elements) {
            fprintf(stderr, "[SECURITY] Memory allocation failed for array elements (capacity: %d)\n", capacity);
            // Note: object is already linked in VM, will be freed by GC
            return NULL;
        }
    } else {
        array->elements = NULL;
    }
    
    return array;
}

ember_value ember_make_array(ember_vm* vm, int capacity) {
    ember_value value;
    
    ember_array* array = allocate_array(vm, capacity);
    if (!array) {
        value.type = EMBER_VAL_NIL;
        return value;
    }
    
    value.type = EMBER_VAL_ARRAY;
    value.as.obj_val = (ember_object*)array;
    return value;
}

void array_push(ember_array* array, ember_value value) {
    if (!array) return;
    
    if (array->length >= array->capacity) {
        int old_capacity = array->capacity;
        
        // Check for integer overflow
        if (old_capacity >= INT_MAX / 2) {
            fprintf(stderr, "[SECURITY] Array capacity overflow prevented\n");
            return;
        }
        
        array->capacity = old_capacity < 8 ? 8 : old_capacity * 2;
        ember_value* new_elements = realloc(array->elements, sizeof(ember_value) * array->capacity);
        
        if (!new_elements) {
            fprintf(stderr, "[SECURITY] Memory reallocation failed for array (new capacity: %d)\n", array->capacity);
            array->capacity = old_capacity;  // Restore old capacity
            return;
        }
        
        array->elements = new_elements;
    }
    array->elements[array->length++] = value;
}

// Hash function for values
uint32_t hash_value(ember_value value) {
    switch (value.type) {
        case EMBER_VAL_NIL:
            return 0;
        case EMBER_VAL_BOOL:
            return value.as.bool_val ? 1 : 0;
        case EMBER_VAL_NUMBER: {
            // Simple hash for numbers - use raw bytes
            union { double d; uint64_t i; } u;
            u.d = value.as.number_val;
            return (uint32_t)(u.i ^ (u.i >> 32));
        }
        case EMBER_VAL_STRING: {
            if (value.as.obj_val) {
                ember_string* str = AS_STRING(value);
                // FNV-1a hash algorithm
                uint32_t hash = 2166136261u;
                for (int i = 0; i < str->length; i++) {
                    hash ^= (uint8_t)str->chars[i];
                    hash *= 16777619;
                }
                return hash;
            }
            return 0;
        }
        default:
            // For other types, use pointer address
            return (uint32_t)((uintptr_t)value.as.obj_val);
    }
}

ember_hash_map* allocate_hash_map(ember_vm* vm, int capacity) {
    ember_hash_map* map = (ember_hash_map*)allocate_object(vm, sizeof(ember_hash_map), OBJ_HASH_MAP);
    if (!map) {
        return NULL;
    }
    
    map->length = 0;
    map->capacity = capacity > 0 ? capacity : 8;
    
    // Check for integer overflow
    if (map->capacity > SIZE_MAX / sizeof(ember_hash_entry)) {
        fprintf(stderr, "[SECURITY] Hash map capacity overflow prevented\n");
        return NULL;
    }
    
    map->entries = malloc(sizeof(ember_hash_entry) * map->capacity);
    if (!map->entries) {
        fprintf(stderr, "[SECURITY] Memory allocation failed for hash map entries (capacity: %d)\n", map->capacity);
        // Note: object is already linked in VM, will be freed by GC
        return NULL;
    }
    
    // Initialize all entries as unoccupied
    for (int i = 0; i < map->capacity; i++) {
        map->entries[i].is_occupied = 0;
        map->entries[i].key = ember_make_nil();
        map->entries[i].value = ember_make_nil();
    }
    
    return map;
}

ember_value ember_make_hash_map(ember_vm* vm, int capacity) {
    ember_value value;
    
    ember_hash_map* map = allocate_hash_map(vm, capacity);
    if (!map) {
        value.type = EMBER_VAL_NIL;
        return value;
    }
    
    value.type = EMBER_VAL_HASH_MAP;
    value.as.obj_val = (ember_object*)map;
    return value;
}

static ember_hash_entry* find_entry(ember_hash_entry* entries, int capacity, ember_value key) {
    uint32_t index = hash_value(key) % capacity;
    
    // Linear probing for collision resolution
    for (int i = 0; i < capacity; i++) {
        ember_hash_entry* entry = &entries[index];
        
        if (!entry->is_occupied || values_equal(entry->key, key)) {
            return entry;
        }
        
        index = (index + 1) % capacity;
    }
    
    return NULL; // Should never happen if capacity > length
}

void hash_map_set(ember_hash_map* map, ember_value key, ember_value value) {
    if (!map) return;
    
    // Store old value for write barrier
    ember_value old_val = hash_map_get(map, key);
    
    // Resize if load factor > 0.75
    if (map->length + 1 > map->capacity * 0.75) {
        int old_capacity = map->capacity;
        ember_hash_entry* old_entries = map->entries;
        
        // Check for integer overflow
        if (map->capacity >= INT_MAX / 2) {
            fprintf(stderr, "[SECURITY] Hash map capacity overflow prevented\n");
            return;
        }
        
        map->capacity *= 2;
        
        // Check for allocation size overflow
        if (map->capacity > SIZE_MAX / sizeof(ember_hash_entry)) {
            fprintf(stderr, "[SECURITY] Hash map allocation size overflow prevented\n");
            map->capacity = old_capacity;
            return;
        }
        
        ember_hash_entry* new_entries = malloc(sizeof(ember_hash_entry) * map->capacity);
        if (!new_entries) {
            fprintf(stderr, "[SECURITY] Memory allocation failed for hash map resize (new capacity: %d)\n", map->capacity);
            map->capacity = old_capacity;
            return;
        }
        
        map->entries = new_entries;
        map->length = 0;
        
        // Initialize new entries
        for (int i = 0; i < map->capacity; i++) {
            map->entries[i].is_occupied = 0;
            map->entries[i].key = ember_make_nil();
            map->entries[i].value = ember_make_nil();
        }
        
        // Rehash existing entries
        for (int i = 0; i < old_capacity; i++) {
            if (old_entries[i].is_occupied) {
                hash_map_set(map, old_entries[i].key, old_entries[i].value);
            }
        }
        
        free(old_entries);
    }
    
    ember_hash_entry* entry = find_entry(map->entries, map->capacity, key);
    if (entry) {
        int is_new_key = !entry->is_occupied;
        entry->key = key;
        entry->value = value;
        entry->is_occupied = 1;
        
        if (is_new_key) {
            map->length++;
        }
        
    }
}


// VM-aware hash map set with write barriers
void hash_map_set_with_vm(ember_vm* vm, ember_hash_map* map, ember_value key, ember_value value) {
    if (!map || !vm) return;
    
    // Store old value for write barrier
    ember_value old_val = hash_map_get(map, key);
    
    // Call the regular hash_map_set
    hash_map_set(map, key, value);
    
    // Trigger write barrier for GC
    gc_write_barrier_helper(vm, (ember_object*)map, old_val, value);
}

ember_value hash_map_get(ember_hash_map* map, ember_value key) {
    ember_hash_entry* entry = find_entry(map->entries, map->capacity, key);
    if (entry && entry->is_occupied) {
        return entry->value;
    }
    return ember_make_nil();
}

int hash_map_has_key(ember_hash_map* map, ember_value key) {
    ember_hash_entry* entry = find_entry(map->entries, map->capacity, key);
    return entry && entry->is_occupied;
}

ember_value concatenate_strings(ember_vm* vm, ember_value a, ember_value b) {
    if (!vm || a.type != EMBER_VAL_STRING || b.type != EMBER_VAL_STRING) {
        ember_value nil_val;
        nil_val.type = EMBER_VAL_NIL;
        return nil_val;
    }
    
    ember_string* a_string = AS_STRING(a);
    ember_string* b_string = AS_STRING(b);
    
    if (!a_string || !b_string) {
        ember_value nil_val;
        nil_val.type = EMBER_VAL_NIL;
        return nil_val;
    }
    
    // Check for integer overflow
    if (a_string->length > INT_MAX - b_string->length) {
        fprintf(stderr, "[SECURITY] String concatenation length overflow prevented\n");
        ember_value nil_val;
        nil_val.type = EMBER_VAL_NIL;
        return nil_val;
    }
    
    int length = a_string->length + b_string->length;
    char* chars = malloc(length + 1);
    
    if (!chars) {
        fprintf(stderr, "[SECURITY] Memory allocation failed for string concatenation (length: %d)\n", length);
        ember_value nil_val;
        nil_val.type = EMBER_VAL_NIL;
        return nil_val;
    }
    
    memcpy(chars, a_string->chars, a_string->length);
    memcpy(chars + a_string->length, b_string->chars, b_string->length);
    chars[length] = '\0';
    
    ember_string* result = allocate_string(vm, chars, length);
    if (!result) {
        ember_value nil_val;
        nil_val.type = EMBER_VAL_NIL;
        return nil_val;
    }
    
    ember_value value;
    value.type = EMBER_VAL_STRING;
    value.as.obj_val = (ember_object*)result;
    return value;
}

void free_ember_value(ember_value value) {
    switch (value.type) {
        case EMBER_VAL_STRING:
            // Only free legacy strings, GC-managed strings are freed by GC
            if (value.as.string_val && !value.as.obj_val) {
                free(value.as.string_val);
            }
            break;
        case EMBER_VAL_FUNCTION:
            if (value.as.func_val.name) {
                free(value.as.func_val.name);
            }
            break;
        case EMBER_VAL_ARRAY:
            // GC-managed arrays are freed by GC
            break;
        case EMBER_VAL_HASH_MAP:
            // GC-managed hash maps are freed by GC
            break;
        default:
            break;
    }
}

ember_value copy_ember_value(ember_value value) {
    ember_value copy = value;
    switch (value.type) {
        case EMBER_VAL_STRING:
            if (value.as.string_val) {
                size_t len = strlen(value.as.string_val);
                copy.as.string_val = malloc(len + 1);
                if (!copy.as.string_val) {
                    fprintf(stderr, "[SECURITY] Memory allocation failed for string copy (length: %zu)\n", len);
                    copy.type = EMBER_VAL_NIL;
                    return copy;
                }
                memcpy(copy.as.string_val, value.as.string_val, len + 1);
            }
            break;
        case EMBER_VAL_FUNCTION:
            if (value.as.func_val.name) {
                size_t len = strlen(value.as.func_val.name);
                copy.as.func_val.name = malloc(len + 1);
                if (!copy.as.func_val.name) {
                    fprintf(stderr, "[SECURITY] Memory allocation failed for function name copy (length: %zu)\n", len);
                    copy.type = EMBER_VAL_NIL;
                    return copy;
                }
                memcpy(copy.as.func_val.name, value.as.func_val.name, len + 1);
            }
            break;
        case EMBER_VAL_ARRAY:
            // GC-managed arrays don't need deep copy for values
            break;
        case EMBER_VAL_HASH_MAP:
            // GC-managed hash maps don't need deep copy for values
            break;
        default:
            break;
    }
    return copy;
}

int values_equal(ember_value a, ember_value b) {
    if (a.type != b.type) return 0;
    
    switch (a.type) {
        case EMBER_VAL_NIL:
            return 1;
        case EMBER_VAL_BOOL:
            return a.as.bool_val == b.as.bool_val;
        case EMBER_VAL_NUMBER:
            return a.as.number_val == b.as.number_val;
        case EMBER_VAL_STRING:
            if (a.as.obj_val && b.as.obj_val) {
                ember_string* a_str = AS_STRING(a);
                ember_string* b_str = AS_STRING(b);
                // Optimization: if both strings are interned, we can compare pointers
                if (a_str == b_str) {
                    return 1;
                }
                // Otherwise, compare contents
                return a_str->length == b_str->length &&
                       memcmp(a_str->chars, b_str->chars, a_str->length) == 0;
            }
            return a.as.obj_val == b.as.obj_val;
        case EMBER_VAL_ARRAY:
            if (a.as.obj_val && b.as.obj_val) {
                ember_array* a_arr = AS_ARRAY(a);
                ember_array* b_arr = AS_ARRAY(b);
                if (a_arr->length != b_arr->length) return 0;
                for (int i = 0; i < a_arr->length; i++) {
                    if (!values_equal(a_arr->elements[i], b_arr->elements[i])) {
                        return 0;
                    }
                }
                return 1;
            }
            return a.as.obj_val == b.as.obj_val;
        case EMBER_VAL_HASH_MAP:
            if (a.as.obj_val && b.as.obj_val) {
                ember_hash_map* a_map = AS_HASH_MAP(a);
                ember_hash_map* b_map = AS_HASH_MAP(b);
                if (a_map->length != b_map->length) return 0;
                
                // Check if all key-value pairs match
                for (int i = 0; i < a_map->capacity; i++) {
                    if (a_map->entries[i].is_occupied) {
                        ember_value b_value = hash_map_get(b_map, a_map->entries[i].key);
                        if (!values_equal(a_map->entries[i].value, b_value)) {
                            return 0;
                        }
                    }
                }
                return 1;
            }
            return a.as.obj_val == b.as.obj_val;
        default:
            return 0;
    }
}

void print_value(ember_value value) {
    switch (value.type) {
        case EMBER_VAL_NIL:
            printf("nil");
            break;
        case EMBER_VAL_BOOL:
            printf(value.as.bool_val ? "true" : "false");
            break;
        case EMBER_VAL_NUMBER:
            printf("%.15g", value.as.number_val);
            break;
        case EMBER_VAL_STRING:
            if (value.as.obj_val) {
                printf("%s", AS_CSTRING(value));
            } else if (value.as.string_val) {
                printf("%s", value.as.string_val);
            }
            break;
        case EMBER_VAL_FUNCTION:
            if (value.as.obj_val && value.as.obj_val->type == OBJ_METHOD) {
                ember_bound_method* bound = AS_BOUND_METHOD(value);
                printf("<bound method>");
            } else {
                printf("<function %s>", value.as.func_val.name ? value.as.func_val.name : "anonymous");
            }
            break;
        case EMBER_VAL_NATIVE:
            printf("<native function>");
            break;
        case EMBER_VAL_ARRAY: {
            ember_array* array = AS_ARRAY(value);
            printf("[");
            for (int i = 0; i < array->length; i++) {
                if (i > 0) printf(", ");
                print_value(array->elements[i]);
            }
            printf("]");
            break;
        }
        case EMBER_VAL_HASH_MAP: {
            ember_hash_map* map = AS_HASH_MAP(value);
            printf("{");
            int first = 1;
            for (int i = 0; i < map->capacity; i++) {
                if (map->entries[i].is_occupied) {
                    if (!first) printf(", ");
                    print_value(map->entries[i].key);
                    printf(": ");
                    print_value(map->entries[i].value);
                    first = 0;
                }
            }
            printf("}");
            break;
        }
        case EMBER_VAL_EXCEPTION: {
            ember_exception* exc = AS_EXCEPTION(value);
            printf("<%s: %s>", exc->type ? exc->type : "Exception", 
                   exc->message ? exc->message : "");
            break;
        }
        case EMBER_VAL_CLASS: {
            ember_class* klass = AS_CLASS(value);
            printf("<class %s>", klass->name ? klass->name->chars : "unnamed");
            break;
        }
        case EMBER_VAL_INSTANCE: {
            ember_instance* instance = AS_INSTANCE(value);
            printf("<%s instance>", instance->klass->name ? instance->klass->name->chars : "unnamed");
            break;
        }
    }
}

ember_value ember_make_exception(ember_vm* vm, const char* type, const char* message) {
    ember_exception* exc = (ember_exception*)allocate_object(vm, sizeof(ember_exception), OBJ_EXCEPTION);
    if (!exc) {
        return ember_make_nil();
    }
    
    // Copy type string
    if (type) {
        size_t type_len = strlen(type);
        exc->type = malloc(type_len + 1);
        if (exc->type) {
            memcpy(exc->type, type, type_len + 1);
        }
    } else {
        exc->type = NULL;
    }
    
    // Copy message string
    if (message) {
        size_t msg_len = strlen(message);
        exc->message = malloc(msg_len + 1);
        if (exc->message) {
            memcpy(exc->message, message, msg_len + 1);
        }
    } else {
        exc->message = NULL;
    }
    
    exc->line = 0; // Will be set by VM during throw
    exc->stack_trace = ember_make_nil();
    
    ember_value value;
    value.type = EMBER_VAL_EXCEPTION;
    value.as.obj_val = (ember_object*)exc;
    return value;
}

// String interning stubs (simplified implementation for now)
void init_string_intern_table(ember_vm* vm) {
    // Placeholder - string interning disabled for now
    if (vm) vm->string_intern_table = NULL;
}

void free_string_intern_table(ember_vm* vm) {
    // Placeholder - string interning disabled for now
    if (vm) vm->string_intern_table = NULL;
}

ember_string* find_interned_string(ember_vm* vm, const char* chars, int length) {
    // Placeholder - always return NULL (not found)
    (void)vm; (void)chars; (void)length;
    return NULL;
}

ember_string* intern_string(ember_vm* vm, const char* chars, int length) {
    // Placeholder - just use regular copy_string for now
    return copy_string(vm, chars, length);
}

// OOP Value creation functions

ember_class* allocate_class(ember_vm* vm, const char* name) {
    ember_class* klass = (ember_class*)allocate_object(vm, sizeof(ember_class), OBJ_CLASS);
    if (!klass) {
        return NULL;
    }
    
    // Set class name
    klass->name = copy_string(vm, name, strlen(name));
    if (!klass->name) {
        return NULL;
    }
    
    // Initialize method table
    klass->methods = allocate_hash_map(vm, 8);
    if (!klass->methods) {
        return NULL;
    }
    
    // No superclass by default
    klass->superclass = NULL;
    
    return klass;
}

ember_instance* allocate_instance(ember_vm* vm, ember_class* klass) {
    ember_instance* instance = (ember_instance*)allocate_object(vm, sizeof(ember_instance), OBJ_INSTANCE);
    if (!instance) {
        return NULL;
    }
    
    instance->klass = klass;
    
    // Initialize instance fields
    instance->fields = allocate_hash_map(vm, 8);
    if (!instance->fields) {
        return NULL;
    }
    
    return instance;
}

ember_bound_method* allocate_bound_method(ember_vm* vm, ember_value receiver, ember_value method) {
    ember_bound_method* bound = (ember_bound_method*)allocate_object(vm, sizeof(ember_bound_method), OBJ_METHOD);
    if (!bound) {
        return NULL;
    }
    
    bound->receiver = receiver;
    bound->method = method;
    
    return bound;
}

ember_value ember_make_class(ember_vm* vm, const char* name) {
    ember_class* klass = allocate_class(vm, name);
    if (!klass) {
        return ember_make_nil();
    }
    
    ember_value value;
    value.type = EMBER_VAL_CLASS;
    value.as.obj_val = (ember_object*)klass;
    return value;
}

ember_value ember_make_instance(ember_vm* vm, ember_class* klass) {
    ember_instance* instance = allocate_instance(vm, klass);
    if (!instance) {
        return ember_make_nil();
    }
    
    ember_value value;
    value.type = EMBER_VAL_INSTANCE;
    value.as.obj_val = (ember_object*)instance;
    return value;
}

ember_value ember_make_bound_method(ember_vm* vm, ember_value receiver, ember_value method) {
    ember_bound_method* bound = allocate_bound_method(vm, receiver, method);
    if (!bound) {
        return ember_make_nil();
    }
    
    ember_value value;
    value.type = EMBER_VAL_FUNCTION; // Bound methods are callable
    value.as.obj_val = (ember_object*)bound;
    return value;
}