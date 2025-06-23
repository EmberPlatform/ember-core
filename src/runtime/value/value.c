#include "value.h"
#include "../../vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>

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

// Convert value type to string representation
const char* value_type_to_string(ember_val_type type) {
    switch (type) {
        case EMBER_VAL_NIL: return "nil";
        case EMBER_VAL_BOOL: return "boolean";
        case EMBER_VAL_NUMBER: return "number";
        case EMBER_VAL_STRING: return "string";
        case EMBER_VAL_FUNCTION: return "function";
        case EMBER_VAL_NATIVE: return "native_function";
        case EMBER_VAL_ARRAY: return "array";
        case EMBER_VAL_HASH_MAP: return "hash_map";
        case EMBER_VAL_EXCEPTION: return "exception";
        case EMBER_VAL_CLASS: return "class";
        case EMBER_VAL_INSTANCE: return "instance";
        case EMBER_VAL_PROMISE: return "promise";
        case EMBER_VAL_GENERATOR: return "generator";
        case EMBER_VAL_SET: return "set";
        case EMBER_VAL_MAP: return "map";
        case EMBER_VAL_REGEX: return "regex";
        default: return "unknown";
    }
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

// Enhanced hash function for values with comprehensive type support
uint32_t hash_value(ember_value value) {
    switch (value.type) {
        case EMBER_VAL_NIL:
            return 0;
        case EMBER_VAL_BOOL:
            return value.as.bool_val ? 1 : 0;
        case EMBER_VAL_NUMBER: {
            // Robust hash for numbers - handle special cases and precision
            double d = value.as.number_val;
            if (d == 0.0) return 0; // Handle +0.0 and -0.0 as same
            if (d != d) return 2147483647u; // NaN handling
            
            union { double d; uint64_t i; } u;
            u.d = d;
            // Mix high and low bits for better distribution
            uint64_t hash64 = u.i;
            hash64 ^= hash64 >> 32;
            hash64 *= 0x9e3779b97f4a7c15ULL; // Golden ratio constant
            return (uint32_t)(hash64 ^ (hash64 >> 32));
        }
        case EMBER_VAL_STRING: {
            if (value.as.obj_val) {
                ember_string* str = AS_STRING(value);
                if (!str || !str->chars) return 0;
                
                // Enhanced FNV-1a with better avalanche properties
                uint32_t hash = 2166136261u;
                for (int i = 0; i < str->length; i++) {
                    hash ^= (uint8_t)str->chars[i];
                    hash *= 16777619u;
                }
                // Additional mixing for better distribution
                hash ^= hash >> 16;
                hash *= 0x85ebca6b;
                hash ^= hash >> 13;
                return hash;
            }
            return 0;
        }
        case EMBER_VAL_ARRAY: {
            if (value.as.obj_val) {
                ember_array* arr = AS_ARRAY(value);
                uint32_t hash = 2166136261u;
                // Hash based on array elements (up to first 8 for performance)
                int limit = arr->length > 8 ? 8 : arr->length;
                for (int i = 0; i < limit; i++) {
                    uint32_t elem_hash = hash_value(arr->elements[i]);
                    hash ^= elem_hash;
                    hash *= 16777619u;
                }
                hash ^= (uint32_t)arr->length; // Include length in hash
                return hash;
            }
            return 0;
        }
        case EMBER_VAL_SET:
        case EMBER_VAL_MAP:
        case EMBER_VAL_HASH_MAP: {
            // For collections, use object pointer but add type distinction
            if (value.as.obj_val) {
                uintptr_t ptr = (uintptr_t)value.as.obj_val;
                uint32_t hash = (uint32_t)(ptr ^ (ptr >> 32));
                hash ^= (uint32_t)value.type; // Mix in type
                return hash;
            }
            return 0;
        }
        default:
            // For other object types, use pointer with type mixing
            if (value.as.obj_val) {
                uintptr_t ptr = (uintptr_t)value.as.obj_val;
                uint32_t hash = (uint32_t)(ptr ^ (ptr >> 32));
                hash ^= (uint32_t)value.type << 24; // Mix in type in high bits
                return hash;
            }
            return (uint32_t)value.type;
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
        case EMBER_VAL_CLASS:
            // Classes are equal if they are the same object
            return a.as.obj_val == b.as.obj_val;
        case EMBER_VAL_INSTANCE:
            // Instances are equal if they are the same object
            return a.as.obj_val == b.as.obj_val;
        case EMBER_VAL_EXCEPTION:
            // Exceptions are equal if they are the same object
            return a.as.obj_val == b.as.obj_val;
        case EMBER_VAL_PROMISE:
            // Promises are equal if they are the same object
            return a.as.obj_val == b.as.obj_val;
        case EMBER_VAL_GENERATOR:
            // Generators are equal if they are the same object
            return a.as.obj_val == b.as.obj_val;
        case EMBER_VAL_ITERATOR:
            // Iterators are equal if they are the same object
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
            printf("<%s: %s>", 
                   exc->type_name ? exc->type_name : "Exception", 
                   exc->message ? exc->message : "");
            if (exc->file_name && exc->line_number > 0) {
                printf(" at %s:%d", exc->file_name, exc->line_number);
            }
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
        case EMBER_VAL_ITERATOR: {
            ember_iterator* iterator = AS_ITERATOR(value);
            printf("<Iterator [type: %d, index: %d]>", iterator->type, iterator->index);
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
        exc->type_name = malloc(type_len + 1);
        if (exc->type_name) {
            memcpy(exc->type_name, type, type_len + 1);
        }
    } else {
        exc->type_name = NULL;
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
    
    exc->line_number = 0; // Will be set by VM during throw
    exc->column_number = 0;
    exc->file_name = NULL;
    exc->stack_frames = NULL;
    exc->stack_frame_count = 0;
    exc->cause = ember_make_nil();
    exc->data = ember_make_nil();
    exc->suppressed_count = 0;
    exc->suppressed_exceptions = NULL;
    
    ember_value value;
    value.type = EMBER_VAL_EXCEPTION;
    value.as.obj_val = (ember_object*)exc;
    return value;
}

// Enhanced exception creation with detailed information
ember_value ember_make_exception_detailed(ember_vm* vm, ember_exception_type type, const char* message, 
                                         const char* file_name, int line_number, int column_number) {
    ember_exception* exc = (ember_exception*)allocate_object(vm, sizeof(ember_exception), OBJ_EXCEPTION);
    if (!exc) {
        return ember_make_nil();
    }
    
    // Set exception type
    exc->exception_type = type;
    exc->type_name = malloc(strlen(ember_exception_type_to_string(type)) + 1);
    if (exc->type_name) {
        strcpy(exc->type_name, ember_exception_type_to_string(type));
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
    
    // Set location information
    if (file_name) {
        size_t file_len = strlen(file_name);
        exc->file_name = malloc(file_len + 1);
        if (exc->file_name) {
            memcpy(exc->file_name, file_name, file_len + 1);
        }
    } else {
        exc->file_name = NULL;
    }
    
    exc->line_number = line_number;
    exc->column_number = column_number;
    
    // Initialize stack trace
    exc->stack_frames = NULL;
    exc->stack_frame_count = 0;
    
    // Initialize other fields
    exc->cause = ember_make_nil();
    exc->data = ember_make_nil();
    exc->timestamp = (uint64_t)time(NULL);
    exc->suppressed_count = 0;
    exc->suppressed_exceptions = NULL;
    
    // Capture current stack trace
    ember_capture_stack_trace(vm, exc);
    
    ember_value value;
    value.type = EMBER_VAL_EXCEPTION;
    value.as.obj_val = (ember_object*)exc;
    return value;
}

// Convenience functions for creating specific exception types
ember_value ember_make_type_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_TYPE_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_runtime_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_RUNTIME_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_syntax_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_SYNTAX_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_reference_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_REFERENCE_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_range_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_RANGE_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_memory_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_MEMORY_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_security_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_SECURITY_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_io_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_IO_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_network_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_NETWORK_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_timeout_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_TIMEOUT_ERROR, message, NULL, 0, 0);
}

ember_value ember_make_assertion_error(ember_vm* vm, const char* message) {
    return ember_make_exception_detailed(vm, EMBER_EXCEPTION_ASSERTION_ERROR, message, NULL, 0, 0);
}

// Convert exception type to string
const char* ember_exception_type_to_string(ember_exception_type type) {
    switch (type) {
        case EMBER_EXCEPTION_ERROR: return "Error";
        case EMBER_EXCEPTION_TYPE_ERROR: return "TypeError";
        case EMBER_EXCEPTION_RUNTIME_ERROR: return "RuntimeError";
        case EMBER_EXCEPTION_SYNTAX_ERROR: return "SyntaxError";
        case EMBER_EXCEPTION_REFERENCE_ERROR: return "ReferenceError";
        case EMBER_EXCEPTION_RANGE_ERROR: return "RangeError";
        case EMBER_EXCEPTION_MEMORY_ERROR: return "MemoryError";
        case EMBER_EXCEPTION_SECURITY_ERROR: return "SecurityError";
        case EMBER_EXCEPTION_IO_ERROR: return "IOError";
        case EMBER_EXCEPTION_NETWORK_ERROR: return "NetworkError";
        case EMBER_EXCEPTION_TIMEOUT_ERROR: return "TimeoutError";
        case EMBER_EXCEPTION_ASSERTION_ERROR: return "AssertionError";
        case EMBER_EXCEPTION_CUSTOM: return "CustomError";
        default: return "UnknownError";
    }
}

// Add a stack frame to an exception
void ember_exception_add_stack_frame(ember_vm* vm, ember_exception* exc, const char* function_name, 
                                     const char* file_name, int line_number, int column_number, 
                                     uint8_t* instruction_ptr) {
    (void)vm; // Currently unused
    if (!exc) return;
    
    // Reallocate stack frames array
    exc->stack_frames = realloc(exc->stack_frames, sizeof(ember_stack_frame) * (exc->stack_frame_count + 1));
    if (!exc->stack_frames) {
        fprintf(stderr, "[SECURITY] Memory allocation failed for stack frame\n");
        return;
    }
    
    ember_stack_frame* frame = &exc->stack_frames[exc->stack_frame_count];
    
    // Copy function name
    if (function_name) {
        size_t name_len = strlen(function_name);
        frame->function_name = malloc(name_len + 1);
        if (frame->function_name) {
            memcpy(frame->function_name, function_name, name_len + 1);
        }
    } else {
        frame->function_name = malloc(strlen("<script>") + 1);
        if (frame->function_name) {
            strcpy(frame->function_name, "<script>");
        }
    }
    
    // Copy file name
    if (file_name) {
        size_t file_len = strlen(file_name);
        frame->file_name = malloc(file_len + 1);
        if (frame->file_name) {
            memcpy(frame->file_name, file_name, file_len + 1);
        }
    } else {
        frame->file_name = malloc(strlen("<unknown>") + 1);
        if (frame->file_name) {
            strcpy(frame->file_name, "<unknown>");
        }
    }
    
    frame->line_number = line_number;
    frame->column_number = column_number;
    frame->instruction_ptr = instruction_ptr;
    frame->locals = ember_make_nil(); // TODO: Capture local variables
    
    exc->stack_frame_count++;
}

// Set the cause of an exception (for exception chaining)
void ember_exception_set_cause(ember_vm* vm, ember_exception* exc, ember_value cause) {
    (void)vm; // Currently unused
    if (!exc) return;
    exc->cause = cause;
}

// Add a suppressed exception
void ember_exception_add_suppressed(ember_vm* vm, ember_exception* exc, ember_value suppressed) {
    (void)vm; // Currently unused
    if (!exc) return;
    
    // Reallocate suppressed exceptions array
    exc->suppressed_exceptions = realloc(exc->suppressed_exceptions, 
                                        sizeof(ember_value) * (exc->suppressed_count + 1));
    if (!exc->suppressed_exceptions) {
        fprintf(stderr, "[SECURITY] Memory allocation failed for suppressed exception\n");
        return;
    }
    
    exc->suppressed_exceptions[exc->suppressed_count] = suppressed;
    exc->suppressed_count++;
}

// Capture the current stack trace
void ember_capture_stack_trace(ember_vm* vm, ember_exception* exc) {
    if (!vm || !exc) return;
    
    // Add current call stack frames
    for (int i = vm->call_stack_depth - 1; i >= 0; i--) {
        ember_call_frame* frame = &vm->call_stack[i];
        ember_exception_add_stack_frame(vm, exc, frame->function_name, 
                                       frame->location.filename,
                                       frame->location.line,
                                       frame->location.column,
                                       frame->instruction_pointer);
    }
}

// Get stack trace as a formatted string
ember_value ember_exception_get_stack_trace_string(ember_vm* vm, ember_exception* exc) {
    if (!exc || exc->stack_frame_count == 0) {
        return ember_make_string_gc(vm, "No stack trace available");
    }
    
    char* buffer = malloc(4096); // Initial buffer size
    if (!buffer) {
        return ember_make_string_gc(vm, "Memory error: cannot format stack trace");
    }
    
    size_t buffer_size = 4096;
    size_t offset = 0;
    
    for (int i = 0; i < exc->stack_frame_count; i++) {
        ember_stack_frame* frame = &exc->stack_frames[i];
        
        char line_buffer[512];
        snprintf(line_buffer, sizeof(line_buffer), "  at %s (%s:%d:%d)\n",
                frame->function_name ? frame->function_name : "<unknown>",
                frame->file_name ? frame->file_name : "<unknown>",
                frame->line_number,
                frame->column_number);
        
        size_t line_len = strlen(line_buffer);
        
        // Resize buffer if needed
        if (offset + line_len + 1 > buffer_size) {
            buffer_size *= 2;
            buffer = realloc(buffer, buffer_size);
            if (!buffer) {
                return ember_make_string_gc(vm, "Memory error: cannot format stack trace");
            }
        }
        
        memcpy(buffer + offset, line_buffer, line_len);
        offset += line_len;
    }
    
    buffer[offset] = '\0';
    ember_value result = ember_make_string_gc(vm, buffer);
    free(buffer);
    return result;
}

// Print detailed exception information
void ember_print_exception_details(ember_vm* vm, ember_exception* exc) {
    if (!exc) return;
    
    // Print exception type and message
    printf("%s: %s\n", 
           exc->type_name ? exc->type_name : "Exception",
           exc->message ? exc->message : "");
    
    // Print location if available
    if (exc->file_name && exc->line_number > 0) {
        printf("  at %s:%d:%d\n", exc->file_name, exc->line_number, exc->column_number);
    }
    
    // Print stack trace
    if (exc->stack_frame_count > 0) {
        printf("Stack trace:\n");
        for (int i = 0; i < exc->stack_frame_count; i++) {
            ember_stack_frame* frame = &exc->stack_frames[i];
            printf("  at %s (%s:%d:%d)\n",
                   frame->function_name ? frame->function_name : "<unknown>",
                   frame->file_name ? frame->file_name : "<unknown>",
                   frame->line_number,
                   frame->column_number);
        }
    }
    
    // Print cause if available
    if (exc->cause.type == EMBER_VAL_EXCEPTION) {
        printf("Caused by: ");
        ember_exception* cause = AS_EXCEPTION(exc->cause);
        ember_print_exception_details(vm, cause);
    }
    
    // Print suppressed exceptions
    for (int i = 0; i < exc->suppressed_count; i++) {
        if (exc->suppressed_exceptions[i].type == EMBER_VAL_EXCEPTION) {
            printf("Suppressed: ");
            ember_exception* suppressed = AS_EXCEPTION(exc->suppressed_exceptions[i]);
            ember_print_exception_details(vm, suppressed);
        }
    }
}

// Check if exception matches a specific type
int ember_exception_matches_type(ember_exception* exc, ember_exception_type type) {
    return exc && exc->exception_type == type;
}

// Wrap an existing exception with a new message
ember_value ember_wrap_exception(ember_vm* vm, ember_value original, const char* new_message) {
    ember_value wrapper = ember_make_exception_detailed(vm, EMBER_EXCEPTION_ERROR, new_message, NULL, 0, 0);
    if (wrapper.type == EMBER_VAL_EXCEPTION && original.type == EMBER_VAL_EXCEPTION) {
        ember_exception* wrapper_exc = AS_EXCEPTION(wrapper);
        ember_exception_set_cause(vm, wrapper_exc, original);
    }
    return wrapper;
}

// Get the root cause of an exception chain
ember_value ember_get_root_cause(ember_value exception) {
    if (exception.type != EMBER_VAL_EXCEPTION) {
        return exception;
    }
    
    ember_exception* exc = AS_EXCEPTION(exception);
    while (exc->cause.type == EMBER_VAL_EXCEPTION) {
        exc = AS_EXCEPTION(exc->cause);
    }
    
    ember_value result;
    result.type = EMBER_VAL_EXCEPTION;
    result.as.obj_val = (ember_object*)exc;
    return result;
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

// Enhanced Map methods for comprehensive functionality
ember_value map_keys(ember_vm* vm, ember_map* map) {
    if (!map || !vm) return ember_make_nil();
    
    ember_array* keys_array = (ember_array*)allocate_object(vm, sizeof(ember_array), OBJ_ARRAY);
    if (!keys_array) return ember_make_nil();
    
    keys_array->capacity = map->size > 0 ? map->size : 1;
    keys_array->length = 0;
    keys_array->elements = malloc(sizeof(ember_value) * keys_array->capacity);
    if (!keys_array->elements) {
        return ember_make_nil();
    }
    
    // Collect all keys
    for (int i = 0; i < map->entries->capacity; i++) {
        if (map->entries->entries[i].is_occupied) {
            keys_array->elements[keys_array->length++] = map->entries->entries[i].key;
        }
    }
    
    ember_value result;
    result.type = EMBER_VAL_ARRAY;
    result.as.obj_val = (ember_object*)keys_array;
    return result;
}

ember_value map_values(ember_vm* vm, ember_map* map) {
    if (!map || !vm) return ember_make_nil();
    
    ember_array* values_array = (ember_array*)allocate_object(vm, sizeof(ember_array), OBJ_ARRAY);
    if (!values_array) return ember_make_nil();
    
    values_array->capacity = map->size > 0 ? map->size : 1;
    values_array->length = 0;
    values_array->elements = malloc(sizeof(ember_value) * values_array->capacity);
    if (!values_array->elements) {
        return ember_make_nil();
    }
    
    // Collect all values
    for (int i = 0; i < map->entries->capacity; i++) {
        if (map->entries->entries[i].is_occupied) {
            values_array->elements[values_array->length++] = map->entries->entries[i].value;
        }
    }
    
    ember_value result;
    result.type = EMBER_VAL_ARRAY;
    result.as.obj_val = (ember_object*)values_array;
    return result;
}

ember_value map_entries(ember_vm* vm, ember_map* map) {
    if (!map || !vm) return ember_make_nil();
    
    ember_array* entries_array = (ember_array*)allocate_object(vm, sizeof(ember_array), OBJ_ARRAY);
    if (!entries_array) return ember_make_nil();
    
    entries_array->capacity = map->size > 0 ? map->size : 1;
    entries_array->length = 0;
    entries_array->elements = malloc(sizeof(ember_value) * entries_array->capacity);
    if (!entries_array->elements) {
        return ember_make_nil();
    }
    
    // Collect all key-value pairs as [key, value] arrays
    for (int i = 0; i < map->entries->capacity; i++) {
        if (map->entries->entries[i].is_occupied) {
            // Create [key, value] array for each entry
            ember_array* pair = (ember_array*)allocate_object(vm, sizeof(ember_array), OBJ_ARRAY);
            if (!pair) continue;
            
            pair->capacity = 2;
            pair->length = 2;
            pair->elements = malloc(sizeof(ember_value) * 2);
            if (!pair->elements) continue;
            
            pair->elements[0] = map->entries->entries[i].key;
            pair->elements[1] = map->entries->entries[i].value;
            
            ember_value pair_value;
            pair_value.type = EMBER_VAL_ARRAY;
            pair_value.as.obj_val = (ember_object*)pair;
            
            entries_array->elements[entries_array->length++] = pair_value;
        }
    }
    
    ember_value result;
    result.type = EMBER_VAL_ARRAY;
    result.as.obj_val = (ember_object*)entries_array;
    return result;
}

// Enhanced Set methods for set operations
ember_value set_to_array(ember_vm* vm, ember_set* set) {
    if (!set || !vm) return ember_make_nil();
    
    ember_array* array = (ember_array*)allocate_object(vm, sizeof(ember_array), OBJ_ARRAY);
    if (!array) return ember_make_nil();
    
    array->capacity = set->size > 0 ? set->size : 1;
    array->length = 0;
    array->elements = malloc(sizeof(ember_value) * array->capacity);
    if (!array->elements) {
        return ember_make_nil();
    }
    
    // Collect all elements
    for (int i = 0; i < set->elements->capacity; i++) {
        if (set->elements->entries[i].is_occupied) {
            array->elements[array->length++] = set->elements->entries[i].key;
        }
    }
    
    ember_value result;
    result.type = EMBER_VAL_ARRAY;
    result.as.obj_val = (ember_object*)array;
    return result;
}

ember_value set_union(ember_vm* vm, ember_set* set1, ember_set* set2) {
    if (!set1 || !set2 || !vm) return ember_make_nil();
    
    ember_set* result_set = allocate_set(vm);
    if (!result_set) return ember_make_nil();
    
    // Add all elements from set1
    for (int i = 0; i < set1->elements->capacity; i++) {
        if (set1->elements->entries[i].is_occupied) {
            set_add(result_set, set1->elements->entries[i].key);
        }
    }
    
    // Add all elements from set2
    for (int i = 0; i < set2->elements->capacity; i++) {
        if (set2->elements->entries[i].is_occupied) {
            set_add(result_set, set2->elements->entries[i].key);
        }
    }
    
    ember_value result;
    result.type = EMBER_VAL_SET;
    result.as.obj_val = (ember_object*)result_set;
    return result;
}

ember_value set_intersection(ember_vm* vm, ember_set* set1, ember_set* set2) {
    if (!set1 || !set2 || !vm) return ember_make_nil();
    
    ember_set* result_set = allocate_set(vm);
    if (!result_set) return ember_make_nil();
    
    // Add elements that exist in both sets
    for (int i = 0; i < set1->elements->capacity; i++) {
        if (set1->elements->entries[i].is_occupied) {
            ember_value elem = set1->elements->entries[i].key;
            if (set_has(set2, elem)) {
                set_add(result_set, elem);
            }
        }
    }
    
    ember_value result;
    result.type = EMBER_VAL_SET;
    result.as.obj_val = (ember_object*)result_set;
    return result;
}

ember_value set_difference(ember_vm* vm, ember_set* set1, ember_set* set2) {
    if (!set1 || !set2 || !vm) return ember_make_nil();
    
    ember_set* result_set = allocate_set(vm);
    if (!result_set) return ember_make_nil();
    
    // Add elements that exist in set1 but not in set2
    for (int i = 0; i < set1->elements->capacity; i++) {
        if (set1->elements->entries[i].is_occupied) {
            ember_value elem = set1->elements->entries[i].key;
            if (!set_has(set2, elem)) {
                set_add(result_set, elem);
            }
        }
    }
    
    ember_value result;
    result.type = EMBER_VAL_SET;
    result.as.obj_val = (ember_object*)result_set;
    return result;
}

int set_is_subset(ember_set* subset, ember_set* superset) {
    if (!subset || !superset) return 0;
    if (subset->size > superset->size) return 0;
    
    // Check if all elements of subset are in superset
    for (int i = 0; i < subset->elements->capacity; i++) {
        if (subset->elements->entries[i].is_occupied) {
            if (!set_has(superset, subset->elements->entries[i].key)) {
                return 0;
            }
        }
    }
    
    return 1;
}

// Array enhancement methods for functional programming

// Array.forEach(callback) - execute callback for each element
void array_foreach(ember_vm* vm, ember_array* array, ember_value callback) {
    if (!vm || !array || callback.type != EMBER_VAL_FUNCTION) return;
    
    for (int i = 0; i < array->length; i++) {
        // Call callback with (element, index, array)
        ember_value args[3];
        args[0] = array->elements[i];           // element
        args[1] = ember_make_number(i);         // index
        
        // Create array value for third parameter
        ember_value array_val;
        array_val.type = EMBER_VAL_ARRAY;
        array_val.as.obj_val = (ember_object*)array;
        args[2] = array_val;                    // array
        
        // Execute callback (simplified - would need proper function call implementation)
        if (callback.as.native_val) {
            callback.as.native_val(vm, 3, args);
        }
    }
}

// Array.map(callback) - create new array with transformed elements
ember_value array_map(ember_vm* vm, ember_array* array, ember_value callback) {
    if (!vm || !array || callback.type != EMBER_VAL_FUNCTION) {
        return ember_make_nil();
    }
    
    ember_array* result = (ember_array*)allocate_object(vm, sizeof(ember_array), OBJ_ARRAY);
    if (!result) return ember_make_nil();
    
    result->capacity = array->length > 0 ? array->length : 1;
    result->length = 0;
    result->elements = malloc(sizeof(ember_value) * result->capacity);
    if (!result->elements) return ember_make_nil();
    
    for (int i = 0; i < array->length; i++) {
        // Call callback with (element, index, array)
        ember_value args[3];
        args[0] = array->elements[i];           // element
        args[1] = ember_make_number(i);         // index
        
        ember_value array_val;
        array_val.type = EMBER_VAL_ARRAY;
        array_val.as.obj_val = (ember_object*)array;
        args[2] = array_val;                    // array
        
        // Execute callback and store result
        ember_value transformed = ember_make_nil();
        if (callback.as.native_val) {
            transformed = callback.as.native_val(vm, 3, args);
        }
        
        result->elements[result->length++] = transformed;
    }
    
    ember_value result_val;
    result_val.type = EMBER_VAL_ARRAY;
    result_val.as.obj_val = (ember_object*)result;
    return result_val;
}

// Array.filter(callback) - create new array with elements that pass test
ember_value array_filter(ember_vm* vm, ember_array* array, ember_value callback) {
    if (!vm || !array || callback.type != EMBER_VAL_FUNCTION) {
        return ember_make_nil();
    }
    
    ember_array* result = (ember_array*)allocate_object(vm, sizeof(ember_array), OBJ_ARRAY);
    if (!result) return ember_make_nil();
    
    result->capacity = array->length > 0 ? array->length : 1;
    result->length = 0;
    result->elements = malloc(sizeof(ember_value) * result->capacity);
    if (!result->elements) return ember_make_nil();
    
    for (int i = 0; i < array->length; i++) {
        // Call callback with (element, index, array)
        ember_value args[3];
        args[0] = array->elements[i];           // element
        args[1] = ember_make_number(i);         // index
        
        ember_value array_val;
        array_val.type = EMBER_VAL_ARRAY;
        array_val.as.obj_val = (ember_object*)array;
        args[2] = array_val;                    // array
        
        // Execute callback and check result
        ember_value test_result = ember_make_nil();
        if (callback.as.native_val) {
            test_result = callback.as.native_val(vm, 3, args);
        }
        
        // If result is truthy, include element
        if ((test_result.type == EMBER_VAL_BOOL && test_result.as.bool_val) ||
            (test_result.type == EMBER_VAL_NUMBER && test_result.as.number_val != 0.0) ||
            (test_result.type == EMBER_VAL_STRING && test_result.as.obj_val != NULL)) {
            result->elements[result->length++] = array->elements[i];
        }
    }
    
    ember_value result_val;
    result_val.type = EMBER_VAL_ARRAY;
    result_val.as.obj_val = (ember_object*)result;
    return result_val;
}

// Array.reduce(callback, initialValue) - reduce array to single value
ember_value array_reduce(ember_vm* vm, ember_array* array, ember_value callback, ember_value initial) {
    if (!vm || !array || callback.type != EMBER_VAL_FUNCTION) {
        return ember_make_nil();
    }
    
    if (array->length == 0) {
        return initial;
    }
    
    ember_value accumulator = initial;
    int start_index = 0;
    
    // If no initial value provided, use first element
    if (initial.type == EMBER_VAL_NIL && array->length > 0) {
        accumulator = array->elements[0];
        start_index = 1;
    }
    
    for (int i = start_index; i < array->length; i++) {
        // Call callback with (accumulator, currentValue, index, array)
        ember_value args[4];
        args[0] = accumulator;                  // accumulator
        args[1] = array->elements[i];           // currentValue
        args[2] = ember_make_number(i);         // index
        
        ember_value array_val;
        array_val.type = EMBER_VAL_ARRAY;
        array_val.as.obj_val = (ember_object*)array;
        args[3] = array_val;                    // array
        
        // Execute callback and update accumulator
        if (callback.as.native_val) {
            accumulator = callback.as.native_val(vm, 4, args);
        }
    }
    
    return accumulator;
}

// Array.find(callback) - find first element that passes test
ember_value array_find(ember_vm* vm, ember_array* array, ember_value callback) {
    if (!vm || !array || callback.type != EMBER_VAL_FUNCTION) {
        return ember_make_nil();
    }
    
    for (int i = 0; i < array->length; i++) {
        // Call callback with (element, index, array)
        ember_value args[3];
        args[0] = array->elements[i];           // element
        args[1] = ember_make_number(i);         // index
        
        ember_value array_val;
        array_val.type = EMBER_VAL_ARRAY;
        array_val.as.obj_val = (ember_object*)array;
        args[2] = array_val;                    // array
        
        // Execute callback and check result
        ember_value test_result = ember_make_nil();
        if (callback.as.native_val) {
            test_result = callback.as.native_val(vm, 3, args);
        }
        
        // If result is truthy, return element
        if ((test_result.type == EMBER_VAL_BOOL && test_result.as.bool_val) ||
            (test_result.type == EMBER_VAL_NUMBER && test_result.as.number_val != 0.0) ||
            (test_result.type == EMBER_VAL_STRING && test_result.as.obj_val != NULL)) {
            return array->elements[i];
        }
    }
    
    return ember_make_nil();
}

// Array.some(callback) - test if at least one element passes test
int array_some(ember_vm* vm, ember_array* array, ember_value callback) {
    if (!vm || !array || callback.type != EMBER_VAL_FUNCTION) {
        return 0;
    }
    
    for (int i = 0; i < array->length; i++) {
        // Call callback with (element, index, array)
        ember_value args[3];
        args[0] = array->elements[i];           // element
        args[1] = ember_make_number(i);         // index
        
        ember_value array_val;
        array_val.type = EMBER_VAL_ARRAY;
        array_val.as.obj_val = (ember_object*)array;
        args[2] = array_val;                    // array
        
        // Execute callback and check result
        ember_value test_result = ember_make_nil();
        if (callback.as.native_val) {
            test_result = callback.as.native_val(vm, 3, args);
        }
        
        // If result is truthy, return true
        if ((test_result.type == EMBER_VAL_BOOL && test_result.as.bool_val) ||
            (test_result.type == EMBER_VAL_NUMBER && test_result.as.number_val != 0.0) ||
            (test_result.type == EMBER_VAL_STRING && test_result.as.obj_val != NULL)) {
            return 1;
        }
    }
    
    return 0;
}

// Array.every(callback) - test if all elements pass test
int array_every(ember_vm* vm, ember_array* array, ember_value callback) {
    if (!vm || !array || callback.type != EMBER_VAL_FUNCTION) {
        return 0;
    }
    
    for (int i = 0; i < array->length; i++) {
        // Call callback with (element, index, array)
        ember_value args[3];
        args[0] = array->elements[i];           // element
        args[1] = ember_make_number(i);         // index
        
        ember_value array_val;
        array_val.type = EMBER_VAL_ARRAY;
        array_val.as.obj_val = (ember_object*)array;
        args[2] = array_val;                    // array
        
        // Execute callback and check result
        ember_value test_result = ember_make_nil();
        if (callback.as.native_val) {
            test_result = callback.as.native_val(vm, 3, args);
        }
        
        // If result is falsy, return false
        if (!((test_result.type == EMBER_VAL_BOOL && test_result.as.bool_val) ||
              (test_result.type == EMBER_VAL_NUMBER && test_result.as.number_val != 0.0) ||
              (test_result.type == EMBER_VAL_STRING && test_result.as.obj_val != NULL))) {
            return 0;
        }
    }
    
    return 1;
}

// Array.indexOf(searchElement) - find index of element
int array_index_of(ember_array* array, ember_value search_element) {
    if (!array) return -1;
    
    for (int i = 0; i < array->length; i++) {
        if (values_equal(array->elements[i], search_element)) {
            return i;
        }
    }
    
    return -1;
}

// Array.includes(searchElement) - check if element exists
int array_includes(ember_array* array, ember_value search_element) {
    return array_index_of(array, search_element) != -1;
}

// Iterator implementation for collections

// Create a new iterator for any collection
ember_value ember_make_iterator(ember_vm* vm, ember_value collection, ember_iterator_type type) {
    if (!vm) return ember_make_nil();
    
    ember_iterator* iterator = (ember_iterator*)allocate_object(vm, sizeof(ember_iterator), OBJ_ITERATOR);
    if (!iterator) return ember_make_nil();
    
    iterator->type = type;
    iterator->collection = collection;
    iterator->index = 0;
    
    // Set capacity and length based on collection type
    switch (collection.type) {
        case EMBER_VAL_ARRAY: {
            ember_array* array = AS_ARRAY(collection);
            iterator->capacity = array->capacity;
            iterator->length = array->length;
            break;
        }
        case EMBER_VAL_SET: {
            ember_set* set = AS_SET(collection);
            iterator->capacity = set->elements->capacity;
            iterator->length = set->size;
            break;
        }
        case EMBER_VAL_MAP: {
            ember_map* map = AS_MAP(collection);
            iterator->capacity = map->entries->capacity;
            iterator->length = map->size;
            break;
        }
        default:
            iterator->capacity = 0;
            iterator->length = 0;
            break;
    }
    
    ember_value result;
    result.type = EMBER_VAL_ITERATOR;
    result.as.obj_val = (ember_object*)iterator;
    return result;
}

// Get next value from iterator
ember_iterator_result iterator_next(ember_iterator* iterator) {
    ember_iterator_result result;
    result.value = ember_make_nil();
    result.done = 1;
    
    if (!iterator) return result;
    
    switch (iterator->type) {
        case ITERATOR_ARRAY: {
            if (iterator->collection.type != EMBER_VAL_ARRAY) break;
            ember_array* array = AS_ARRAY(iterator->collection);
            
            if (iterator->index < array->length) {
                result.value = array->elements[iterator->index];
                result.done = 0;
                iterator->index++;
            }
            break;
        }
        case ITERATOR_SET: {
            if (iterator->collection.type != EMBER_VAL_SET) break;
            ember_set* set = AS_SET(iterator->collection);
            
            // Find next occupied entry
            while (iterator->index < set->elements->capacity) {
                if (set->elements->entries[iterator->index].is_occupied) {
                    result.value = set->elements->entries[iterator->index].key;
                    result.done = 0;
                    iterator->index++;
                    return result;
                }
                iterator->index++;
            }
            break;
        }
        case ITERATOR_MAP_KEYS: {
            if (iterator->collection.type != EMBER_VAL_MAP) break;
            ember_map* map = AS_MAP(iterator->collection);
            
            // Find next occupied entry
            while (iterator->index < map->entries->capacity) {
                if (map->entries->entries[iterator->index].is_occupied) {
                    result.value = map->entries->entries[iterator->index].key;
                    result.done = 0;
                    iterator->index++;
                    return result;
                }
                iterator->index++;
            }
            break;
        }
        case ITERATOR_MAP_VALUES: {
            if (iterator->collection.type != EMBER_VAL_MAP) break;
            ember_map* map = AS_MAP(iterator->collection);
            
            // Find next occupied entry
            while (iterator->index < map->entries->capacity) {
                if (map->entries->entries[iterator->index].is_occupied) {
                    result.value = map->entries->entries[iterator->index].value;
                    result.done = 0;
                    iterator->index++;
                    return result;
                }
                iterator->index++;
            }
            break;
        }
        case ITERATOR_MAP_ENTRIES: {
            if (iterator->collection.type != EMBER_VAL_MAP) break;
            ember_map* map = AS_MAP(iterator->collection);
            
            // Find next occupied entry and return [key, value] array
            while (iterator->index < map->entries->capacity) {
                if (map->entries->entries[iterator->index].is_occupied) {
                    // Create [key, value] array
                    ember_array* entry_array = (ember_array*)malloc(sizeof(ember_array));
                    if (!entry_array) break;
                    
                    entry_array->capacity = 2;
                    entry_array->length = 2;
                    entry_array->elements = malloc(sizeof(ember_value) * 2);
                    if (!entry_array->elements) {
                        free(entry_array);
                        break;
                    }
                    
                    entry_array->elements[0] = map->entries->entries[iterator->index].key;
                    entry_array->elements[1] = map->entries->entries[iterator->index].value;
                    
                    // Note: This is simplified - in a real implementation, this array should be GC-managed
                    result.value.type = EMBER_VAL_ARRAY;
                    result.value.as.obj_val = (ember_object*)entry_array;
                    result.done = 0;
                    iterator->index++;
                    return result;
                }
                iterator->index++;
            }
            break;
        }
    }
    
    return result;
}

// Check if iterator is done
int iterator_done(ember_iterator* iterator) {
    if (!iterator) return 1;
    
    ember_iterator_result result = iterator_next(iterator);
    // Reset index to previous position since next() incremented it
    if (!result.done) {
        iterator->index--;
    }
    return result.done;
}

// Convenience functions for creating specific iterators

ember_value array_iterator(ember_vm* vm, ember_array* array) {
    ember_value array_val;
    array_val.type = EMBER_VAL_ARRAY;
    array_val.as.obj_val = (ember_object*)array;
    return ember_make_iterator(vm, array_val, ITERATOR_ARRAY);
}

ember_value set_iterator(ember_vm* vm, ember_set* set) {
    ember_value set_val;
    set_val.type = EMBER_VAL_SET;
    set_val.as.obj_val = (ember_object*)set;
    return ember_make_iterator(vm, set_val, ITERATOR_SET);
}

ember_value map_keys_iterator(ember_vm* vm, ember_map* map) {
    ember_value map_val;
    map_val.type = EMBER_VAL_MAP;
    map_val.as.obj_val = (ember_object*)map;
    return ember_make_iterator(vm, map_val, ITERATOR_MAP_KEYS);
}

ember_value map_values_iterator(ember_vm* vm, ember_map* map) {
    ember_value map_val;
    map_val.type = EMBER_VAL_MAP;
    map_val.as.obj_val = (ember_object*)map;
    return ember_make_iterator(vm, map_val, ITERATOR_MAP_VALUES);
}

ember_value map_entries_iterator(ember_vm* vm, ember_map* map) {
    ember_value map_val;
    map_val.type = EMBER_VAL_MAP;
    map_val.as.obj_val = (ember_object*)map;
    return ember_make_iterator(vm, map_val, ITERATOR_MAP_ENTRIES);
}

// Regex allocation and creation functions
