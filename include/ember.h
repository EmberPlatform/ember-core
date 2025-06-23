#ifndef EMBER_H
#define EMBER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Ember version (read from VERSION file during build)
#ifndef EMBER_VERSION_STRING
#define EMBER_VERSION "1.0.0"
#else
#define EMBER_VERSION EMBER_VERSION_STRING
#endif

// Maximum stack size for the VM
#define EMBER_STACK_MAX 256
#define EMBER_CONST_POOL_MAX 256
#define EMBER_MAX_LOCALS 256
#define EMBER_MAX_MODULES 64
#define EMBER_MAX_PATH_LEN 512
#define EMBER_MAX_MOUNTS 32
#define EMBER_MAX_ARGS 64
#define EMBER_MAX_GLOBALS 1024

// Error handling constants
#define EMBER_MAX_CALL_STACK 64
#define EMBER_MAX_ERROR_MESSAGE 512
#define EMBER_MAX_FILENAME 256
#define EMBER_MAX_EXCEPTION_HANDLERS 32
#define EMBER_MAX_FINALLY_BLOCKS 16

// VFS mount flags
#define EMBER_MOUNT_RW 0x01    // Read-write access
#define EMBER_MOUNT_RO 0x02    // Read-only access

// Standard library paths
#ifdef __APPLE__
    #define EMBER_SYSTEM_LIB_PATH "/usr/local/lib/ember"
    #define EMBER_USER_LIB_PATH "~/.local/lib/ember"
#elif __linux__
    #define EMBER_SYSTEM_LIB_PATH "/usr/lib/ember"
    #define EMBER_USER_LIB_PATH "~/.local/lib/ember"
#else
    #define EMBER_SYSTEM_LIB_PATH "./lib/ember"
    #define EMBER_USER_LIB_PATH "./lib/ember"
#endif

// Forward declarations for optimization support
typedef struct ember_vm ember_vm;
typedef struct ember_chunk ember_chunk;

// Performance optimization API
typedef struct ember_optimization_config ember_optimization_config_t;
typedef struct ember_optimization_stats ember_optimization_stats_t;
typedef struct ember_optimization_capabilities ember_optimization_capabilities_t;

// Optimization management functions
int ember_initialize_optimizations(ember_optimization_config_t* config);
void ember_shutdown_optimizations(void);
ember_vm* ember_create_optimized_vm(ember_optimization_config_t* config);
int ember_run_optimized(ember_vm* vm);
void ember_get_optimization_stats(ember_vm* vm, ember_optimization_stats_t* stats);
void ember_print_optimization_stats(ember_vm* vm);
ember_optimization_capabilities_t ember_get_optimization_capabilities(void);

// Value types
typedef enum {
    EMBER_VAL_NIL,
    EMBER_VAL_BOOL,
    EMBER_VAL_NUMBER,
    EMBER_VAL_STRING,
    EMBER_VAL_FUNCTION,
    EMBER_VAL_NATIVE,
    EMBER_VAL_ARRAY,
    EMBER_VAL_HASH_MAP,
    EMBER_VAL_EXCEPTION,
    EMBER_VAL_CLASS,
    EMBER_VAL_INSTANCE,
    EMBER_VAL_PROMISE,
    EMBER_VAL_GENERATOR,
    EMBER_VAL_SET,
    EMBER_VAL_MAP,
    EMBER_VAL_REGEX,
    EMBER_VAL_ITERATOR
} ember_val_type;

// Opcodes for the bytecode VM
typedef enum {
    OP_PUSH_CONST,    // Push constant onto stack
    OP_POP,           // Pop value from stack
    OP_ADD,           // Add top two values
    OP_SUB,           // Subtract top two values
    OP_MUL,           // Multiply top two values
    OP_DIV,           // Divide top two values
    OP_MOD,           // Modulo operation
    OP_EQUAL,         // Equal comparison
    OP_NOT_EQUAL,     // Not equal comparison
    OP_LESS,          // Less than comparison
    OP_LESS_EQUAL,    // Less than or equal comparison
    OP_GREATER,       // Greater than comparison
    OP_GREATER_EQUAL, // Greater than or equal comparison
    OP_JUMP,          // Unconditional jump
    OP_JUMP_IF_FALSE, // Conditional jump if top is false
    OP_LOOP,          // Loop back to earlier instruction
    OP_CALL,          // Call function
    OP_RETURN,        // Return from function
    OP_SET_LOCAL,     // Set local variable
    OP_GET_LOCAL,     // Get local variable
    OP_SET_GLOBAL,    // Set global variable
    OP_GET_GLOBAL,    // Get global variable
    OP_AND,           // Logical and
    OP_OR,            // Logical or
    OP_NOT,           // Logical not
    OP_ARRAY_NEW,     // Create new array
    OP_ARRAY_GET,     // Get array element
    OP_ARRAY_SET,     // Set array element
    OP_ARRAY_LEN,     // Get array length
    OP_HASH_MAP_NEW,  // Create new hash map
    OP_HASH_MAP_GET,  // Get hash map element
    OP_HASH_MAP_SET,  // Set hash map element
    OP_HASH_MAP_LEN,  // Get hash map length
    OP_STRING_INTERPOLATE, // String interpolation operation
    OP_BREAK,         // Break out of loop
    OP_CONTINUE,      // Continue to next loop iteration
    OP_TRY_BEGIN,     // Begin try block
    OP_TRY_END,       // End try block
    OP_CATCH_BEGIN,   // Begin catch block
    OP_CATCH_END,     // End catch block
    OP_FINALLY_BEGIN, // Begin finally block
    OP_FINALLY_END,   // End finally block
    OP_THROW,         // Throw exception
    OP_RETHROW,       // Rethrow current exception
    OP_POP_HANDLER,   // Pop exception handler
    OP_CATCH_TYPE,    // Begin typed catch block (with exception type checking)
    OP_EXCEPTION_MATCH, // Check if current exception matches specified type
    OP_CLASS_DEF,     // Define a class
    OP_METHOD_DEF,    // Define a method
    OP_INSTANCE_NEW,  // Create new instance
    OP_GET_PROPERTY,  // Get object property
    OP_SET_PROPERTY,  // Set object property
    OP_INVOKE,        // Invoke method
    OP_INHERIT,       // Inherit from superclass
    OP_GET_SUPER,     // Get superclass method
    OP_PROMISE_NEW,   // Create new promise
    OP_PROMISE_RESOLVE, // Resolve promise
    OP_PROMISE_REJECT, // Reject promise
    OP_AWAIT,         // Await promise resolution
    OP_YIELD,         // Yield from generator
    OP_GENERATOR_NEW, // Create new generator
    OP_GENERATOR_NEXT, // Get next value from generator
    OP_SET_NEW,       // Create new set
    OP_SET_ADD,       // Add element to set
    OP_SET_HAS,       // Check if set has element
    OP_SET_DELETE,    // Delete element from set
    OP_SET_SIZE,      // Get set size
    OP_SET_CLEAR,     // Clear all elements from set
    OP_MAP_NEW,       // Create new map
    OP_MAP_SET,       // Set map key-value pair
    OP_MAP_GET,       // Get map value by key
    OP_MAP_HAS,       // Check if map has key
    OP_MAP_DELETE,    // Delete key from map
    OP_MAP_SIZE,      // Get map size
    OP_MAP_CLEAR,     // Clear all key-value pairs from map
    OP_REGEX_NEW,     // Create new regex from pattern
    OP_REGEX_TEST,    // Test if string matches regex
    OP_REGEX_MATCH,   // Get match details from regex
    OP_REGEX_REPLACE, // Replace matches in string
    OP_REGEX_SPLIT,   // Split string by regex
    OP_SWITCH,        // Switch statement
    OP_CASE,          // Case clause
    OP_DEFAULT,       // Default clause
    // Module system opcodes
    OP_MODULE_INIT,   // Initialize module context
    OP_MODULE_EXPORT, // Export value from module
    OP_MODULE_EXPORT_DEFAULT, // Export default value
    OP_MODULE_IMPORT, // Import from module
    OP_MODULE_IMPORT_ALL, // Import all exports
    OP_MODULE_REQUIRE, // Require module (CommonJS style)
    OP_HALT           // Stop execution
} ember_opcode;

// Token types for lexer
typedef enum {
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_INTERPOLATED_STRING,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_MODULO,
    TOKEN_PLUS_PLUS,      // ++
    TOKEN_MINUS_MINUS,    // --
    TOKEN_PLUS_EQUAL,     // +=
    TOKEN_MINUS_EQUAL,    // -=
    TOKEN_MULTIPLY_EQUAL, // *=
    TOKEN_DIVIDE_EQUAL,   // /=
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_DO,
    TOKEN_FOR,
    TOKEN_FN,
    TOKEN_FUNCTION,
    TOKEN_RETURN,
    TOKEN_IMPORT,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_COMMA,
    TOKEN_COLON,
    TOKEN_AT,
    TOKEN_AND,          // "and" keyword
    TOKEN_OR,           // "or" keyword  
    TOKEN_NOT,          // "not" keyword or "!" operator
    TOKEN_AND_AND,      // "&&" operator
    TOKEN_OR_OR,        // "||" operator
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_IDENTIFIER,
    TOKEN_SEMICOLON,
    TOKEN_NEWLINE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_TRY,
    TOKEN_CATCH,
    TOKEN_FINALLY,
    TOKEN_THROW,
    TOKEN_CLASS,
    TOKEN_EXTENDS,
    TOKEN_NEW,
    TOKEN_THIS,
    TOKEN_SUPER,
    TOKEN_DOT,
    TOKEN_ASYNC,
    TOKEN_AWAIT,
    TOKEN_YIELD,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    // Module system tokens
    TOKEN_EXPORT,
    TOKEN_FROM,
    TOKEN_AS,
    TOKEN_REQUIRE,
    TOKEN_EOF,
    TOKEN_ERROR
} ember_token_type;

// Token structure
typedef struct {
    ember_token_type type;
    const char* start;
    int length;
    int line;
    double number; // For number tokens
} ember_token;

// Forward declarations
typedef struct ember_vm ember_vm;
typedef struct ember_value ember_value;
typedef struct ember_chunk ember_chunk;
typedef struct ember_object ember_object;
typedef struct ember_module ember_module;
typedef struct ember_mount_point ember_mount_point;

// Object types for garbage collection
typedef enum {
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_HASH_MAP,
    OBJ_EXCEPTION,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_METHOD,
    OBJ_PROMISE,
    OBJ_GENERATOR,
    OBJ_SET,
    OBJ_MAP,
    OBJ_REGEX,
    OBJ_ITERATOR
} ember_object_type;

// Base object structure for GC
struct ember_object {
    ember_object_type type;
    int is_marked;
    struct ember_object* next;
};

// String object structure
typedef struct {
    ember_object obj;
    char* chars;
    int length;
} ember_string;

// Array object structure
typedef struct {
    ember_object obj;
    ember_value* elements;
    int length;
    int capacity;
} ember_array;

// Ember value structure
struct ember_value {
    ember_val_type type;
    union {
        int bool_val;
        double number_val;
        char* string_val; // Legacy string support
        ember_object* obj_val;
        struct {
            ember_chunk* chunk;
            char* name;
        } func_val;
        struct ember_value (*native_val)(ember_vm* vm, int argc, struct ember_value* argv);
    } as;
};

// Hash map entry structure
typedef struct {
    ember_value key;
    ember_value value;
    int is_occupied;
} ember_hash_entry;

// Hash map object structure
typedef struct {
    ember_object obj;
    ember_hash_entry* entries;
    int length;     // Number of occupied entries
    int capacity;   // Total capacity
} ember_hash_map;

// Exception types for built-in exceptions
typedef enum {
    EMBER_EXCEPTION_ERROR,           // Generic Error
    EMBER_EXCEPTION_TYPE_ERROR,      // TypeError
    EMBER_EXCEPTION_RUNTIME_ERROR,   // RuntimeError
    EMBER_EXCEPTION_SYNTAX_ERROR,    // SyntaxError
    EMBER_EXCEPTION_REFERENCE_ERROR, // ReferenceError
    EMBER_EXCEPTION_RANGE_ERROR,     // RangeError
    EMBER_EXCEPTION_MEMORY_ERROR,    // MemoryError
    EMBER_EXCEPTION_SECURITY_ERROR,  // SecurityError
    EMBER_EXCEPTION_IO_ERROR,        // IOError
    EMBER_EXCEPTION_NETWORK_ERROR,   // NetworkError
    EMBER_EXCEPTION_TIMEOUT_ERROR,   // TimeoutError
    EMBER_EXCEPTION_ASSERTION_ERROR, // AssertionError
    EMBER_EXCEPTION_CUSTOM          // Custom user-defined exception
} ember_exception_type;

// Stack trace frame structure
typedef struct {
    char* function_name;      // Function name or "<script>" for top-level
    char* file_name;          // Source file name
    int line_number;          // Line number in source
    int column_number;        // Column number in source
    uint8_t* instruction_ptr; // VM instruction pointer
    ember_value locals;       // Local variables at the time of exception (for debugging)
} ember_stack_frame;

// Enhanced exception object structure
typedef struct {
    ember_object obj;
    ember_exception_type exception_type;  // Type of exception
    char* message;                        // Error message
    char* type_name;                      // String representation of type
    char* file_name;                      // Source file where exception occurred
    int line_number;                      // Line number where exception occurred
    int column_number;                    // Column number where exception occurred
    ember_stack_frame* stack_frames;      // Stack trace frames
    int stack_frame_count;                // Number of stack frames
    ember_value cause;                    // Cause of this exception (nested exceptions)
    ember_value data;                     // Additional data attached to exception
    uint64_t timestamp;                   // When the exception was created
    int suppressed_count;                 // Number of suppressed exceptions
    ember_value* suppressed_exceptions;   // Array of suppressed exceptions
} ember_exception;

// Class object structure
typedef struct {
    ember_object obj;
    ember_string* name;                    // Class name
    ember_hash_map* methods;               // Method table
    struct ember_class* superclass;       // Parent class (NULL for root classes)
} ember_class;

// Instance object structure  
typedef struct {
    ember_object obj;
    ember_class* klass;                    // Class definition
    ember_hash_map* fields;                // Instance fields
} ember_instance;

// Method object structure (bound method)
typedef struct {
    ember_object obj;
    ember_value receiver;                  // The instance this method is bound to
    ember_value method;                    // The function value
} ember_bound_method;

// Promise states
typedef enum {
    PROMISE_PENDING,
    PROMISE_RESOLVED,
    PROMISE_REJECTED
} ember_promise_state;

// Promise object structure
typedef struct {
    ember_object obj;
    ember_promise_state state;
    ember_value value;                     // Result value or rejection reason
    ember_array* then_callbacks;          // Array of then callbacks
    ember_array* catch_callbacks;         // Array of catch callbacks
    ember_array* finally_callbacks;       // Array of finally callbacks
} ember_promise;

// Generator states
typedef enum {
    GENERATOR_CREATED,
    GENERATOR_SUSPENDED,
    GENERATOR_COMPLETED
} ember_generator_state;

// Generator object structure
typedef struct {
    ember_object obj;
    ember_generator_state state;
    ember_chunk* chunk;                    // Function bytecode
    uint8_t* ip;                          // Current instruction pointer
    ember_value* locals;                   // Local variable state
    int local_count;                       // Number of locals
    ember_value* stack;                    // Generator stack state
    int stack_top;                         // Stack pointer
    ember_value yielded_value;             // Last yielded value
} ember_generator;

// Set object structure
typedef struct {
    ember_object obj;
    ember_hash_map* elements;              // Internal hash map for element storage
    int size;                              // Number of elements in set
} ember_set;

// Map object structure
typedef struct {
    ember_object obj;
    ember_hash_map* entries;               // Internal hash map for key-value storage
    int size;                              // Number of key-value pairs in map
} ember_map;

// Regex flags
typedef enum {
    REGEX_NONE = 0,
    REGEX_CASE_INSENSITIVE = 1,
    REGEX_MULTILINE = 2,
    REGEX_GLOBAL = 4,
    REGEX_DOTALL = 8
} ember_regex_flags;

// Regex match result
typedef struct {
    int start;                             // Start position of match
    int end;                               // End position of match
    char* match;                           // Matched text
} ember_regex_match;

// Regex object structure
typedef struct {
    ember_object obj;
    char* pattern;                         // Original regex pattern
    ember_regex_flags flags;               // Regex flags
    void* compiled_regex;                  // Compiled regex (implementation specific)
    ember_array* groups;                   // Array of capture groups from last match
    int last_index;                        // Last match position (for global matching)
} ember_regex;

// Iterator types for different collections
typedef enum {
    ITERATOR_ARRAY,
    ITERATOR_SET,
    ITERATOR_MAP_KEYS,
    ITERATOR_MAP_VALUES,
    ITERATOR_MAP_ENTRIES
} ember_iterator_type;

// Iterator result structure
typedef struct {
    ember_value value;                     // Current value
    int done;                              // Whether iteration is complete
} ember_iterator_result;

// Iterator object structure
typedef struct {
    ember_object obj;
    ember_iterator_type type;              // Type of iterator
    ember_value collection;                // Collection being iterated
    int index;                             // Current position
    int capacity;                          // Collection capacity (for optimization)
    int length;                            // Collection length
} ember_iterator;

// Exception handler structure for try/catch/finally
typedef struct {
    uint8_t* try_start;         // Start of try block
    uint8_t* try_end;           // End of try block  
    uint8_t* catch_start;       // Start of catch block (NULL if no catch)
    uint8_t* finally_start;     // Start of finally block (NULL if no finally)
    uint8_t* finally_end;       // End of finally block
    int stack_depth;            // Stack depth when handler was installed
    int local_count;            // Local variable count when handler was installed
    int is_active;              // Whether this handler is currently active
} ember_exception_handler;

// Finally block tracking structure
typedef struct {
    uint8_t* finally_start;     // Start of finally block
    uint8_t* finally_end;       // End of finally block
    int stack_depth;            // Stack depth to restore after finally
    int is_exception;           // Whether we're in finally due to exception
} ember_finally_block;

// Standardized result codes
typedef enum {
    EMBER_OK = 0,
    EMBER_ERR_COMPILE = -1,
    EMBER_ERR_RUNTIME = -2,
    EMBER_ERR_TYPE = -3,
    EMBER_ERR_MEMORY = -4,
    EMBER_ERR_SECURITY = -5,
    EMBER_ERR_SYSTEM = -6,
    EMBER_ERR_INTERNAL = -7
} ember_result_code;

// VM operation result codes
typedef enum {
    VM_RESULT_OK = 0,
    VM_RESULT_ERROR = -1,
    VM_RESULT_CONTINUE = 1
} vm_operation_result;

// Error types for categorizing errors
typedef enum {
    EMBER_ERROR_SYNTAX,      // Parse-time syntax errors
    EMBER_ERROR_RUNTIME,     // Runtime execution errors
    EMBER_ERROR_TYPE,        // Type mismatch errors
    EMBER_ERROR_MEMORY,      // Memory allocation errors
    EMBER_ERROR_SECURITY,    // Security violations
    EMBER_ERROR_IMPORT,      // Module/import errors
    EMBER_ERROR_SYSTEM       // System/OS-level errors
} ember_error_type;

// Source location information
typedef struct ember_source_location {
    const char* filename;    // Source file name (may be NULL for REPL/eval)
    int line;               // Line number (1-based)
    int column;             // Column position (1-based)
    const char* line_text;  // Full text of the line containing the error
} ember_source_location;

// Call stack frame for runtime errors
typedef struct ember_call_frame {
    const char* function_name;  // Function name (or "<script>" for top-level)
    ember_source_location location;
    uint8_t* instruction_pointer; // VM instruction pointer at time of call
} ember_call_frame;

// Comprehensive error information
typedef struct ember_error {
    ember_error_type type;
    char message[EMBER_MAX_ERROR_MESSAGE];
    ember_source_location location;
    
    // Call stack for runtime errors
    ember_call_frame call_stack[EMBER_MAX_CALL_STACK];
    int call_stack_depth;
    
    // Additional context
    const char* source_code;     // Full source code (for context display)
    const char* expected_token;  // For syntax errors - what was expected
    const char* actual_token;    // For syntax errors - what was found
    
    // VM state at error time (for runtime errors)
    int stack_size;
    int instruction_offset;      // Offset into current chunk
} ember_error;

// Function pointer type for native functions
typedef ember_value (*ember_native_func)(ember_vm* vm, int argc, ember_value* argv);

// Bytecode chunk structure
struct ember_chunk {
    uint8_t* code;
    int capacity;
    int count;
    ember_value* constants;
    int const_capacity;
    int const_count;
};

// Module structure for library loading
struct ember_module {
    char* name;
    char* path;
    ember_chunk* chunk;
    int is_loaded;
    struct {
        char* key;
        ember_value value;
    } exports[EMBER_MAX_LOCALS];
    int export_count;
};

// Virtual filesystem mount point structure
struct ember_mount_point {
    char* virtual_path;      // Virtual path in Ember (e.g., "/app")
    char* host_path;         // Real filesystem path (e.g., "/home/user/project")
    int flags;               // EMBER_MOUNT_RW or EMBER_MOUNT_RO
};

// Runtime loop context for proper break/continue handling
typedef struct {
    uint8_t* loop_start;     // Start of loop (for continue)
    uint8_t* loop_end;       // End of loop (for break) - set when loop completes
    int stack_depth;         // Stack depth when loop started (for cleanup)
    int local_count;         // Local variable count when loop started
} ember_runtime_loop_context;

// Ember VM structure
struct ember_vm {
    ember_chunk* chunk;
    uint8_t* ip; // Instruction pointer
    ember_value stack[EMBER_STACK_MAX];
    int stack_top;
    ember_value locals[EMBER_MAX_LOCALS];
    int local_count;
    struct {
        char* key;
        ember_value value;
    } globals[EMBER_MAX_LOCALS];
    int global_count;
    
    // Module system
    ember_module modules[EMBER_MAX_MODULES];
    int module_count;
    char* module_paths[8];
    int module_path_count;
    
    // Garbage collection
    ember_object* objects;
    int bytes_allocated;
    int next_gc;
    
    // Function chunk tracking
    ember_chunk* function_chunks[EMBER_MAX_LOCALS];
    int function_chunk_count;
    
    // Virtual filesystem for security
    struct ember_mount_point* mounts;
    int mount_count;
    int mount_capacity;
    
    // Exception handling
    ember_exception_handler exception_handlers[EMBER_MAX_EXCEPTION_HANDLERS];
    int exception_handler_count;
    ember_finally_block finally_blocks[EMBER_MAX_FINALLY_BLOCKS];
    int finally_block_count;
    ember_value current_exception;  // Currently thrown exception
    int exception_pending;          // Whether an exception is being propagated
    
    // Enhanced error tracking and call stack
    ember_call_frame call_stack[EMBER_MAX_CALL_STACK];
    int call_stack_depth;
    ember_error* current_error;     // Current error (if any)
    
    // String interning table
    ember_hash_map* string_intern_table;  // Hash table for string interning
    
    // Startup optimization flags
    int lazy_stdlib_loading;        // Whether to lazily load standard library
    int stdlib_initialized;        // Whether standard library has been loaded
    void* bytecode_cache;  // Bytecode cache for faster execution (placeholder)
    
    // Loop context tracking for break/continue
    ember_runtime_loop_context loop_contexts[8];  // Stack of nested loop contexts (max 8 levels)
    int loop_depth;                                // Current loop nesting depth
    
    // Debugging support
    void* debug_hooks;              // Debug hooks structure (ember_debug_hooks*)
    int debug_enabled;              // Whether debugging is enabled
    
    // Memory management context
    void* memory_context;           // VM memory management context (vm_memory_context*)
    
    // Async/await support
    ember_array* pending_promises;   // Array of promises waiting to be resolved
    ember_value* async_stack;        // Stack for async operation context
    int async_stack_top;             // Async stack pointer
    int is_async_context;            // Whether we're in an async function context
    
    // Generator support
    ember_generator* current_generator; // Currently executing generator (if any)
    
    // Performance optimization support
    void* jit_integration;              // JIT compiler integration (jit_vm_integration_t*)
    bool jit_enabled;                   // Whether JIT compilation is enabled
    void* arena_allocator;              // Arena allocator for memory optimization
    bool memory_optimized;              // Whether memory optimization is enabled
    void* vm_pool_context;              // VM pool context for concurrent execution
    bool vm_pool_enabled;               // Whether VM pool is enabled
    
    // Performance statistics
    uint64_t instructions_executed;     // Total instructions executed
    uint64_t function_calls;            // Total function calls made
    uint64_t jit_compilations;          // Number of JIT compilations triggered
    uint64_t memory_allocations;        // Number of memory allocations
    uint64_t gc_collections;            // Number of garbage collections
};

// API functions
ember_vm* ember_new_vm(void);
void ember_free_vm(ember_vm* vm);
void ember_register_func(ember_vm* vm, const char* name, ember_native_func func);
int ember_eval(ember_vm* vm, const char* source);
int ember_call(ember_vm* vm, const char* func_name, int argc, ember_value* argv);
ember_value ember_make_number(double num);
ember_value ember_make_bool(int b);
ember_value ember_make_string(const char* str);
ember_value ember_make_string_gc(ember_vm* vm, const char* str);
ember_value ember_make_array(ember_vm* vm, int capacity);
ember_value ember_make_hash_map(ember_vm* vm, int capacity);
ember_value ember_make_exception(ember_vm* vm, const char* type, const char* message);

// Enhanced exception creation functions
ember_value ember_make_exception_detailed(ember_vm* vm, ember_exception_type type, const char* message, 
                                         const char* file_name, int line_number, int column_number);
ember_value ember_make_type_error(ember_vm* vm, const char* message);
ember_value ember_make_runtime_error(ember_vm* vm, const char* message);
ember_value ember_make_syntax_error(ember_vm* vm, const char* message);
ember_value ember_make_reference_error(ember_vm* vm, const char* message);
ember_value ember_make_range_error(ember_vm* vm, const char* message);
ember_value ember_make_memory_error(ember_vm* vm, const char* message);
ember_value ember_make_security_error(ember_vm* vm, const char* message);
ember_value ember_make_io_error(ember_vm* vm, const char* message);
ember_value ember_make_network_error(ember_vm* vm, const char* message);
ember_value ember_make_timeout_error(ember_vm* vm, const char* message);
ember_value ember_make_assertion_error(ember_vm* vm, const char* message);

// Exception manipulation functions
void ember_exception_add_stack_frame(ember_vm* vm, ember_exception* exc, const char* function_name, 
                                     const char* file_name, int line_number, int column_number, 
                                     uint8_t* instruction_ptr);
void ember_exception_set_cause(ember_vm* vm, ember_exception* exc, ember_value cause);
void ember_exception_add_suppressed(ember_vm* vm, ember_exception* exc, ember_value suppressed);
ember_value ember_exception_get_stack_trace_string(ember_vm* vm, ember_exception* exc);
const char* ember_exception_type_to_string(ember_exception_type type);

// Exception stack trace utilities
void ember_capture_stack_trace(ember_vm* vm, ember_exception* exc);
void ember_print_exception_details(ember_vm* vm, ember_exception* exc);
int ember_exception_matches_type(ember_exception* exc, ember_exception_type type);

// Exception chaining utilities (for nested exceptions)
ember_value ember_wrap_exception(ember_vm* vm, ember_value original, const char* new_message);
ember_value ember_get_root_cause(ember_value exception);
ember_value ember_make_class(ember_vm* vm, const char* name);
ember_value ember_make_instance(ember_vm* vm, ember_class* klass);
ember_value ember_make_bound_method(ember_vm* vm, ember_value receiver, ember_value method);
ember_value ember_make_promise(ember_vm* vm);
ember_value ember_make_generator(ember_vm* vm, ember_chunk* chunk);
ember_value ember_make_set(ember_vm* vm);
ember_value ember_make_map(ember_vm* vm);
ember_value ember_make_regex(ember_vm* vm, const char* pattern, ember_regex_flags flags);
ember_value ember_make_nil(void);

// Utility functions
const char* value_type_to_string(ember_val_type type);

// Set operations
int set_add(ember_set* set, ember_value element);
int set_has(ember_set* set, ember_value element);
int set_delete(ember_set* set, ember_value element);
void set_clear(ember_set* set);

// Enhanced Set operations
ember_value set_to_array(ember_vm* vm, ember_set* set);
ember_value set_union(ember_vm* vm, ember_set* set1, ember_set* set2);
ember_value set_intersection(ember_vm* vm, ember_set* set1, ember_set* set2);
ember_value set_difference(ember_vm* vm, ember_set* set1, ember_set* set2);
int set_is_subset(ember_set* subset, ember_set* superset);

// Map operations  
int map_set(ember_map* map, ember_value key, ember_value value);
ember_value map_get(ember_map* map, ember_value key);
int map_has(ember_map* map, ember_value key);
int map_delete(ember_map* map, ember_value key);
void map_clear(ember_map* map);

// Enhanced Map operations
ember_value map_keys(ember_vm* vm, ember_map* map);
ember_value map_values(ember_vm* vm, ember_map* map);
ember_value map_entries(ember_vm* vm, ember_map* map);

// Regex operations
int regex_test(ember_regex* regex, const char* text);
ember_array* regex_match(ember_vm* vm, ember_regex* regex, const char* text);
ember_value regex_replace(ember_vm* vm, ember_regex* regex, const char* text, const char* replacement);
ember_array* regex_split(ember_vm* vm, ember_regex* regex, const char* text);

// VM collection operation handlers
vm_operation_result vm_handle_set_new(ember_vm* vm);
vm_operation_result vm_handle_set_add(ember_vm* vm);
vm_operation_result vm_handle_set_has(ember_vm* vm);
vm_operation_result vm_handle_set_delete(ember_vm* vm);
vm_operation_result vm_handle_set_size(ember_vm* vm);
vm_operation_result vm_handle_set_clear(ember_vm* vm);
vm_operation_result vm_handle_map_new(ember_vm* vm);
vm_operation_result vm_handle_map_set(ember_vm* vm);
vm_operation_result vm_handle_map_get(ember_vm* vm);
vm_operation_result vm_handle_map_has(ember_vm* vm);
vm_operation_result vm_handle_map_delete(ember_vm* vm);
vm_operation_result vm_handle_map_size(ember_vm* vm);
vm_operation_result vm_handle_map_clear(ember_vm* vm);

// VM regex operation handlers
vm_operation_result vm_handle_regex_new(ember_vm* vm);
vm_operation_result vm_handle_regex_test(ember_vm* vm);
vm_operation_result vm_handle_regex_match(ember_vm* vm);
vm_operation_result vm_handle_regex_replace(ember_vm* vm);
vm_operation_result vm_handle_regex_split(ember_vm* vm);
ember_value ember_peek_stack_top(ember_vm* vm);
void ember_print_value(ember_value value);
void print_value(ember_value value);
int values_equal(ember_value a, ember_value b);

// Array operation functions needed by stdlib
void array_push(ember_array* array, ember_value value);

// Enhanced Array methods for functional programming
void array_foreach(ember_vm* vm, ember_array* array, ember_value callback);
ember_value array_map(ember_vm* vm, ember_array* array, ember_value callback);
ember_value array_filter(ember_vm* vm, ember_array* array, ember_value callback);
ember_value array_reduce(ember_vm* vm, ember_array* array, ember_value callback, ember_value initial);
ember_value array_find(ember_vm* vm, ember_array* array, ember_value callback);
int array_some(ember_vm* vm, ember_array* array, ember_value callback);
int array_every(ember_vm* vm, ember_array* array, ember_value callback);
int array_index_of(ember_array* array, ember_value search_element);
int array_includes(ember_array* array, ember_value search_element);

// Iterator protocol functions
ember_value ember_make_iterator(ember_vm* vm, ember_value collection, ember_iterator_type type);
ember_iterator_result iterator_next(ember_iterator* iterator);
int iterator_done(ember_iterator* iterator);
ember_value array_iterator(ember_vm* vm, ember_array* array);
ember_value set_iterator(ember_vm* vm, ember_set* set);
ember_value map_keys_iterator(ember_vm* vm, ember_map* map);
ember_value map_values_iterator(ember_vm* vm, ember_map* map);
ember_value map_entries_iterator(ember_vm* vm, ember_map* map);

// Hash map operation functions needed by stdlib
ember_hash_map* allocate_hash_map(ember_vm* vm, int capacity);
void hash_map_set(ember_hash_map* map, ember_value key, ember_value value);
void hash_map_set_with_vm(ember_vm* vm, ember_hash_map* map, ember_value key, ember_value value);
ember_value hash_map_get(ember_hash_map* map, ember_value key);

// Performance optimization API
void vm_set_optimization_level(int level); // 0=none, 1=basic, 2=advanced, 3=max
void vm_print_performance_stats(void);

// Enhanced Garbage Collection API
void ember_gc_configure(ember_vm* vm, int enable_generational, int enable_incremental, 
                        int enable_write_barriers, int enable_object_pooling);
void ember_gc_print_statistics(ember_vm* vm);
void ember_gc_collect(ember_vm* vm);

// Startup optimization and performance profiling API
typedef struct {
    double vm_creation_time;      // Time to create VM structure
    double stdlib_init_time;      // Time to initialize standard library
    double parser_init_time;      // Time to initialize parser
    double vfs_init_time;         // Time to initialize VFS
    double gc_init_time;          // Time to initialize GC
    double total_startup_time;    // Total startup time
} ember_startup_profile;

ember_vm* ember_new_vm_optimized(int lazy_stdlib);  // VM with optional lazy stdlib loading
void ember_get_startup_profile(ember_startup_profile* profile);
void ember_print_startup_profile(void);
void ember_enable_lazy_loading(ember_vm* vm, int enable);

// Bytecode caching API (placeholders)
void ember_set_bytecode_cache_dir(const char* cache_dir);

// Module/Library API functions
int ember_import_module(ember_vm* vm, const char* module_name);
int ember_install_library(const char* library_name, const char* source_path);
char* ember_resolve_module_path(const char* module_name, const char* current_file);
void ember_add_module_path(ember_vm* vm, const char* path);

// Virtual filesystem API functions
int ember_vfs_mount(ember_vm* vm, const char* virtual_path, const char* host_path, int flags);
int ember_vfs_unmount(ember_vm* vm, const char* virtual_path);
char* ember_vfs_resolve(ember_vm* vm, const char* virtual_path);
int ember_vfs_check_access(ember_vm* vm, const char* virtual_path, int write_access);
void ember_vfs_init(ember_vm* vm);
void ember_vfs_cleanup(ember_vm* vm);

// Enhanced error handling API functions
void ember_set_current_source(const char* source, const char* filename);
ember_error* ember_error_syntax(ember_token* token, const char* message, const char* expected);
ember_error* ember_error_runtime(ember_vm* vm, const char* message);
ember_error* ember_error_type_mismatch(ember_vm* vm, const char* message, const char* expected_type, const char* actual_type);
ember_error* ember_error_security(ember_vm* vm, const char* message);
ember_error* ember_error_memory(const char* message);
void ember_error_print(ember_error* error);
void ember_error_print_with_context(ember_error* error, int context_lines);
void ember_error_free(ember_error* error);
void ember_push_call_frame(ember_vm* vm, const char* function_name, ember_source_location location);
void ember_pop_call_frame(ember_vm* vm);
void ember_print_call_stack(ember_vm* vm);
void ember_vm_set_error(ember_vm* vm, ember_error* error);
ember_error* ember_vm_get_error(ember_vm* vm);
void ember_vm_clear_error(ember_vm* vm);
int ember_vm_has_error(ember_vm* vm);

// Built-in native functions
ember_value ember_native_print(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_abs(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_sqrt(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_type(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_len(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_max(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_min(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_floor(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_ceil(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_round(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_pow(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_not(ember_vm* vm, int argc, ember_value* argv);

// Exception functions
ember_value ember_native_is_exception(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_get_exception_type(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_get_stack_trace(ember_vm* vm, int argc, ember_value* argv);

// String manipulation functions
ember_value ember_native_substr(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_split(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_join(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_starts_with(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_ends_with(ember_vm* vm, int argc, ember_value* argv);

// File I/O functions
ember_value ember_native_read_file(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_write_file(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_append_file(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_file_exists(ember_vm* vm, int argc, ember_value* argv);

// Type conversion functions (explicit, security-focused)
ember_value ember_native_str(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_num(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_int(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_bool(ember_vm* vm, int argc, ember_value* argv);

// Secure VM Pool API
// Error codes for VM pool operations
#define EMBER_SUCCESS 0
#define EMBER_ERROR_INVALID_PARAMETER -1
#define EMBER_ERROR_OPERATION_FAILED -2
#define EMBER_ERROR_MEMORY_ALLOCATION -3
#define EMBER_ERROR_SECURITY_VIOLATION -4
#define EMBER_ERROR_RESOURCE_EXHAUSTED -5

// Maximum safe values for configuration parameters
#define EMBER_MAX_SAFE_SIZE 1000000
#define EMBER_MAX_SAFE_POOL_SIZE 1000
#define EMBER_MAX_SAFE_TIMEOUT 86400000  // 24 hours in milliseconds

// Secure VM pool configuration structure
typedef struct {
    uint32_t initial_size;          // Initial pool size (validated range: 1-1000)
    uint32_t chunk_size;            // Growth chunk size (validated range: 1-100)
    uint32_t thread_cache_size;     // Thread cache size (validated range: 1-100)
    uint32_t max_vms_per_thread;    // Max VMs per thread (validated range: 1-100)
    uint32_t rate_limit_window_ms;  // Rate limit window in ms (validated range: 100-86400000)
    uint32_t rate_limit_max_allocs; // Max allocations per window (validated range: 1-10000)
} vm_pool_config_t;

// Secure VM pool API functions
int ember_pool_init(const vm_pool_config_t* config);
void ember_pool_cleanup(void);
ember_vm* ember_pool_get_vm(void);
void ember_pool_release_vm(ember_vm* vm);

// Object helper macros
#define IS_NUMBER(value) ((value).type == EMBER_VAL_NUMBER)
#define AS_NUMBER(value) ((value).as.number_val)
#define IS_BOOL(value) ((value).type == EMBER_VAL_BOOL)
#define AS_BOOL(value) ((value).as.bool_val)
#define IS_STRING(value) ((value).type == EMBER_VAL_STRING)
#define AS_STRING(value) ((ember_string*)((value).as.obj_val))
#define AS_CSTRING(value) (((ember_string*)((value).as.obj_val))->chars)
#define IS_ARRAY(value) ((value).type == EMBER_VAL_ARRAY)
#define AS_ARRAY(value) ((ember_array*)((value).as.obj_val))
#define IS_HASH_MAP(value) ((value).type == EMBER_VAL_HASH_MAP)
#define AS_HASH_MAP(value) ((ember_hash_map*)((value).as.obj_val))
#define IS_EXCEPTION(value) ((value).type == EMBER_VAL_EXCEPTION)
#define AS_EXCEPTION(value) ((ember_exception*)((value).as.obj_val))
#define IS_CLASS(value) ((value).type == EMBER_VAL_CLASS)
#define AS_CLASS(value) ((ember_class*)((value).as.obj_val))
#define IS_INSTANCE(value) ((value).type == EMBER_VAL_INSTANCE)
#define AS_INSTANCE(value) ((ember_instance*)((value).as.obj_val))
#define IS_BOUND_METHOD(value) ((value).type == EMBER_VAL_FUNCTION && ((value).as.obj_val)->type == OBJ_METHOD)
#define AS_BOUND_METHOD(value) ((ember_bound_method*)((value).as.obj_val))
#define IS_PROMISE(value) ((value).type == EMBER_VAL_PROMISE)
#define AS_PROMISE(value) ((ember_promise*)((value).as.obj_val))
#define IS_GENERATOR(value) ((value).type == EMBER_VAL_GENERATOR)
#define AS_GENERATOR(value) ((ember_generator*)((value).as.obj_val))
#define IS_SET(value) ((value).type == EMBER_VAL_SET)
#define AS_SET(value) ((ember_set*)((value).as.obj_val))
#define IS_MAP(value) ((value).type == EMBER_VAL_MAP)
#define AS_MAP(value) ((ember_map*)((value).as.obj_val))
#define IS_REGEX(value) ((value).type == EMBER_VAL_REGEX)
#define AS_REGEX(value) ((ember_regex*)((value).as.obj_val))
// Iterator access macros
#define AS_ITERATOR(value) ((ember_iterator*)((value).as.obj_val))

#ifdef __cplusplus
}
#endif

#endif // EMBER_H