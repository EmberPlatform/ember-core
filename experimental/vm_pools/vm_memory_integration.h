#ifndef VM_MEMORY_INTEGRATION_H
#define VM_MEMORY_INTEGRATION_H

// Arena allocator moved to experimental
// #include "../memory/arena_allocator.h"
#include "../../vm.h"

#ifdef __cplusplus
extern "C" {
#endif

// VM Memory Management Integration
// Provides high-performance memory management for the Ember VM using
// arena allocators, NUMA-aware allocation, and cache-optimized layouts

// VM Memory Statistics
typedef struct {
    // Basic statistics
    uint64_t total_allocations;
    uint64_t total_bytes_allocated;
    uint64_t peak_memory_usage;
    uint64_t current_memory_usage;
    
    // Performance metrics
    double allocation_efficiency;
    double memory_locality_ratio;
    uint64_t cache_hit_ratio;
    double fragmentation_percentage;
    
    // NUMA metrics
    uint64_t numa_local_allocations;
    uint64_t numa_remote_allocations;
    double numa_efficiency;
    
    // Allocation breakdown
    uint64_t object_allocations;
    uint64_t string_allocations;
    uint64_t array_allocations;
    uint64_t hashmap_allocations;
    uint64_t large_allocations;
    
    // Performance timings
    uint64_t total_allocation_time_ns;
    uint64_t avg_allocation_time_ns;
    uint64_t gc_time_ns;
    uint64_t compaction_time_ns;
} vm_memory_stats;

// VM Memory Configuration
typedef struct {
    // Arena settings
    size_t default_arena_size;
    size_t max_arenas;
    bool enable_numa_awareness;
    bool enable_hugepages;
    bool enable_prefetching;
    
    // SLAB allocator settings
    bool enable_slab_allocator;
    size_t slab_cache_size;
    
    // Large object settings
    size_t large_object_threshold;
    bool use_mmap_for_large_objects;
    
    // Garbage collection settings
    bool enable_generational_gc;
    bool enable_incremental_gc;
    double gc_threshold_factor;
    
    // Optimization settings
    bool enable_memory_compaction;
    double compaction_threshold;
    bool enable_cache_optimization;
    bool enable_memory_pooling;
    
    // Performance monitoring
    bool enable_performance_tracking;
    bool enable_allocation_profiling;
} vm_memory_config;

// Enhanced VM structure with optimized memory management
typedef struct vm_memory_context {
    // Core memory managers
    memory_manager* primary_manager;
    memory_manager* object_manager;
    memory_manager* string_manager;
    
    // Specialized allocators
    arena_allocator* vm_arena;           // For VM internal structures
    arena_allocator* object_arena;       // For Ember objects
    arena_allocator* string_arena;       // For string interning
    slab_allocator_t* small_object_slab; // For small objects
    large_object_allocator_t* large_allocator; // For large objects
    
    // NUMA context
    int preferred_numa_node;
    bool numa_available;
    
    // Memory pools for common objects
    struct {
        void* string_pool;
        void* array_pool;
        void* hashmap_pool;
        void* instance_pool;
        void* function_pool;
    } object_pools;
    
    // Performance tracking
    vm_memory_stats stats;
    vm_memory_config config;
    
    // GC integration
    struct {
        bool gc_in_progress;
        uint64_t last_gc_time;
        size_t bytes_since_last_gc;
        double gc_pressure;
    } gc_state;
    
    // Memory pressure handling
    struct {
        bool under_pressure;
        size_t pressure_threshold;
        void (*pressure_callback)(struct vm_memory_context*);
        uint64_t last_pressure_check;
    } pressure_state;
    
    // Thread safety
    pthread_mutex_t allocation_mutex;
    pthread_rwlock_t stats_lock;
    
} vm_memory_context;

// VM Memory Management API

// Initialization and configuration
vm_memory_context* vm_memory_init(const vm_memory_config* config);
void vm_memory_destroy(vm_memory_context* ctx);
int vm_memory_configure(vm_memory_context* ctx, const vm_memory_config* config);
vm_memory_config vm_memory_get_default_config(void);

// High-performance object allocation
void* vm_alloc_object(vm_memory_context* ctx, size_t size, ember_object_type type);
void* vm_alloc_string(vm_memory_context* ctx, size_t length);
void* vm_alloc_array(vm_memory_context* ctx, size_t element_count);
void* vm_alloc_hashmap(vm_memory_context* ctx, size_t initial_capacity);
void* vm_alloc_instance(vm_memory_context* ctx, size_t field_count);
void* vm_alloc_function(vm_memory_context* ctx, size_t bytecode_size);

// Cache-optimized allocation
void* vm_alloc_cache_aligned(vm_memory_context* ctx, size_t size);
void* vm_alloc_numa_local(vm_memory_context* ctx, size_t size);
void* vm_alloc_large_object(vm_memory_context* ctx, size_t size);

// String interning with NUMA awareness
ember_string* vm_intern_string(vm_memory_context* ctx, const char* chars, size_t length);
ember_string* vm_intern_string_fast(vm_memory_context* ctx, const char* chars, size_t length);

// Memory pool management
void* vm_pool_acquire(vm_memory_context* ctx, ember_object_type type);
void vm_pool_release(vm_memory_context* ctx, void* obj, ember_object_type type);

// NUMA-aware operations
int vm_memory_get_current_numa_node(vm_memory_context* ctx);
void vm_memory_bind_to_numa_node(vm_memory_context* ctx, int node);
void* vm_alloc_on_numa_node(vm_memory_context* ctx, size_t size, int node);

// Memory compaction and optimization
void vm_memory_compact(vm_memory_context* ctx);
void vm_memory_optimize_layout(vm_memory_context* ctx);
void vm_memory_defragment(vm_memory_context* ctx);

// Garbage collection integration
void vm_memory_prepare_gc(vm_memory_context* ctx);
void vm_memory_finalize_gc(vm_memory_context* ctx);
void vm_memory_mark_objects(vm_memory_context* ctx, void** roots, size_t root_count);

// Performance monitoring
void vm_memory_get_stats(vm_memory_context* ctx, vm_memory_stats* stats);
void vm_memory_reset_stats(vm_memory_context* ctx);
void vm_memory_print_stats(vm_memory_context* ctx);
double vm_memory_get_efficiency(vm_memory_context* ctx);
double vm_memory_get_locality_ratio(vm_memory_context* ctx);

// Memory pressure handling
void vm_memory_check_pressure(vm_memory_context* ctx);
void vm_memory_handle_pressure(vm_memory_context* ctx);
void vm_memory_set_pressure_callback(vm_memory_context* ctx, 
                                     void (*callback)(vm_memory_context*));

// VM Integration functions
int vm_integrate_memory_management(ember_vm* vm, const vm_memory_config* config);
void vm_cleanup_memory_management(ember_vm* vm);
void* vm_allocate_enhanced(ember_vm* vm, size_t size, ember_object_type type);
void vm_trigger_compaction(ember_vm* vm);

// Object lifecycle tracking
void vm_memory_track_allocation(vm_memory_context* ctx, void* ptr, size_t size, ember_object_type type);
void vm_memory_track_deallocation(vm_memory_context* ctx, void* ptr);
void vm_memory_track_access(vm_memory_context* ctx, void* ptr);

// Cache optimization helpers
void vm_memory_prefetch_object(void* obj, ember_object_type type);
void vm_memory_flush_cache_lines(void* addr, size_t size);
void vm_memory_optimize_object_layout(void* obj, ember_object_type type);

// Structure-of-arrays optimization for better cache locality
typedef struct {
    // Separate arrays for different object fields
    ember_object_type* types;
    bool* is_marked;
    ember_object** next_ptrs;
    void** data_ptrs;
    size_t* sizes;
    
    size_t capacity;
    size_t count;
} vm_object_array;

vm_object_array* vm_create_object_array(vm_memory_context* ctx, size_t initial_capacity);
void vm_destroy_object_array(vm_memory_context* ctx, vm_object_array* array);
size_t vm_add_object_to_array(vm_object_array* array, ember_object* obj);
void vm_remove_object_from_array(vm_object_array* array, size_t index);

// Memory layout analysis and optimization
typedef struct {
    size_t total_memory;
    size_t wasted_memory;
    size_t fragmented_memory;
    double cache_utilization;
    double numa_efficiency;
    size_t object_count_by_type[16];  // Up to 16 object types
    double access_pattern_locality;
} vm_memory_analysis;

vm_memory_analysis vm_analyze_memory_layout(vm_memory_context* ctx);
void vm_optimize_based_on_analysis(vm_memory_context* ctx, const vm_memory_analysis* analysis);

// Debugging and validation
bool vm_memory_validate_integrity(vm_memory_context* ctx);
void vm_memory_dump_state(vm_memory_context* ctx, const char* filename);
void vm_memory_enable_debug_mode(vm_memory_context* ctx, bool enable);

// Advanced allocation strategies
typedef enum {
    VM_ALLOC_STRATEGY_DEFAULT,
    VM_ALLOC_STRATEGY_NUMA_LOCAL,
    VM_ALLOC_STRATEGY_CACHE_OPTIMIZED,
    VM_ALLOC_STRATEGY_LARGE_OBJECT,
    VM_ALLOC_STRATEGY_TEMPORARY,
    VM_ALLOC_STRATEGY_PERSISTENT
} vm_allocation_strategy;

void* vm_alloc_with_strategy(vm_memory_context* ctx, size_t size, 
                            ember_object_type type, vm_allocation_strategy strategy);

// Memory bandwidth optimization
void vm_memory_optimize_bandwidth(vm_memory_context* ctx);
void vm_memory_enable_streaming_stores(vm_memory_context* ctx, bool enable);
void vm_memory_prefetch_pattern(vm_memory_context* ctx, void** ptrs, size_t count);

// Integration with existing VM structures
#define VM_MEMORY_ENHANCED(vm) ((vm_memory_context*)(vm)->memory_context)
#define VM_ALLOC_ENHANCED(vm, size, type) vm_allocate_enhanced(vm, size, type)

// Macros for high-performance allocation
#define VM_FAST_ALLOC(ctx, size, type) \
    __builtin_expect(vm_alloc_object(ctx, size, type), 1)

#define VM_CACHE_ALLOC(ctx, size) \
    vm_alloc_cache_aligned(ctx, size)

#define VM_NUMA_ALLOC(ctx, size) \
    vm_alloc_numa_local(ctx, size)

// Performance measurement macros
#define VM_MEMORY_TIMER_START() \
    uint64_t __start_time = vm_get_time_ns()

#define VM_MEMORY_TIMER_END(ctx, operation) \
    do { \
        uint64_t __end_time = vm_get_time_ns(); \
        vm_memory_track_timing(ctx, operation, __end_time - __start_time); \
    } while(0)

// Utility functions
uint64_t vm_get_time_ns(void);
void vm_memory_track_timing(vm_memory_context* ctx, const char* operation, uint64_t time_ns);

#ifdef __cplusplus
}
#endif

#endif // VM_MEMORY_INTEGRATION_H