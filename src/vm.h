#ifndef EMBER_VM_H
#define EMBER_VM_H

#include <stddef.h>
#include <stdint.h>
#include "ember.h"

// VM creation and management
ember_vm* ember_new_vm(void);
void ember_free_vm(ember_vm* vm);

// Execution
int ember_run(ember_vm* vm);
int ember_eval(ember_vm* vm, const char* source);
int ember_call(ember_vm* vm, const char* func_name, int argc, ember_value* argv);

// Stack operations
void push(ember_vm* vm, ember_value value);
ember_value pop(ember_vm* vm);
ember_value peek(ember_vm* vm, int distance);
ember_value ember_peek_stack_top(ember_vm* vm);

// Function registration
void ember_register_func(ember_vm* vm, const char* name, ember_native_func func);

// Enhanced Garbage collection
void collect_garbage(ember_vm* vm);
void mark_object(ember_object* object);
void mark_value(ember_value value);

// Enhanced GC functions
ember_object* allocate_object_enhanced(ember_vm* vm, size_t size, ember_object_type type);
void gc_configure(ember_vm* vm, int enable_generational, int enable_incremental, 
                  int enable_write_barriers, int enable_object_pooling);
void gc_print_statistics(ember_vm* vm);
void gc_write_barrier_helper(ember_vm* vm, ember_object* obj, ember_value old_val, ember_value new_val);
void gc_init(ember_vm* vm);
void gc_cleanup(ember_vm* vm);

// Chunk operations
void init_chunk(ember_chunk* chunk);
void free_chunk(ember_chunk* chunk);
void write_chunk(ember_chunk* chunk, uint8_t byte);
int add_constant(ember_chunk* chunk, ember_value value);

// Emit functions for parser compatibility
void emit_byte(ember_chunk* chunk, uint8_t byte);
void emit_bytes(ember_chunk* chunk, uint8_t byte1, uint8_t byte2);

// Function tracking
void track_function_chunk(ember_vm* vm, ember_chunk* chunk);

// Debug
void print_stack_trace(ember_vm* vm);

// Performance optimization API
void vm_optimization_init(ember_vm* vm);
void vm_optimization_cleanup(void);
void vm_set_optimization_level(int level); // 0=none, 1=basic, 2=advanced, 3=max
void vm_print_performance_stats(void);

// Break/Continue support
void vm_push_loop_context(ember_vm* vm, uint8_t* loop_start);
void vm_pop_loop_context(ember_vm* vm);
ember_runtime_loop_context* vm_get_current_loop_context(ember_vm* vm);

// VM pool support functions
int ember_vm_init(ember_vm* vm);
void ember_vm_cleanup(ember_vm* vm);
void ember_vm_clear_error(ember_vm* vm);
ember_value ember_make_nil(void);

#endif // EMBER_VM_H