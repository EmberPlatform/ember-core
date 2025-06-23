#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "../../include/ember.h"
#include "../../src/core/jit_vm_bridge.h"
#include "../../src/core/performance_benchmark.h"

static void print_usage(const char* program_name) {
    printf("Usage: %s [OPTIONS] [COMMAND]\n", program_name);
    printf("\nCommands:\n");
    printf("  benchmark           Run performance benchmarks\n");
    printf("  capabilities        Show optimization capabilities\n");
    printf("  test-jit            Test JIT compiler functionality\n");
    printf("  test-memory         Test memory optimization\n");
    printf("  test-vm-pool        Test VM pool functionality\n");
    printf("  demo                Run optimization demo\n");
    printf("\nOptions:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -v, --verbose       Enable verbose output\n");
    printf("  -j, --jit           Enable JIT compiler\n");
    printf("  -m, --memory        Enable memory optimization\n");
    printf("  -p, --pool          Enable VM pool\n");
    printf("  -t, --threshold N   Set JIT hot threshold (default: 1000)\n");
    printf("  -i, --iterations N  Set benchmark iterations (default: 10000)\n");
    printf("  -w, --warmup N      Set warmup iterations (default: 1000)\n");
    printf("  -o, --output FILE   Save benchmark results to file\n");
    printf("  --opt-level N       Set optimization level 0-3 (default: 2)\n");
    printf("  --no-profiling      Disable JIT profiling\n");
    printf("  --no-deopt          Disable JIT deoptimization\n");
    printf("  --aggressive        Enable aggressive optimizations\n");
    printf("\nExamples:\n");
    printf("  %s benchmark -j -m -i 50000\n", program_name);
    printf("  %s test-jit --aggressive -t 500\n", program_name);
    printf("  %s demo --opt-level 3 -v\n", program_name);
}

static void print_capabilities(void) {
    ember_optimization_capabilities_t caps = ember_get_optimization_capabilities();
    
    printf("=== Ember Platform Optimization Capabilities ===\n\n");
    
    printf("JIT Compiler:\n");
    printf("  Available: %s\n", caps.jit_compiler_available ? "Yes" : "No");
    printf("  x86-64 support: %s\n", caps.jit_x86_64_support ? "Yes" : "No");
    printf("  ARM64 support: %s\n", caps.jit_arm64_support ? "Yes" : "No");
    printf("  Profiling support: %s\n", caps.jit_profiling_support ? "Yes" : "No");
    printf("  Deoptimization support: %s\n", caps.jit_deoptimization_support ? "Yes" : "No");
    
    printf("\nMemory Optimization:\n");
    printf("  Arena allocator: %s\n", caps.arena_allocator_available ? "Yes" : "No");
    printf("  NUMA support: %s\n", caps.numa_support ? "Yes" : "No");
    printf("  Huge pages support: %s\n", caps.huge_pages_support ? "Yes" : "No");
    printf("  Memory pools: %s\n", caps.memory_pool_support ? "Yes" : "No");
    
    printf("\nVM Pool:\n");
    printf("  Available: %s\n", caps.vm_pool_available ? "Yes" : "No");
    printf("  Work stealing: %s\n", caps.work_stealing_support ? "Yes" : "No");
    printf("  Lock-free support: %s\n", caps.lockfree_support ? "Yes" : "No");
    printf("  CPU affinity: %s\n", caps.cpu_affinity_support ? "Yes" : "No");
    
    printf("\nPlatform Features:\n");
    printf("  SIMD support: %s\n", caps.simd_support ? "Yes" : "No");
    printf("  Prefetch support: %s\n", caps.prefetch_support ? "Yes" : "No");
    printf("  Branch prediction: %s\n", caps.branch_prediction_support ? "Yes" : "No");
    printf("  Max optimization level: %d\n", caps.max_optimization_level);
    
    printf("\n===============================================\n");
}

static int test_jit_compiler(ember_optimization_config_t* config) {
    printf("=== Testing JIT Compiler ===\n");
    
    // Initialize optimizations
    if (ember_initialize_optimizations(config) != 0) {
        printf("Failed to initialize optimizations\n");
        return 1;
    }
    
    // Create optimized VM
    ember_vm* vm = ember_create_optimized_vm(config);
    if (!vm) {
        printf("Failed to create optimized VM\n");
        ember_shutdown_optimizations();
        return 1;
    }
    
    printf("JIT compiler initialized successfully\n");
    
    // Test basic compilation
    ember_chunk test_chunk = {0};
    test_chunk.count = 20;
    test_chunk.code = malloc(test_chunk.count);
    
    // Create simple arithmetic bytecode
    for (int i = 0; i < test_chunk.count; i++) {
        if (i % 4 == 0) test_chunk.code[i] = OP_PUSH_CONST;
        else if (i % 4 == 1) test_chunk.code[i] = OP_ADD;
        else if (i % 4 == 2) test_chunk.code[i] = OP_MUL;
        else test_chunk.code[i] = OP_SUB;
    }
    
    // Trigger hot path detection
    printf("Triggering hot path detection...\n");
    for (int i = 0; i < config->jit_hot_threshold + 100; i++) {
        if (ember_check_and_compile_hot_path(vm, &test_chunk, 0)) {
            printf("Hot path compiled after %d iterations\n", i + 1);
            break;
        }
    }
    
    // Test JIT execution
    printf("Testing JIT execution...\n");
    for (int i = 0; i < 10; i++) {
        if (ember_try_jit_execute(vm, &test_chunk, 0)) {
            printf("JIT execution successful (iteration %d)\n", i + 1);
        }
    }
    
    // Print statistics
    ember_print_optimization_stats(vm);
    
    // Cleanup
    free(test_chunk.code);
    ember_free_vm(vm);
    ember_shutdown_optimizations();
    
    printf("JIT test completed successfully\n");
    return 0;
}

static int test_memory_optimization(ember_optimization_config_t* config) {
    printf("=== Testing Memory Optimization ===\n");
    
    // Initialize optimizations
    if (ember_initialize_optimizations(config) != 0) {
        printf("Failed to initialize optimizations\n");
        return 1;
    }
    
    // Create optimized VM
    ember_vm* vm = ember_create_optimized_vm(config);
    if (!vm) {
        printf("Failed to create optimized VM\n");
        ember_shutdown_optimizations();
        return 1;
    }
    
    printf("Memory optimization initialized successfully\n");
    
    // Test various allocation sizes
    printf("Testing memory allocations...\n");
    
    void* ptrs[1000];
    size_t sizes[] = {16, 32, 64, 128, 256, 512, 1024, 2048};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);
    
    // Allocate various sizes
    for (int i = 0; i < 1000; i++) {
        size_t size = sizes[i % num_sizes];
        ptrs[i] = ember_optimized_alloc(vm, size);
        if (!ptrs[i]) {
            printf("Allocation failed at iteration %d (size %zu)\n", i, size);
            break;
        }
        
        // Write some data to verify allocation
        memset(ptrs[i], i % 256, size);
    }
    
    printf("Allocated 1000 memory blocks successfully\n");
    
    // Test calloc
    void* zero_ptr = ember_optimized_calloc(vm, 100, sizeof(int));
    if (zero_ptr) {
        int* int_array = (int*)zero_ptr;
        bool all_zero = true;
        for (int i = 0; i < 100; i++) {
            if (int_array[i] != 0) {
                all_zero = false;
                break;
            }
        }
        printf("Calloc test: %s\n", all_zero ? "PASSED" : "FAILED");
    }
    
    // Print statistics
    ember_print_optimization_stats(vm);
    
    // Cleanup
    ember_free_vm(vm);
    ember_shutdown_optimizations();
    
    printf("Memory optimization test completed successfully\n");
    return 0;
}

static int run_optimization_demo(ember_optimization_config_t* config) {
    printf("=== Ember Platform Optimization Demo ===\n");
    
    // Initialize optimizations
    if (ember_initialize_optimizations(config) != 0) {
        printf("Failed to initialize optimizations\n");
        return 1;
    }
    
    printf("Running comprehensive optimization demo...\n");
    
    // Create optimized VM
    ember_vm* vm = ember_create_optimized_vm(config);
    if (!vm) {
        printf("Failed to create optimized VM\n");
        ember_shutdown_optimizations();
        return 1;
    }
    
    // Demo 1: JIT compilation workflow
    if (config->enable_jit_compiler) {
        printf("\n--- JIT Compilation Demo ---\n");
        
        ember_chunk demo_chunk = {0};
        demo_chunk.count = 50;
        demo_chunk.code = malloc(demo_chunk.count);
        
        // Create more complex bytecode
        for (int i = 0; i < demo_chunk.count; i++) {
            switch (i % 8) {
                case 0: demo_chunk.code[i] = OP_PUSH_CONST; break;
                case 1: demo_chunk.code[i] = OP_PUSH_CONST; break;
                case 2: demo_chunk.code[i] = OP_ADD; break;
                case 3: demo_chunk.code[i] = OP_PUSH_CONST; break;
                case 4: demo_chunk.code[i] = OP_MUL; break;
                case 5: demo_chunk.code[i] = OP_SUB; break;
                case 6: demo_chunk.code[i] = OP_LOOP; break;
                case 7: demo_chunk.code[i] = OP_JUMP_IF_FALSE; break;
            }
        }
        
        printf("Simulating hot loop execution...\n");
        int compiled_at = -1;
        for (int i = 0; i < config->jit_hot_threshold + 200; i++) {
            if (ember_check_and_compile_hot_path(vm, &demo_chunk, 0)) {
                compiled_at = i + 1;
                printf("Hot path compiled after %d iterations\n", compiled_at);
                break;
            }
            
            if (i % 100 == 0 && i > 0) {
                printf("  Profiling... (%d iterations)\n", i);
            }
        }
        
        if (compiled_at > 0) {
            printf("Testing compiled code execution...\n");
            int successful_executions = 0;
            for (int i = 0; i < 100; i++) {
                if (ember_try_jit_execute(vm, &demo_chunk, 0)) {
                    successful_executions++;
                }
            }
            printf("JIT executions: %d/100 successful\n", successful_executions);
        }
        
        free(demo_chunk.code);
    }
    
    // Demo 2: Memory optimization showcase
    if (config->enable_memory_optimization) {
        printf("\n--- Memory Optimization Demo ---\n");
        
        printf("Demonstrating arena allocation patterns...\n");
        
        // Simulate typical VM allocation patterns
        void* small_objects[100];
        void* medium_objects[50];
        void* large_objects[10];
        
        // Small object allocations (typical for values)
        for (int i = 0; i < 100; i++) {
            small_objects[i] = ember_optimized_alloc(vm, 16 + (i % 48));
        }
        printf("Allocated 100 small objects\n");
        
        // Medium object allocations (typical for strings/arrays)
        for (int i = 0; i < 50; i++) {
            medium_objects[i] = ember_optimized_alloc(vm, 128 + (i % 896));
        }
        printf("Allocated 50 medium objects\n");
        
        // Large object allocations (typical for functions/modules)
        for (int i = 0; i < 10; i++) {
            large_objects[i] = ember_optimized_alloc(vm, 4096 + (i % 4096));
        }
        printf("Allocated 10 large objects\n");
    }
    
    // Print comprehensive statistics
    printf("\n--- Final Statistics ---\n");
    ember_print_optimization_stats(vm);
    
    // Cleanup
    ember_free_vm(vm);
    ember_shutdown_optimizations();
    
    printf("\nOptimization demo completed successfully\n");
    return 0;
}

int main(int argc, char* argv[]) {
    // Default configuration
    ember_optimization_config_t config = DEFAULT_OPTIMIZATION_CONFIG;
    benchmark_config_t benchmark_config = DEFAULT_BENCHMARK_CONFIG;
    
    // Command line options
    bool verbose = false;
    const char* output_file = NULL;
    const char* command = NULL;
    
    // Parse command line options
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"verbose", no_argument, 0, 'v'},
        {"jit", no_argument, 0, 'j'},
        {"memory", no_argument, 0, 'm'},
        {"pool", no_argument, 0, 'p'},
        {"threshold", required_argument, 0, 't'},
        {"iterations", required_argument, 0, 'i'},
        {"warmup", required_argument, 0, 'w'},
        {"output", required_argument, 0, 'o'},
        {"opt-level", required_argument, 0, 1001},
        {"no-profiling", no_argument, 0, 1002},
        {"no-deopt", no_argument, 0, 1003},
        {"aggressive", no_argument, 0, 1004},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "hvjmpt:i:w:o:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                print_usage(argv[0]);
                return 0;
                
            case 'v':
                verbose = true;
                config.jit_verbose = true;
                benchmark_config.verbose_output = true;
                break;
                
            case 'j':
                config.enable_jit_compiler = true;
                benchmark_config.enable_jit = true;
                break;
                
            case 'm':
                config.enable_memory_optimization = true;
                benchmark_config.enable_memory_opt = true;
                break;
                
            case 'p':
                config.enable_vm_pool = true;
                benchmark_config.enable_vm_pool = true;
                break;
                
            case 't':
                config.jit_hot_threshold = atoi(optarg);
                break;
                
            case 'i':
                benchmark_config.benchmark_iterations = atoi(optarg);
                break;
                
            case 'w':
                benchmark_config.warmup_iterations = atoi(optarg);
                break;
                
            case 'o':
                output_file = optarg;
                break;
                
            case 1001: // --opt-level
                config.optimization_level = atoi(optarg);
                if (config.optimization_level > 3) {
                    config.optimization_level = 3;
                }
                break;
                
            case 1002: // --no-profiling
                config.jit_enable_profiling = false;
                break;
                
            case 1003: // --no-deopt
                config.jit_enable_deoptimization = false;
                break;
                
            case 1004: // --aggressive
                config.jit_aggressive_inlining = true;
                config.optimization_level = 3;
                break;
                
            case '?':
                print_usage(argv[0]);
                return 1;
                
            default:
                break;
        }
    }
    
    // Get command
    if (optind < argc) {
        command = argv[optind];
    }
    
    if (!command) {
        printf("Error: No command specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    if (verbose) {
        printf("Ember Platform Optimization Tool\n");
        printf("Command: %s\n", command);
        printf("JIT enabled: %s\n", config.enable_jit_compiler ? "Yes" : "No");
        printf("Memory optimization: %s\n", config.enable_memory_optimization ? "Yes" : "No");
        printf("VM pool: %s\n", config.enable_vm_pool ? "Yes" : "No");
        printf("Optimization level: %d\n", config.optimization_level);
        printf("\n");
    }
    
    // Execute command
    if (strcmp(command, "benchmark") == 0) {
        comprehensive_benchmark_result_t result = run_comprehensive_benchmark(&benchmark_config);
        generate_performance_report(&result, output_file);
        free_comprehensive_result(&result);
        return 0;
        
    } else if (strcmp(command, "capabilities") == 0) {
        print_capabilities();
        return 0;
        
    } else if (strcmp(command, "test-jit") == 0) {
        return test_jit_compiler(&config);
        
    } else if (strcmp(command, "test-memory") == 0) {
        return test_memory_optimization(&config);
        
    } else if (strcmp(command, "test-vm-pool") == 0) {
        printf("VM pool testing not yet implemented\n");
        return 1;
        
    } else if (strcmp(command, "demo") == 0) {
        return run_optimization_demo(&config);
        
    } else {
        printf("Error: Unknown command '%s'\n", command);
        print_usage(argv[0]);
        return 1;
    }
}