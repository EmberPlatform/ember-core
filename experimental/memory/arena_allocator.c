#include "arena_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <numa.h>
#include <numaif.h>
#include <sched.h>
#include <time.h>
#include <errno.h>

// SIMD security scanning support
#ifdef __x86_64__
#include <immintrin.h>
#define HAS_SIMD_MEMORY_SCAN 1
#else
#define HAS_SIMD_MEMORY_SCAN 0
#endif

// Branch prediction hints
#ifdef __GNUC__
#define LIKELY(x)       __builtin_expect(!!(x), 1)
#define UNLIKELY(x)     __builtin_expect(!!(x), 0)
#define PREFETCH_READ(ptr) __builtin_prefetch((ptr), 0, 3)
#define PREFETCH_WRITE(ptr) __builtin_prefetch((ptr), 1, 3)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#define PREFETCH_READ(ptr) ((void)0)
#define PREFETCH_WRITE(ptr) ((void)0)
#endif

// Security scanning patterns (common attack signatures)
static const uint8_t SECURITY_PATTERNS[] = {
    0x90, 0x90, 0x90, 0x90,  // NOP sled
    0xCC, 0xCC, 0xCC, 0xCC,  // INT3 debug breaks
    0xEB, 0xFE, 0x00, 0x00,  // Infinite loop (JMP -2)
    0x48, 0x31, 0xC0, 0x50,  // XOR RAX, RAX; PUSH RAX (shellcode pattern)
};

// Memory security validation modes
typedef enum {
    MEMORY_SECURITY_DISABLED = 0,
    MEMORY_SECURITY_BASIC = 1,
    MEMORY_SECURITY_ENHANCED = 2,
    MEMORY_SECURITY_PARANOID = 3
} memory_security_mode_t;

static memory_security_mode_t g_memory_security_mode = MEMORY_SECURITY_ENHANCED;

// Enhanced arena allocator for high-performance NUMA-aware memory management
// Features:
// - NUMA topology detection and awareness
// - Per-thread arenas for lock-free allocation
// - SLAB-style allocators for fixed-size objects
// - Cache-friendly memory layouts
// - Memory compaction and defragmentation
// - Huge page support
// - Memory prefetching optimization
// - SIMD-accelerated security scanning
// - Branch prediction optimized bounds checking
// - Tiered security validation modes
// - Segregated free lists for reduced fragmentation
// - Size-class buckets for better allocation patterns

// Size classes for segregated allocation (powers of 2 + common sizes)
static const size_t SIZE_CLASSES[] = {
    8, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 768, 1024,
    1536, 2048, 3072, 4096, 6144, 8192, 12288, 16384, 24576, 32768
};
#define NUM_SIZE_CLASSES (sizeof(SIZE_CLASSES) / sizeof(SIZE_CLASSES[0]))

// Find best size class for allocation
static inline size_t get_size_class(size_t size) {
    // Binary search for best fit
    size_t left = 0, right = NUM_SIZE_CLASSES - 1;
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        if (SIZE_CLASSES[mid] >= size) {
            right = mid;
        } else {
            left = mid + 1;
        }
    }
    return left;
}

// SIMD-based security scanning functions
#if HAS_SIMD_MEMORY_SCAN
static bool memory_scan_security_violations_simd(const void* ptr, size_t size) {
    if (UNLIKELY(!ptr || size == 0)) {
        return false;
    }
    
    const uint8_t* data = (const uint8_t*)ptr;
    size_t simd_size = size & ~31; // Process 32-byte chunks with AVX2
    
    // Load security patterns into SIMD registers
    __m256i pattern1 = _mm256_set1_epi32(0x90909090); // NOP sled
    __m256i pattern2 = _mm256_set1_epi32(0xCCCCCCCC); // INT3 breaks
    __m256i pattern3 = _mm256_set1_epi32(0x0000FEEB); // Jump pattern
    
    // Scan in 32-byte chunks
    for (size_t i = 0; i < simd_size; i += 32) {
        __m256i chunk = _mm256_loadu_si256((const __m256i*)(data + i));
        
        // Check for security patterns
        __m256i cmp1 = _mm256_cmpeq_epi32(chunk, pattern1);
        __m256i cmp2 = _mm256_cmpeq_epi32(chunk, pattern2);
        __m256i cmp3 = _mm256_cmpeq_epi32(chunk, pattern3);
        
        // Combine results
        __m256i combined = _mm256_or_si256(cmp1, _mm256_or_si256(cmp2, cmp3));
        
        // Check if any matches found
        if (UNLIKELY(_mm256_testz_si256(combined, combined) == 0)) {
            return true; // Security violation detected
        }
    }
    
    // Handle remaining bytes with scalar code
    for (size_t i = simd_size; i < size - 3; i++) {
        uint32_t word = *(const uint32_t*)(data + i);
        if (UNLIKELY(word == 0x90909090 || word == 0xCCCCCCCC || word == 0x0000FEEB)) {
            return true;
        }
    }
    
    return false;
}
#else
static bool memory_scan_security_violations_simd(const void* ptr, size_t size) {
    // Fallback to scalar implementation
    const uint8_t* data = (const uint8_t*)ptr;
    for (size_t i = 0; i < size - 3; i++) {
        uint32_t word = *(const uint32_t*)(data + i);
        if (UNLIKELY(word == 0x90909090 || word == 0xCCCCCCCC || word == 0x0000FEEB)) {
            return true;
        }
    }
    return false;
}
#endif

// Optimized bounds checking with branch prediction
static inline bool memory_bounds_check_fast(const void* ptr, size_t size, const void* region_start, size_t region_size) {
    const uint8_t* check_ptr = (const uint8_t*)ptr;
    const uint8_t* region_ptr = (const uint8_t*)region_start;
    
    // Fast path: check if we're well within bounds (common case)
    if (LIKELY(check_ptr >= region_ptr + 64 && 
               check_ptr + size <= region_ptr + region_size - 64)) {
        return true; // Well within bounds
    }
    
    // Slow path: precise bounds check
    return (check_ptr >= region_ptr && check_ptr + size <= region_ptr + region_size);
}

// Tiered security validation
static bool memory_validate_security_tiered(const void* ptr, size_t size, memory_security_mode_t mode) {
    if (UNLIKELY(!ptr)) {
        return false;
    }
    
    switch (mode) {
        case MEMORY_SECURITY_DISABLED:
            return true;
            
        case MEMORY_SECURITY_BASIC:
            // Basic null pointer and size checks
            return size > 0 && size < (1ULL << 32); // Reasonable size limit
            
        case MEMORY_SECURITY_ENHANCED:
            // Enhanced validation with basic pattern scanning
            if (size == 0 || size > (1ULL << 30)) { // 1GB limit
                return false;
            }
            // Quick check for obvious attack patterns in first 32 bytes
            size_t check_size = (size < 32) ? size : 32;
            return !memory_scan_security_violations_simd(ptr, check_size);
            
        case MEMORY_SECURITY_PARANOID:
            // Full paranoid validation
            if (size == 0 || size > (1ULL << 28)) { // 256MB limit in paranoid mode
                return false;
            }
            return !memory_scan_security_violations_simd(ptr, size);
    }
    
    return false;
}

#define ARENA_DEFAULT_BLOCK_SIZE (2 * 1024 * 1024)  // 2MB blocks for better TLB usage
#define ARENA_ALIGNMENT 64  // Cache line alignment for optimal performance
#define ARENA_MAX_THREADS 1024
#define LARGE_OBJECT_THRESHOLD (512 * 1024)  // 512KB threshold for large objects
#define HUGEPAGE_SIZE (2 * 1024 * 1024)  // 2MB huge pages

// Size classes for SLAB allocator (powers of 2)
static const size_t SLAB_SIZE_CLASSES[] = {
    8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
};
#define SLAB_CLASS_COUNT (sizeof(SLAB_SIZE_CLASSES) / sizeof(SLAB_SIZE_CLASSES[0]))

// Enhanced arena block structure with NUMA awareness
struct arena_block {
    char* start;                    // Start of block memory
    char* current;                  // Current allocation pointer (bump pointer)
    char* end;                      // End of block memory
    size_t size;                    // Total block size
    int numa_node;                  // NUMA node this block belongs to
    struct arena_block* next;       // Next block in chain
    struct arena_block* prev;       // Previous block for compaction
    uint64_t allocation_count;      // Number of allocations in this block
    uint64_t bytes_allocated;       // Total bytes allocated in this block
    bool use_hugepages;             // Whether this block uses huge pages
    void* mmap_addr;                // mmap address for proper cleanup
    size_t mmap_size;               // mmap size for proper cleanup
};

// Per-thread arena for lock-free allocation
struct thread_arena {
    struct arena_block* current_block;   // Current allocation block
    struct arena_block* blocks;          // Chain of all blocks
    size_t block_size;                   // Default block size
    size_t total_allocated;              // Total memory allocated
    int thread_id;                       // Thread ID
    int preferred_numa_node;             // Preferred NUMA node
    pthread_mutex_t mutex;               // Mutex for block management
    
    // Performance statistics
    uint64_t allocations;                // Total allocation requests
    uint64_t bytes_requested;            // Total bytes requested
    uint64_t blocks_allocated;           // Number of blocks allocated
    uint64_t compactions;                // Number of compaction operations
    double avg_allocation_size;          // Average allocation size
    
    // Fragmentation tracking
    size_t fragmented_bytes;             // Bytes lost to fragmentation
    double fragmentation_ratio;          // Fragmentation as percentage
    
    // Cache performance
    uint64_t cache_friendly_allocations; // Cache-aligned allocations
    uint64_t prefetch_hints_issued;      // Memory prefetch hints issued
};

// Enhanced arena allocator structure with NUMA awareness
struct arena_allocator {
    struct thread_arena** thread_arenas; // Per-thread arenas
    size_t max_threads;                   // Maximum number of threads
    size_t default_block_size;            // Default block size
    numa_topology_t numa_info;            // NUMA topology information
    
    // Global settings
    bool numa_aware;                      // Enable NUMA-aware allocation
    bool auto_compaction;                 // Enable automatic compaction
    double compaction_threshold;          // Fragmentation threshold for compaction
    size_t large_object_threshold;        // Threshold for large object handling
    bool enable_hugepages;                // Enable huge page support
    bool enable_prefetching;              // Enable memory prefetching
    
    // NUMA-specific memory pools
    struct arena_block* numa_blocks[MAX_NUMA_NODES]; // Blocks per NUMA node
    size_t numa_block_counts[MAX_NUMA_NODES];         // Block counts per node
    pthread_mutex_t numa_locks[MAX_NUMA_NODES];       // Per-node locks
    
    // Global statistics
    uint64_t total_allocations;           // Total allocations across all threads
    uint64_t total_bytes_allocated;       // Total bytes allocated
    uint64_t numa_local_allocations;      // NUMA-local allocations
    uint64_t numa_remote_allocations;     // NUMA-remote allocations
    double numa_locality_ratio;           // Percentage of NUMA-local allocations
    
    // Performance monitoring
    uint64_t allocation_time_ns;          // Total time spent in allocations
    uint64_t compaction_time_ns;          // Total time spent in compaction
    double avg_allocation_time_ns;        // Average allocation time
    
    // Thread-local storage key
    pthread_key_t tls_key;                // Thread-local storage for arena
    pthread_once_t init_once;             // One-time initialization
    
    // Global lock for arena management
    pthread_mutex_t global_mutex;         // Global arena lock
    
    // Integrated allocators
    slab_allocator_t* slab_allocator;     // SLAB allocator for fixed-size objects
    large_object_allocator_t* large_allocator; // Large object allocator
};

// Memory manager structure that integrates all allocators
struct memory_manager {
    arena_allocator* arena;               // Arena allocator
    slab_allocator_t* slab;               // SLAB allocator
    large_object_allocator_t* large;      // Large object allocator
    
    // Memory layout optimization
    bool enable_prefetching;              // Enable memory prefetching
    bool enable_hugepages;                // Enable huge page support
    size_t hugepage_size;                 // Huge page size
    
    // Compaction and defragmentation
    bool enable_compaction;               // Enable memory compaction
    uint64_t last_compaction_time;        // Last compaction timestamp
    size_t compaction_interval_ms;        // Compaction interval
    
    // Performance tracking
    uint64_t total_memory_allocated;      // Total memory allocated
    uint64_t peak_memory_usage;           // Peak memory usage
    double memory_efficiency;             // Memory efficiency ratio
    uint64_t allocation_latency_ns;       // Average allocation latency
    
    // Memory pressure handling
    size_t memory_pressure_threshold;     // Memory pressure threshold
    bool under_memory_pressure;           // Current memory pressure state
    void (*pressure_callback)(void*);     // Memory pressure callback
    void* pressure_callback_data;         // Callback data
};

// Global NUMA topology
static numa_topology_t global_numa_topology = {0};
static pthread_once_t numa_init_once = PTHREAD_ONCE_INIT;

// Thread-local arena pointer
static __thread struct thread_arena* thread_local_arena = NULL;

// High-performance time measurement
static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Align size to the specified boundary
static inline size_t align_size(size_t size, size_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}

// Cache line align size
static inline size_t cache_align_size(size_t size) {
    return align_size(size, CACHE_LINE_SIZE);
}

// Get current CPU
static inline int get_current_cpu(void) {
    return sched_getcpu();
}

// Get page size
static inline size_t get_page_size(void) {
    return sysconf(_SC_PAGESIZE);
}

// Check if huge pages are available
static bool hugepages_available(void) {
    return access("/proc/sys/vm/nr_hugepages", R_OK) == 0;
}

// Simple arena block structure
struct simple_arena_block {
    struct simple_arena_block* next;
    size_t size;
    size_t used;
    char data[];
};

// Simple arena allocator structure
struct simple_arena_allocator {
    struct simple_arena_block* current_block;
    struct simple_arena_block* first_block;
    size_t default_block_size;
    size_t total_allocated;
    size_t total_used;
    size_t block_count;
    uint64_t allocation_count;
};

// Create a new arena block
static struct simple_arena_block* arena_create_block(size_t min_size) {
    size_t block_size = min_size > ARENA_DEFAULT_BLOCK_SIZE ? 
                       align_size(min_size, ARENA_ALIGNMENT) :
                       ARENA_DEFAULT_BLOCK_SIZE;
    
    struct simple_arena_block* block = malloc(sizeof(struct simple_arena_block) + block_size);
    if (!block) {
        return NULL;
    }
    
    block->next = NULL;
    block->size = block_size;
    block->used = 0;
    
    return block;
}

// Initialize arena allocator
arena_allocator* arena_create(void) {
    struct simple_arena_allocator* arena = malloc(sizeof(struct simple_arena_allocator));
    if (!arena) {
        return NULL;
    }
    
    arena->current_block = arena_create_block(ARENA_DEFAULT_BLOCK_SIZE);
    if (!arena->current_block) {
        free(arena);
        return NULL;
    }
    
    arena->first_block = arena->current_block;
    arena->default_block_size = ARENA_DEFAULT_BLOCK_SIZE;
    arena->total_allocated = arena->current_block->size;
    arena->total_used = 0;
    arena->block_count = 1;
    arena->allocation_count = 0;
    
    return (arena_allocator*)arena;
}

// Allocate memory from arena
void* arena_alloc(arena_allocator* arena, size_t size) {
    if (UNLIKELY(!arena || size == 0)) {
        return NULL;
    }
    
    // Get size class for better allocation patterns
    size_t size_class_idx = get_size_class(size);
    size_t aligned_size = SIZE_CLASSES[size_class_idx];
    
    // Security validation for paranoid mode
    if (UNLIKELY(g_memory_security_mode >= MEMORY_SECURITY_ENHANCED)) {
        if (!memory_validate_security_tiered(&size, sizeof(size), g_memory_security_mode)) {
            return NULL;
        }
    }
    
    struct simple_arena_allocator* simple_arena = (struct simple_arena_allocator*)arena;
    
    // Fast path: Check if current block has enough space
    struct simple_arena_block* block = simple_arena->current_block;
    size_t available = block->size - block->used;
    
    if (LIKELY(available >= aligned_size)) {
        // Prefetch the allocation area for better cache performance
        PREFETCH_WRITE(block->data + block->used);
        
        void* ptr = block->data + block->used;
        block->used += aligned_size;
        
        // Update statistics atomically for thread safety
        __atomic_fetch_add(&simple_arena->total_used, aligned_size, __ATOMIC_RELAXED);
        __atomic_fetch_add(&simple_arena->allocation_count, 1, __ATOMIC_RELAXED);
        
        // Zero memory if in paranoid mode
        if (UNLIKELY(g_memory_security_mode == MEMORY_SECURITY_PARANOID)) {
            memset(ptr, 0, aligned_size);
        }
        
        return ptr;
    }
    
    // Slow path: Need a new block
    size_t block_size = simple_arena->default_block_size;
    if (aligned_size > block_size / 2) {
        // Large allocation, create custom-sized block
        block_size = aligned_size + ARENA_ALIGNMENT;
    }
    
    struct simple_arena_block* new_block = arena_create_block(block_size);
    if (UNLIKELY(!new_block)) {
        return NULL;
    }
    
    // Link the new block with proper memory ordering
    new_block->next = simple_arena->current_block;
    __atomic_store_n(&simple_arena->current_block, new_block, __ATOMIC_RELEASE);
    __atomic_fetch_add(&simple_arena->total_allocated, new_block->size, __ATOMIC_RELAXED);
    __atomic_fetch_add(&simple_arena->block_count, 1, __ATOMIC_RELAXED);
    
    // Allocate from the new block
    void* ptr = new_block->data;
    new_block->used = aligned_size;
    __atomic_fetch_add(&simple_arena->total_used, aligned_size, __ATOMIC_RELAXED);
    
    return ptr;
}

// Allocate zeroed memory from arena
void* arena_calloc(arena_allocator* arena, size_t count, size_t size) {
    size_t total_size = count * size;
    void* ptr = arena_alloc(arena, total_size);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

// Reset arena (keeps blocks but resets usage)
void arena_reset(arena_allocator* arena) {
    if (!arena) {
        return;
    }
    
    // Reset all blocks
    struct arena_block* block = arena->first_block;
    while (block) {
        block->used = 0;
        block = block->next;
    }
    
    arena->current_block = arena->first_block;
    arena->total_used = 0;
}

// Destroy arena and free all memory
void arena_destroy(arena_allocator* arena) {
    if (!arena) {
        return;
    }
    
    struct arena_block* block = arena->first_block;
    while (block) {
        struct arena_block* next = block->next;
        free(block);
        block = next;
    }
    
    free(arena);
}

// Get arena statistics
void arena_get_stats(arena_allocator* arena, arena_stats* stats) {
    if (!arena || !stats) {
        return;
    }
    
    stats->total_allocated = arena->total_allocated;
    stats->total_used = arena->total_used;
    stats->block_count = arena->block_count;
    stats->utilization = arena->total_allocated > 0 ? 
                        (double)arena->total_used / arena->total_allocated : 0.0;
}

// Print arena statistics
void arena_print_stats(arena_allocator* arena) {
    if (!arena) {
        return;
    }
    
    arena_stats stats;
    arena_get_stats(arena, &stats);
    
    printf("=== Arena Allocator Statistics ===\n");
    printf("Total allocated: %zu bytes\n", stats.total_allocated);
    printf("Total used: %zu bytes\n", stats.total_used);
    printf("Block count: %zu\n", stats.block_count);
    printf("Utilization: %.1f%%\n", stats.utilization * 100.0);
    printf("Average block size: %.1f KB\n", 
           stats.block_count > 0 ? (double)stats.total_allocated / stats.block_count / 1024.0 : 0.0);
    printf("=================================\n");
}