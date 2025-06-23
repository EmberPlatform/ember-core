#include "vm_memory_integration.h"
#include "../memory/numa_arena_allocator.c"  // Include our optimized allocator
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/mman.h>
#include <unistd.h>

// VM Memory Management Integration Implementation
// Provides enterprise-grade memory management for the Ember VM

// High-performance time measurement
uint64_t vm_get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Default memory configuration
vm_memory_config vm_memory_get_default_config(void) {
    vm_memory_config config = {
        // Arena settings
        .default_arena_size = 2 * 1024 * 1024,  // 2MB arenas
        .max_arenas = 1024,
        .enable_numa_awareness = true,
        .enable_hugepages = true,
        .enable_prefetching = true,
        
        // SLAB allocator settings
        .enable_slab_allocator = true,
        .slab_cache_size = 256,
        
        // Large object settings
        .large_object_threshold = 512 * 1024,  // 512KB
        .use_mmap_for_large_objects = true,
        
        // Garbage collection settings
        .enable_generational_gc = true,
        .enable_incremental_gc = true,
        .gc_threshold_factor = 1.5,
        
        // Optimization settings
        .enable_memory_compaction = true,
        .compaction_threshold = 0.25,  // 25% fragmentation
        .enable_cache_optimization = true,
        .enable_memory_pooling = true,
        
        // Performance monitoring
        .enable_performance_tracking = true,
        .enable_allocation_profiling = false  // Disabled by default for performance
    };
    return config;
}

// Initialize VM memory context
vm_memory_context* vm_memory_init(const vm_memory_config* config) {
    vm_memory_context* ctx = malloc(sizeof(vm_memory_context));
    if (!ctx) {
        return NULL;
    }
    
    memset(ctx, 0, sizeof(vm_memory_context));
    
    // Copy configuration
    if (config) {
        ctx->config = *config;
    } else {
        ctx->config = vm_memory_get_default_config();
    }
    
    // Initialize memory managers
    ctx->primary_manager = memory_manager_create();
    ctx->object_manager = memory_manager_create();
    ctx->string_manager = memory_manager_create();
    
    if (!ctx->primary_manager || !ctx->object_manager || !ctx->string_manager) {
        vm_memory_destroy(ctx);
        return NULL;
    }
    
    // Initialize arena allocators
    ctx->vm_arena = arena_create_numa_aware(ctx->config.default_arena_size, ctx->config.max_arenas);
    ctx->object_arena = arena_create_numa_aware(ctx->config.default_arena_size / 2, ctx->config.max_arenas);
    ctx->string_arena = arena_create_numa_aware(ctx->config.default_arena_size / 4, ctx->config.max_arenas);
    
    if (!ctx->vm_arena || !ctx->object_arena || !ctx->string_arena) {
        vm_memory_destroy(ctx);
        return NULL;
    }
    
    // Initialize SLAB allocator
    if (ctx->config.enable_slab_allocator) {
        ctx->small_object_slab = slab_allocator_create();
        if (!ctx->small_object_slab) {
            vm_memory_destroy(ctx);
            return NULL;
        }
    }
    
    // Initialize large object allocator
    ctx->large_allocator = large_object_allocator_create();
    if (!ctx->large_allocator) {
        vm_memory_destroy(ctx);
        return NULL;
    }
    
    // Initialize NUMA context
    ctx->preferred_numa_node = numa_get_current_node();
    ctx->numa_available = global_numa_topology.available;
    
    // Initialize object pools (simplified implementation)
    memset(&ctx->object_pools, 0, sizeof(ctx->object_pools));
    
    // Initialize GC state
    ctx->gc_state.gc_in_progress = false;
    ctx->gc_state.last_gc_time = vm_get_time_ns();
    ctx->gc_state.bytes_since_last_gc = 0;
    ctx->gc_state.gc_pressure = 0.0;
    
    // Initialize memory pressure handling
    ctx->pressure_state.under_pressure = false;
    ctx->pressure_state.pressure_threshold = 1024 * 1024 * 1024;  // 1GB default
    ctx->pressure_state.pressure_callback = NULL;
    ctx->pressure_state.last_pressure_check = vm_get_time_ns();
    
    // Initialize threading
    pthread_mutex_init(&ctx->allocation_mutex, NULL);
    pthread_rwlock_init(&ctx->stats_lock, NULL);
    
    // Initialize statistics
    memset(&ctx->stats, 0, sizeof(vm_memory_stats));
    
    return ctx;
}

// Destroy VM memory context
void vm_memory_destroy(vm_memory_context* ctx) {
    if (!ctx) {
        return;
    }
    
    // Destroy memory managers
    if (ctx->primary_manager) {
        memory_manager_destroy(ctx->primary_manager);
    }
    if (ctx->object_manager) {
        memory_manager_destroy(ctx->object_manager);
    }
    if (ctx->string_manager) {
        memory_manager_destroy(ctx->string_manager);
    }
    
    // Destroy arena allocators
    if (ctx->vm_arena) {
        arena_destroy(ctx->vm_arena);
    }
    if (ctx->object_arena) {
        arena_destroy(ctx->object_arena);
    }
    if (ctx->string_arena) {
        arena_destroy(ctx->string_arena);
    }
    
    // Destroy SLAB allocator
    if (ctx->small_object_slab) {
        slab_allocator_destroy(ctx->small_object_slab);
    }
    
    // Destroy large object allocator
    if (ctx->large_allocator) {
        large_object_allocator_destroy(ctx->large_allocator);
    }
    
    // Cleanup threading
    pthread_mutex_destroy(&ctx->allocation_mutex);
    pthread_rwlock_destroy(&ctx->stats_lock);
    
    free(ctx);
}

// High-performance object allocation
void* vm_alloc_object(vm_memory_context* ctx, size_t size, ember_object_type type) {
    if (!ctx || size == 0) {
        return NULL;
    }
    
    uint64_t start_time = vm_get_time_ns();
    void* ptr = NULL;
    
    // Route allocation based on size and type
    if (size >= ctx->config.large_object_threshold) {
        // Use large object allocator
        ptr = large_object_alloc(ctx->large_allocator, size, ctx->preferred_numa_node);
        if (ptr) {
            __sync_fetch_and_add(&ctx->stats.large_allocations, 1);
        }
    } else if (ctx->config.enable_slab_allocator && size <= 65536) {
        // Use SLAB allocator for small/medium objects
        ptr = slab_alloc(ctx->small_object_slab, size);
        if (ptr) {
            __sync_fetch_and_add(&ctx->stats.object_allocations, 1);
        }
    } else {
        // Use appropriate arena
        arena_allocator* arena = ctx->object_arena;
        
        switch (type) {
            case OBJ_STRING:
                arena = ctx->string_arena;
                break;
            case OBJ_ARRAY:
            case OBJ_HASH_MAP:
                arena = ctx->object_arena;
                break;
            default:
                arena = ctx->vm_arena;
                break;
        }
        
        ptr = arena_alloc(arena, size);
        
        // Update type-specific statistics
        switch (type) {
            case OBJ_STRING:
                __sync_fetch_and_add(&ctx->stats.string_allocations, 1);
                break;
            case OBJ_ARRAY:
                __sync_fetch_and_add(&ctx->stats.array_allocations, 1);
                break;
            case OBJ_HASH_MAP:
                __sync_fetch_and_add(&ctx->stats.hashmap_allocations, 1);
                break;
            default:
                __sync_fetch_and_add(&ctx->stats.object_allocations, 1);
                break;
        }
    }
    
    // Update global statistics
    if (ptr) {
        uint64_t end_time = vm_get_time_ns();
        uint64_t alloc_time = end_time - start_time;
        
        __sync_fetch_and_add(&ctx->stats.total_allocations, 1);
        __sync_fetch_and_add(&ctx->stats.total_bytes_allocated, size);
        __sync_fetch_and_add(&ctx->stats.current_memory_usage, size);
        __sync_fetch_and_add(&ctx->stats.total_allocation_time_ns, alloc_time);
        
        // Update peak memory usage
        if (ctx->stats.current_memory_usage > ctx->stats.peak_memory_usage) {
            ctx->stats.peak_memory_usage = ctx->stats.current_memory_usage;
        }
        
        // Update average allocation time
        ctx->stats.avg_allocation_time_ns = 
            ctx->stats.total_allocation_time_ns / ctx->stats.total_allocations;
        
        // Track NUMA locality
        int current_node = numa_get_current_node();
        if (current_node == ctx->preferred_numa_node) {
            __sync_fetch_and_add(&ctx->stats.numa_local_allocations, 1);
        } else {
            __sync_fetch_and_add(&ctx->stats.numa_remote_allocations, 1);
        }
        
        // Update NUMA efficiency
        uint64_t total_numa_allocs = ctx->stats.numa_local_allocations + ctx->stats.numa_remote_allocations;
        if (total_numa_allocs > 0) {
            ctx->stats.numa_efficiency = (double)ctx->stats.numa_local_allocations / total_numa_allocs;
        }
        
        // Track GC pressure
        ctx->gc_state.bytes_since_last_gc += size;
        if (ctx->gc_state.bytes_since_last_gc > ctx->config.default_arena_size) {
            ctx->gc_state.gc_pressure += 0.1;
        }
        
        // Memory prefetching optimization
        if (ctx->config.enable_prefetching && size >= 64) {
            memory_prefetch_write(ptr, size < 256 ? size : 256);
        }
    }
    
    return ptr;
}

// Cache-aligned allocation
void* vm_alloc_cache_aligned(vm_memory_context* ctx, size_t size) {
    if (!ctx || size == 0) {
        return NULL;
    }
    
    return arena_alloc_aligned(ctx->object_arena, size, CACHE_LINE_SIZE);
}

// NUMA-local allocation
void* vm_alloc_numa_local(vm_memory_context* ctx, size_t size) {
    if (!ctx || size == 0) {
        return NULL;
    }
    
    return arena_alloc_numa(ctx->object_arena, size, ctx->preferred_numa_node);
}

// String interning with NUMA awareness
ember_string* vm_intern_string(vm_memory_context* ctx, const char* chars, size_t length) {
    if (!ctx || !chars || length == 0) {
        return NULL;
    }
    
    // Allocate string object from string arena
    size_t total_size = sizeof(ember_string) + length + 1;  // +1 for null terminator
    ember_string* str = (ember_string*)arena_alloc(ctx->string_arena, total_size);
    
    if (str) {
        str->obj.type = OBJ_STRING;
        str->obj.is_marked = false;
        str->obj.next = NULL;
        str->length = length;
        str->chars = (char*)(str + 1);  // Point to memory after struct
        
        // Copy string data
        memcpy(str->chars, chars, length);
        str->chars[length] = '\0';
        
        // Note: ember_string doesn't have a hash field in this implementation
        // String interning would normally be handled by the VM's string table
        
        // Prefetch string data for better cache performance
        if (ctx->config.enable_prefetching) {
            memory_prefetch_read(str->chars, length);
        }
    }
    
    return str;
}

// Memory compaction
void vm_memory_compact(vm_memory_context* ctx) {
    if (!ctx || !ctx->config.enable_memory_compaction) {
        return;
    }
    
    uint64_t start_time = vm_get_time_ns();
    
    // Compact all arenas
    arena_compact(ctx->vm_arena);
    arena_compact(ctx->object_arena);
    arena_compact(ctx->string_arena);
    
    uint64_t end_time = vm_get_time_ns();
    ctx->stats.compaction_time_ns += (end_time - start_time);
    
    // Update fragmentation statistics
    double vm_frag = arena_get_fragmentation(ctx->vm_arena);
    double obj_frag = arena_get_fragmentation(ctx->object_arena);
    double str_frag = arena_get_fragmentation(ctx->string_arena);
    
    ctx->stats.fragmentation_percentage = (vm_frag + obj_frag + str_frag) / 3.0 * 100.0;
}

// Get memory statistics
void vm_memory_get_stats(vm_memory_context* ctx, vm_memory_stats* stats) {
    if (!ctx || !stats) {
        return;
    }
    
    pthread_rwlock_rdlock(&ctx->stats_lock);
    *stats = ctx->stats;
    
    // Calculate derived statistics
    if (stats->total_allocations > 0) {
        stats->allocation_efficiency = 
            (double)stats->current_memory_usage / stats->total_bytes_allocated;
    }
    
    uint64_t total_locality_allocs = stats->numa_local_allocations + stats->numa_remote_allocations;
    if (total_locality_allocs > 0) {
        stats->memory_locality_ratio = 
            (double)stats->numa_local_allocations / total_locality_allocs;
    }
    
    pthread_rwlock_unlock(&ctx->stats_lock);
}

// Print memory statistics
void vm_memory_print_stats(vm_memory_context* ctx) {
    if (!ctx) {
        return;
    }
    
    vm_memory_stats stats;
    vm_memory_get_stats(ctx, &stats);
    
    printf("\n=== VM Memory Management Statistics ===\n");
    printf("Allocation Performance:\n");
    printf("  Total allocations: %lu\n", stats.total_allocations);
    printf("  Total bytes allocated: %lu (%.2f MB)\n", 
           stats.total_bytes_allocated, (double)stats.total_bytes_allocated / (1024 * 1024));
    printf("  Peak memory usage: %lu (%.2f MB)\n", 
           stats.peak_memory_usage, (double)stats.peak_memory_usage / (1024 * 1024));
    printf("  Current memory usage: %lu (%.2f MB)\n", 
           stats.current_memory_usage, (double)stats.current_memory_usage / (1024 * 1024));
    printf("  Average allocation time: %lu ns\n", stats.avg_allocation_time_ns);
    
    printf("\nEfficiency Metrics:\n");
    printf("  Allocation efficiency: %.1f%%\n", stats.allocation_efficiency * 100.0);
    printf("  Memory locality ratio: %.1f%%\n", stats.memory_locality_ratio * 100.0);
    printf("  Fragmentation: %.1f%%\n", stats.fragmentation_percentage);
    
    printf("\nNUMA Performance:\n");
    printf("  NUMA local allocations: %lu\n", stats.numa_local_allocations);
    printf("  NUMA remote allocations: %lu\n", stats.numa_remote_allocations);
    printf("  NUMA efficiency: %.1f%%\n", stats.numa_efficiency * 100.0);
    
    printf("\nAllocation Breakdown:\n");
    printf("  Object allocations: %lu\n", stats.object_allocations);
    printf("  String allocations: %lu\n", stats.string_allocations);
    printf("  Array allocations: %lu\n", stats.array_allocations);
    printf("  HashMap allocations: %lu\n", stats.hashmap_allocations);
    printf("  Large allocations: %lu\n", stats.large_allocations);
    
    printf("\nTiming Information:\n");
    printf("  Total allocation time: %.2f ms\n", (double)stats.total_allocation_time_ns / 1000000.0);
    printf("  GC time: %.2f ms\n", (double)stats.gc_time_ns / 1000000.0);
    printf("  Compaction time: %.2f ms\n", (double)stats.compaction_time_ns / 1000000.0);
    
    printf("========================================\n\n");
    
    // Print component statistics
    printf("Component Details:\n");
    arena_print_stats(ctx->vm_arena);
    arena_print_stats(ctx->object_arena);
    arena_print_stats(ctx->string_arena);
    
    if (ctx->small_object_slab) {
        slab_print_statistics(ctx->small_object_slab);
    }
}

// VM Integration functions
int vm_integrate_memory_management(ember_vm* vm, const vm_memory_config* config) {
    if (!vm) {
        return -1;
    }
    
    // Initialize memory context
    vm_memory_context* ctx = vm_memory_init(config);
    if (!ctx) {
        return -1;
    }
    
    // Store memory context in VM
    vm->memory_context = ctx;
    
    // Note: In this implementation, the VM structure doesn't have an allocate_object function pointer
    // Enhanced allocation would be handled through explicit function calls to vm_allocate_enhanced
    
    return 0;
}

// Cleanup memory management
void vm_cleanup_memory_management(ember_vm* vm) {
    if (!vm || !vm->memory_context) {
        return;
    }
    
    vm_memory_context* ctx = (vm_memory_context*)vm->memory_context;
    vm_memory_destroy(ctx);
    vm->memory_context = NULL;
}

// Enhanced allocation function for VM integration
void* vm_allocate_enhanced(ember_vm* vm, size_t size, ember_object_type type) {
    if (!vm || !vm->memory_context) {
        return malloc(size);  // Fallback to standard malloc
    }
    
    vm_memory_context* ctx = (vm_memory_context*)vm->memory_context;
    return vm_alloc_object(ctx, size, type);
}

// Trigger memory compaction
void vm_trigger_compaction(ember_vm* vm) {
    if (!vm || !vm->memory_context) {
        return;
    }
    
    vm_memory_context* ctx = (vm_memory_context*)vm->memory_context;
    vm_memory_compact(ctx);
}

// Memory pressure checking
void vm_memory_check_pressure(vm_memory_context* ctx) {
    if (!ctx) {
        return;
    }
    
    uint64_t current_time = vm_get_time_ns();
    uint64_t time_since_check = current_time - ctx->pressure_state.last_pressure_check;
    
    // Check pressure every 100ms
    if (time_since_check < 100000000ULL) {
        return;
    }
    
    ctx->pressure_state.last_pressure_check = current_time;
    
    // Check if we're under memory pressure
    bool was_under_pressure = ctx->pressure_state.under_pressure;
    ctx->pressure_state.under_pressure = 
        ctx->stats.current_memory_usage > ctx->pressure_state.pressure_threshold;
    
    // Trigger callback if pressure state changed
    if (!was_under_pressure && ctx->pressure_state.under_pressure && 
        ctx->pressure_state.pressure_callback) {
        ctx->pressure_state.pressure_callback(ctx);
    }
}

// Memory pressure handling
void vm_memory_handle_pressure(vm_memory_context* ctx) {
    if (!ctx || !ctx->pressure_state.under_pressure) {
        return;
    }
    
    // Aggressive compaction under pressure
    vm_memory_compact(ctx);
    
    // Force GC if integrated
    if (!ctx->gc_state.gc_in_progress) {
        ctx->gc_state.gc_pressure = 1.0;  // Maximum pressure
    }
}

// Validation functions
bool vm_memory_validate_integrity(vm_memory_context* ctx) {
    if (!ctx) {
        return false;
    }
    
    // Validate arena allocators
    if (!arena_validate(ctx->vm_arena) ||
        !arena_validate(ctx->object_arena) ||
        !arena_validate(ctx->string_arena)) {
        return false;
    }
    
    // Validate memory managers
    if (!memory_manager_validate(ctx->primary_manager) ||
        !memory_manager_validate(ctx->object_manager) ||
        !memory_manager_validate(ctx->string_manager)) {
        return false;
    }
    
    // Validate statistics consistency
    if (ctx->stats.total_allocations < 
        (ctx->stats.object_allocations + ctx->stats.string_allocations + 
         ctx->stats.array_allocations + ctx->stats.hashmap_allocations + 
         ctx->stats.large_allocations)) {
        return false;
    }
    
    return true;
}

// Object lifecycle tracking
void vm_memory_track_allocation(vm_memory_context* ctx, void* ptr, size_t size, ember_object_type type) {
    (void)size; // Reserved for future memory tracking
    (void)type; // Reserved for future type-specific tracking
    if (!ctx || !ptr) {
        return;
    }
    
    // Track allocation in performance monitoring
    if (ctx->config.enable_allocation_profiling) {
        // In a full implementation, we'd maintain allocation tracking tables
        // For now, just update counters
        __sync_fetch_and_add(&ctx->stats.total_allocations, 1);
    }
}

// Memory prefetching for objects
void vm_memory_prefetch_object(void* obj, ember_object_type type) {
    if (!obj) {
        return;
    }
    
    size_t prefetch_size = 64;  // One cache line by default
    
    switch (type) {
        case OBJ_STRING:
            prefetch_size = 128;  // Strings often accessed sequentially
            break;
        case OBJ_ARRAY:
            prefetch_size = 256;  // Arrays benefit from larger prefetch
            break;
        case OBJ_HASH_MAP:
            prefetch_size = 512;  // Hash maps have complex access patterns
            break;
        default:
            break;
    }
    
    memory_prefetch(obj, prefetch_size);
}

// Cache line flushing
void vm_memory_flush_cache_lines(void* addr, size_t size) {
    if (!addr || size == 0) {
        return;
    }
    
    char* ptr = (char*)addr;
    char* end = ptr + size;
    
    while (ptr < end) {
        __builtin_ia32_clflush(ptr);  // x86-specific cache flush
        ptr += CACHE_LINE_SIZE;
    }
}

// Performance measurement helper
void vm_memory_track_timing(vm_memory_context* ctx, const char* operation, uint64_t time_ns) {
    if (!ctx || !operation) {
        return;
    }
    
    // In a full implementation, we'd maintain operation-specific timing statistics
    // For now, add to general allocation timing
    __sync_fetch_and_add(&ctx->stats.total_allocation_time_ns, time_ns);
}

// Get memory efficiency
double vm_memory_get_efficiency(vm_memory_context* ctx) {
    if (!ctx || ctx->stats.total_bytes_allocated == 0) {
        return 0.0;
    }
    
    return (double)ctx->stats.current_memory_usage / ctx->stats.total_bytes_allocated;
}

// Get locality ratio
double vm_memory_get_locality_ratio(vm_memory_context* ctx) {
    if (!ctx) {
        return 0.0;
    }
    
    uint64_t total = ctx->stats.numa_local_allocations + ctx->stats.numa_remote_allocations;
    if (total == 0) {
        return 0.0;
    }
    
    return (double)ctx->stats.numa_local_allocations / total;
}

// Simple allocation with strategy
void* vm_alloc_with_strategy(vm_memory_context* ctx, size_t size, 
                            ember_object_type type, vm_allocation_strategy strategy) {
    if (!ctx || size == 0) {
        return NULL;
    }
    
    switch (strategy) {
        case VM_ALLOC_STRATEGY_NUMA_LOCAL:
            return vm_alloc_numa_local(ctx, size);
        case VM_ALLOC_STRATEGY_CACHE_OPTIMIZED:
            return vm_alloc_cache_aligned(ctx, size);
        case VM_ALLOC_STRATEGY_LARGE_OBJECT:
            return large_object_alloc(ctx->large_allocator, size, ctx->preferred_numa_node);
        default:
            return vm_alloc_object(ctx, size, type);
    }
}