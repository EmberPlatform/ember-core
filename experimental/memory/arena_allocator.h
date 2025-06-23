#ifndef EMBER_ARENA_ALLOCATOR_H
#define EMBER_ARENA_ALLOCATOR_H

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// NUMA topology detection
#define MAX_NUMA_NODES 64
#define MAX_CPU_CORES 1024

typedef struct {
    int node_count;
    int cores_per_node[MAX_NUMA_NODES];
    int cpu_to_node[MAX_CPU_CORES];
    size_t node_memory[MAX_NUMA_NODES];
    bool available;
} numa_topology_t;

// Cache line size for alignment optimization
#define CACHE_LINE_SIZE 64
#define CACHE_ALIGN(size) (((size) + CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE - 1))

// Forward declarations
typedef struct arena_allocator arena_allocator;
typedef struct arena_block arena_block;
typedef struct thread_arena thread_arena;
typedef struct memory_manager memory_manager;

// Enhanced arena statistics
typedef struct {
    size_t total_allocated;
    size_t total_used;
    size_t block_count;
    double utilization;
    
    // NUMA statistics
    size_t numa_local_allocations;
    size_t numa_remote_allocations;
    double numa_locality_ratio;
    
    // Performance metrics
    uint64_t allocation_count;
    uint64_t allocation_time_ns;
    double avg_allocation_time_ns;
    
    // Fragmentation metrics
    size_t fragmented_bytes;
    double fragmentation_ratio;
    
    // Cache efficiency
    uint64_t cache_hits;
    uint64_t cache_misses;
    double cache_hit_ratio;
} arena_stats;

// Forward declaration for lock-free free list
struct free_node;

// SLAB allocator for fixed-size objects
typedef struct slab_class {
    size_t object_size;
    size_t objects_per_slab;
    void** free_objects;
    size_t free_count;
    void* slabs;
    size_t slab_count;
    int numa_node;
    pthread_mutex_t mutex;
    
    // Lock-free free list (for high-performance allocator)
    volatile struct free_node* free_list;
    
    // Statistics
    uint64_t allocations;
    uint64_t deallocations;
    uint64_t cache_hits;
    uint64_t cache_misses;
} slab_class_t;

typedef struct {
    slab_class_t* size_classes;
    size_t class_count;
    size_t max_object_size;
    bool numa_interleaving;
    
    // NUMA-aware SLAB management
    slab_class_t numa_slabs[MAX_NUMA_NODES][32];
    size_t numa_class_counts[MAX_NUMA_NODES];
    
    // Global statistics
    uint64_t total_slab_allocations;
    uint64_t total_slab_deallocations;
    double cache_hit_ratio;
} slab_allocator_t;

// Large object allocator
typedef struct large_object {
    void* ptr;
    size_t size;
    int numa_node;
    uint64_t allocation_time;
    struct large_object* next;
} large_object_t;

typedef struct {
    large_object_t* objects;
    size_t object_count;
    size_t total_bytes;
    pthread_mutex_t mutex;
    
    // NUMA distribution
    size_t numa_objects[MAX_NUMA_NODES];
    size_t numa_bytes[MAX_NUMA_NODES];
} large_object_allocator_t;

// NUMA topology detection
numa_topology_t* numa_detect_topology(void);
int numa_get_current_node(void);
int numa_get_cpu_node(int cpu);
void numa_bind_to_node(int node);
void numa_set_preferred_node(int node);

// Enhanced arena allocator API
arena_allocator* arena_create(void);
arena_allocator* arena_create_numa_aware(size_t default_block_size, size_t max_threads);
void* arena_alloc(arena_allocator* arena, size_t size);
void* arena_calloc(arena_allocator* arena, size_t count, size_t size);
void* arena_alloc_aligned(arena_allocator* arena, size_t size, size_t alignment);
void* arena_alloc_numa(arena_allocator* arena, size_t size, int numa_node);
void* arena_alloc_cache_aligned(arena_allocator* arena, size_t size);
void arena_reset(arena_allocator* arena);
void arena_destroy(arena_allocator* arena);
void arena_compact(arena_allocator* arena);
double arena_get_fragmentation(arena_allocator* arena);

// Thread-local arena functions
thread_arena* thread_arena_get(arena_allocator* arena);
thread_arena* thread_arena_create(size_t block_size, int numa_node);
void thread_arena_destroy(thread_arena* thread_arena);
void* thread_arena_alloc(thread_arena* thread_arena, size_t size);
void thread_arena_reset(thread_arena* thread_arena);

// SLAB allocator functions
slab_allocator_t* slab_allocator_create(void);
void slab_allocator_destroy(slab_allocator_t* slab);
void* slab_alloc(slab_allocator_t* slab, size_t size);
void slab_free(slab_allocator_t* slab, void* ptr, size_t size);
slab_class_t* slab_get_class(slab_allocator_t* slab, size_t size);
void slab_add_size_class(slab_allocator_t* slab, size_t size, size_t objects_per_slab);

// Large object allocator functions
large_object_allocator_t* large_object_allocator_create(void);
void large_object_allocator_destroy(large_object_allocator_t* large);
void* large_object_alloc(large_object_allocator_t* large, size_t size, int numa_node);
void large_object_free(large_object_allocator_t* large, void* ptr);

// Memory manager functions
memory_manager* memory_manager_create(void);
void memory_manager_destroy(memory_manager* mgr);
void* memory_manager_alloc(memory_manager* mgr, size_t size);
void* memory_manager_alloc_numa(memory_manager* mgr, size_t size, int numa_node);
void memory_manager_free(memory_manager* mgr, void* ptr, size_t size);
void memory_manager_compact(memory_manager* mgr);
void memory_manager_set_pressure_callback(memory_manager* mgr, void (*callback)(void*), void* data);

// Statistics and monitoring
void arena_get_stats(arena_allocator* arena, arena_stats* stats);
void arena_print_stats(arena_allocator* arena);
void slab_print_statistics(slab_allocator_t* slab);
void memory_manager_print_statistics(memory_manager* mgr);
double memory_manager_get_efficiency(memory_manager* mgr);
size_t memory_manager_get_fragmentation(memory_manager* mgr);
double memory_manager_get_numa_locality(memory_manager* mgr);

// Memory layout optimization
void memory_prefetch(void* addr, size_t size);
void memory_prefetch_read(void* addr, size_t size);
void memory_prefetch_write(void* addr, size_t size);
bool memory_enable_hugepages(void* addr, size_t size);
void memory_disable_hugepages(void* addr, size_t size);

// Cache-friendly allocation functions
void* memory_alloc_cache_aligned(memory_manager* mgr, size_t size);
void* memory_alloc_page_aligned(memory_manager* mgr, size_t size);
void* memory_alloc_structure_of_arrays(memory_manager* mgr, size_t element_size, size_t element_count, size_t field_count);

// Memory validation and debugging
bool arena_validate(arena_allocator* arena);
bool slab_validate(slab_allocator_t* slab);
bool memory_manager_validate(memory_manager* mgr);
void memory_manager_dump_state(memory_manager* mgr, const char* filename);

#ifdef __cplusplus
}
#endif

#endif // EMBER_ARENA_ALLOCATOR_H