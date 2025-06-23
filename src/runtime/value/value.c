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
    if ((size_t)map->capacity > SIZE_MAX / sizeof(ember_hash_entry)) {
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
    ember_value old_val __attribute__((unused)) = hash_map_get(map, key);
    
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
        if ((size_t)map->capacity > SIZE_MAX / sizeof(ember_hash_entry)) {
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
        case EMBER_VAL_SET:
            if (a.as.obj_val && b.as.obj_val) {
                ember_set* a_set = AS_SET(a);
                ember_set* b_set = AS_SET(b);
                if (a_set->size != b_set->size) return 0;
                
                // Check if all elements in a are also in b
                for (int i = 0; i < a_set->elements->capacity; i++) {
                    if (a_set->elements->entries[i].is_occupied) {
                        if (!set_has(b_set, a_set->elements->entries[i].key)) {
                            return 0;
                        }
                    }
                }
                return 1;
            }
            return a.as.obj_val == b.as.obj_val;
        case EMBER_VAL_MAP:
            if (a.as.obj_val && b.as.obj_val) {
                ember_map* a_map = AS_MAP(a);
                ember_map* b_map = AS_MAP(b);
                if (a_map->size != b_map->size) return 0;
                
                // Check if all key-value pairs match
                for (int i = 0; i < a_map->entries->capacity; i++) {
                    if (a_map->entries->entries[i].is_occupied) {
                        ember_value b_value = map_get(b_map, a_map->entries->entries[i].key);
                        if (!values_equal(a_map->entries->entries[i].value, b_value)) {
                            return 0;
                        }
                    }
                }
                return 1;
            }
            return a.as.obj_val == b.as.obj_val;
        case EMBER_VAL_REGEX:
            if (a.as.obj_val && b.as.obj_val) {
                ember_regex* a_regex = AS_REGEX(a);
                ember_regex* b_regex = AS_REGEX(b);
                if (a_regex->flags != b_regex->flags) return 0;
                if (!a_regex->pattern && !b_regex->pattern) return 1;
                if (!a_regex->pattern || !b_regex->pattern) return 0;
                return strcmp(a_regex->pattern, b_regex->pattern) == 0;
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
        case EMBER_VAL_PROMISE: {
            ember_promise* promise = AS_PROMISE(value);
            printf("<Promise [%s]>", 
                   promise->state == PROMISE_PENDING ? "pending" :
                   promise->state == PROMISE_RESOLVED ? "resolved" : "rejected");
            break;
        }
        case EMBER_VAL_GENERATOR: {
            ember_generator* generator = AS_GENERATOR(value);
            printf("<Generator [%s]>",
                   generator->state == GENERATOR_CREATED ? "created" :
                   generator->state == GENERATOR_SUSPENDED ? "suspended" : "completed");
            break;
        }
        case EMBER_VAL_SET: {
            ember_set* set = AS_SET(value);
            printf("Set(%d) {", set->size);
            int first = 1;
            for (int i = 0; i < set->elements->capacity; i++) {
                if (set->elements->entries[i].is_occupied) {
                    if (!first) printf(", ");
                    print_value(set->elements->entries[i].key);
                    first = 0;
                }
            }
            printf("}");
            break;
        }
        case EMBER_VAL_MAP: {
            ember_map* map = AS_MAP(value);
            printf("Map(%d) {", map->size);
            int first = 1;
            for (int i = 0; i < map->entries->capacity; i++) {
                if (map->entries->entries[i].is_occupied) {
                    if (!first) printf(", ");
                    print_value(map->entries->entries[i].key);
                    printf(" => ");
                    print_value(map->entries->entries[i].value);
                    first = 0;
                }
            }
            printf("}");
            break;
        }
        case EMBER_VAL_REGEX: {
            ember_regex* regex = AS_REGEX(value);
            printf("/");
            if (regex->pattern) {
                printf("%s", regex->pattern);
            }
            printf("/");
            if (regex->flags & REGEX_GLOBAL) printf("g");
            if (regex->flags & REGEX_CASE_INSENSITIVE) printf("i");
            if (regex->flags & REGEX_MULTILINE) printf("m");
            if (regex->flags & REGEX_DOTALL) printf("s");
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

// Set allocation and creation functions
ember_set* allocate_set(ember_vm* vm) {
    ember_set* set = (ember_set*)allocate_object(vm, sizeof(ember_set), OBJ_SET);
    if (!set) {
        return NULL;
    }
    
    // Initialize with internal hash map for element storage
    set->elements = allocate_hash_map(vm, 8);
    if (!set->elements) {
        return NULL;
    }
    
    set->size = 0;
    return set;
}

ember_value ember_make_set(ember_vm* vm) {
    ember_set* set = allocate_set(vm);
    if (!set) {
        return ember_make_nil();
    }
    
    ember_value value;
    value.type = EMBER_VAL_SET;
    value.as.obj_val = (ember_object*)set;
    return value;
}

// Map allocation and creation functions
ember_map* allocate_map(ember_vm* vm) {
    ember_map* map = (ember_map*)allocate_object(vm, sizeof(ember_map), OBJ_MAP);
    if (!map) {
        return NULL;
    }
    
    // Initialize with internal hash map for key-value storage
    map->entries = allocate_hash_map(vm, 8);
    if (!map->entries) {
        return NULL;
    }
    
    map->size = 0;
    return map;
}

ember_value ember_make_map(ember_vm* vm) {
    ember_map* map = allocate_map(vm);
    if (!map) {
        return ember_make_nil();
    }
    
    ember_value value;
    value.type = EMBER_VAL_MAP;
    value.as.obj_val = (ember_object*)map;
    return value;
}

// Set operation functions
int set_add(ember_set* set, ember_value element) {
    if (!set) return 0;
    
    // Check if element already exists
    if (hash_map_has_key(set->elements, element)) {
        return 1; // Element already exists, no change
    }
    
    // Add element using itself as both key and value (Set behavior)
    hash_map_set(set->elements, element, element);
    set->size = set->elements->length;
    return 1;
}

int set_has(ember_set* set, ember_value element) {
    if (!set) return 0;
    return hash_map_has_key(set->elements, element);
}

int set_delete(ember_set* set, ember_value element) {
    if (!set) return 0;
    
    if (!hash_map_has_key(set->elements, element)) {
        return 0; // Element doesn't exist
    }
    
    // Find and remove the element
    ember_hash_entry* entry = find_entry(set->elements->entries, set->elements->capacity, element);
    if (entry && entry->is_occupied) {
        entry->is_occupied = 0;
        entry->key = ember_make_nil();
        entry->value = ember_make_nil();
        set->elements->length--;
        set->size = set->elements->length;
        return 1;
    }
    
    return 0;
}

void set_clear(ember_set* set) {
    if (!set) return;
    
    // Clear all entries
    for (int i = 0; i < set->elements->capacity; i++) {
        set->elements->entries[i].is_occupied = 0;
        set->elements->entries[i].key = ember_make_nil();
        set->elements->entries[i].value = ember_make_nil();
    }
    
    set->elements->length = 0;
    set->size = 0;
}

// Map operation functions
int map_set(ember_map* map, ember_value key, ember_value value) {
    if (!map) return 0;
    
    int had_key = hash_map_has_key(map->entries, key);
    hash_map_set(map->entries, key, value);
    
    if (!had_key) {
        map->size = map->entries->length;
    }
    
    return 1;
}

ember_value map_get(ember_map* map, ember_value key) {
    if (!map) return ember_make_nil();
    return hash_map_get(map->entries, key);
}

int map_has(ember_map* map, ember_value key) {
    if (!map) return 0;
    return hash_map_has_key(map->entries, key);
}

int map_delete(ember_map* map, ember_value key) {
    if (!map) return 0;
    
    if (!hash_map_has_key(map->entries, key)) {
        return 0; // Key doesn't exist
    }
    
    // Find and remove the key-value pair
    ember_hash_entry* entry = find_entry(map->entries->entries, map->entries->capacity, key);
    if (entry && entry->is_occupied) {
        entry->is_occupied = 0;
        entry->key = ember_make_nil();
        entry->value = ember_make_nil();
        map->entries->length--;
        map->size = map->entries->length;
        return 1;
    }
    
    return 0;
}

void map_clear(ember_map* map) {
    if (!map) return;
    
    // Clear all entries
    for (int i = 0; i < map->entries->capacity; i++) {
        map->entries->entries[i].is_occupied = 0;
        map->entries->entries[i].key = ember_make_nil();
        map->entries->entries[i].value = ember_make_nil();
    }
    
    map->entries->length = 0;
    map->size = 0;
}

// Regex allocation and creation functions
ember_regex* allocate_regex(ember_vm* vm, const char* pattern, ember_regex_flags flags) {
    ember_regex* regex = (ember_regex*)allocate_object(vm, sizeof(ember_regex), OBJ_REGEX);
    if (!regex) {
        return NULL;
    }
    
    // Copy pattern string
    if (pattern) {
        size_t pattern_len = strlen(pattern);
        regex->pattern = malloc(pattern_len + 1);
        if (!regex->pattern) {
            return NULL;
        }
        memcpy(regex->pattern, pattern, pattern_len + 1);
    } else {
        regex->pattern = NULL;
    }
    
    regex->flags = flags;
    regex->compiled_regex = NULL; // Simple implementation - no actual compilation
    regex->groups = allocate_array(vm, 8); // Start with capacity for 8 groups
    regex->last_index = 0;
    
    return regex;
}

ember_value ember_make_regex(ember_vm* vm, const char* pattern, ember_regex_flags flags) {
    ember_regex* regex = allocate_regex(vm, pattern, flags);
    if (!regex) {
        return ember_make_nil();
    }
    
    ember_value value;
    value.type = EMBER_VAL_REGEX;
    value.as.obj_val = (ember_object*)regex;
    return value;
}

// Simple regex operation implementations (basic functionality)
int regex_test(ember_regex* regex, const char* text) {
    if (!regex || !regex->pattern || !text) {
        return 0;
    }
    
    // Simple substring search for basic patterns
    // This is a simplified implementation - in production would use PCRE or similar
    if (regex->flags & REGEX_CASE_INSENSITIVE) {
        // Case-insensitive search (simplified)
        const char* found = strstr(text, regex->pattern);
        return found != NULL;
    } else {
        // Case-sensitive search
        const char* found = strstr(text, regex->pattern);
        return found != NULL;
    }
}

ember_array* regex_match(ember_vm* vm, ember_regex* regex, const char* text) {
    if (!regex || !regex->pattern || !text) {
        return NULL;
    }
    
    ember_array* matches = allocate_array(vm, 4);
    if (!matches) {
        return NULL;
    }
    
    // Simple implementation - find first match
    const char* found = strstr(text, regex->pattern);
    if (found) {
        // Create match result with position and matched text
        ember_regex_match match_info;
        match_info.start = found - text;
        match_info.end = match_info.start + strlen(regex->pattern);
        
        size_t match_len = strlen(regex->pattern);
        match_info.match = malloc(match_len + 1);
        if (match_info.match) {
            memcpy(match_info.match, regex->pattern, match_len + 1);
        }
        
        // Add match to results array
        ember_value match_val = ember_make_string_gc(vm, match_info.match);
        array_push(matches, match_val);
        
        if (match_info.match) {
            free(match_info.match);
        }
    }
    
    return matches;
}

ember_value regex_replace(ember_vm* vm, ember_regex* regex, const char* text, const char* replacement) {
    if (!regex || !regex->pattern || !text || !replacement) {
        return ember_make_nil();
    }
    
    // Simple replace implementation
    const char* found = strstr(text, regex->pattern);
    if (!found) {
        // No match found, return original text
        return ember_make_string_gc(vm, text);
    }
    
    size_t pattern_len = strlen(regex->pattern);
    size_t replacement_len = strlen(replacement);
    size_t prefix_len = found - text;
    size_t suffix_len = strlen(found + pattern_len);
    size_t result_len = prefix_len + replacement_len + suffix_len;
    
    char* result = malloc(result_len + 1);
    if (!result) {
        return ember_make_nil();
    }
    
    // Build result string
    memcpy(result, text, prefix_len);
    memcpy(result + prefix_len, replacement, replacement_len);
    memcpy(result + prefix_len + replacement_len, found + pattern_len, suffix_len);
    result[result_len] = '\0';
    
    ember_value result_val = ember_make_string_gc(vm, result);
    free(result);
    
    return result_val;
}

ember_array* regex_split(ember_vm* vm, ember_regex* regex, const char* text) {
    if (!regex || !regex->pattern || !text) {
        return NULL;
    }
    
    ember_array* parts = allocate_array(vm, 4);
    if (!parts) {
        return NULL;
    }
    
    const char* current = text;
    const char* found;
    
    while ((found = strstr(current, regex->pattern)) != NULL) {
        // Add part before the match
        size_t part_len = found - current;
        char* part = malloc(part_len + 1);
        if (part) {
            memcpy(part, current, part_len);
            part[part_len] = '\0';
            ember_value part_val = ember_make_string_gc(vm, part);
            array_push(parts, part_val);
            free(part);
        }
        
        // Move past the match
        current = found + strlen(regex->pattern);
    }
    
    // Add remaining part
    if (*current) {
        ember_value remaining_val = ember_make_string_gc(vm, current);
        array_push(parts, remaining_val);
    }
    
    return parts;
}