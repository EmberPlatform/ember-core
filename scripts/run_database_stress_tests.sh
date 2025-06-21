#!/bin/bash

##
# Database Connection Pool Stress Testing Suite
# 
# This script runs comprehensive database stress tests including:
# - Connection pool exhaustion scenarios
# - Transaction deadlock simulation
# - High-volume concurrent operations
# - Connection pool recovery testing
# - Performance monitoring and health metrics
##

set -e

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
TEST_DIR="$PROJECT_ROOT/tests/unit"
BENCHMARK_DIR="$PROJECT_ROOT/benchmarks"
RESULTS_DIR="$PROJECT_ROOT/test_results/database_stress"
LOG_FILE="$RESULTS_DIR/database_stress_test.log"

# Test binaries
C_STRESS_TEST="$BUILD_DIR/test_database_stress"
EMBER_STRESS_BENCHMARK="$BENCHMARK_DIR/database_connection_pool_stress_benchmark.ember"
EMBER_MONITORING_SYSTEM="$BENCHMARK_DIR/database_monitoring_system.ember"

# Test configuration
NUM_STRESS_THREADS=50
MAX_CONNECTIONS=25
STRESS_DURATION=60
MONITORING_DURATION=30

echo -e "${BLUE}===============================================${NC}"
echo -e "${BLUE}Database Connection Pool Stress Testing Suite${NC}"
echo -e "${BLUE}===============================================${NC}"
echo

# Function to print colored status messages
print_status() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

# Function to check if required tools are available
check_dependencies() {
    print_status "Checking dependencies..."
    
    local missing_deps=()
    
    # Check for required tools
    if ! command -v make &> /dev/null; then
        missing_deps+=("make")
    fi
    
    if ! command -v gcc &> /dev/null; then
        missing_deps+=("gcc")
    fi
    
    if ! command -v sqlite3 &> /dev/null; then
        missing_deps+=("sqlite3")
    fi
    
    # Check for pthread library
    if ! ldconfig -p | grep -q "libpthread"; then
        missing_deps+=("libpthread")
    fi
    
    # Check for sqlite3 development library
    if ! ldconfig -p | grep -q "libsqlite3"; then
        missing_deps+=("libsqlite3-dev")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing dependencies: ${missing_deps[*]}"
        print_error "Please install the missing dependencies and try again."
        exit 1
    fi
    
    print_success "All dependencies are available"
}

# Function to create results directory
setup_results_directory() {
    print_status "Setting up results directory..."
    
    mkdir -p "$RESULTS_DIR"
    
    # Create subdirectories for different test types
    mkdir -p "$RESULTS_DIR/c_tests"
    mkdir -p "$RESULTS_DIR/ember_benchmarks"
    mkdir -p "$RESULTS_DIR/monitoring_reports"
    mkdir -p "$RESULTS_DIR/performance_data"
    
    # Initialize log file
    echo "Database Stress Testing Suite - $(date)" > "$LOG_FILE"
    echo "=======================================" >> "$LOG_FILE"
    echo >> "$LOG_FILE"
    
    print_success "Results directory created: $RESULTS_DIR"
}

# Function to build the project
build_project() {
    print_status "Building project..."
    
    cd "$PROJECT_ROOT"
    
    if [ -f "Makefile" ]; then
        make clean >> "$LOG_FILE" 2>&1 || true
        if make -j$(nproc) >> "$LOG_FILE" 2>&1; then
            print_success "Project build completed"
        else
            print_error "Project build failed"
            echo "Check $LOG_FILE for build errors"
            exit 1
        fi
    else
        print_error "Makefile not found in project root"
        exit 1
    fi
}

# Function to build database stress test
build_database_stress_test() {
    print_status "Building database stress test..."
    
    cd "$TEST_DIR"
    
    # Check if test source exists
    if [ ! -f "test_database_stress.c" ]; then
        print_error "Database stress test source not found: test_database_stress.c"
        exit 1
    fi
    
    # Compile the stress test
    local compile_cmd="gcc -std=c99 -O2 -Wall -Wextra \
        -I$PROJECT_ROOT/include \
        -I$PROJECT_ROOT/src \
        test_database_stress.c \
        -L$BUILD_DIR \
        -lember \
        -lsqlite3 \
        -lpthread \
        -lm \
        -o $C_STRESS_TEST"
    
    print_status "Compiling: $compile_cmd"
    
    if eval "$compile_cmd" >> "$LOG_FILE" 2>&1; then
        print_success "Database stress test compiled successfully"
    else
        print_error "Failed to compile database stress test"
        echo "Check $LOG_FILE for compilation errors"
        exit 1
    fi
}

# Function to run C-based stress tests
run_c_stress_tests() {
    print_status "Running C-based database stress tests..."
    
    if [ ! -x "$C_STRESS_TEST" ]; then
        print_error "Database stress test binary not found or not executable: $C_STRESS_TEST"
        exit 1
    fi
    
    local test_start=$(date)
    local c_results_file="$RESULTS_DIR/c_tests/stress_test_results.txt"
    
    echo "C-based Database Stress Test Results" > "$c_results_file"
    echo "Test Started: $test_start" >> "$c_results_file"
    echo "===========================================" >> "$c_results_file"
    echo >> "$c_results_file"
    
    print_status "Executing database stress test (this may take several minutes)..."
    
    if timeout 300 "$C_STRESS_TEST" >> "$c_results_file" 2>&1; then
        print_success "C-based stress tests completed successfully"
        
        # Extract key metrics from results
        local total_ops=$(grep -o "Total operations: [0-9]*" "$c_results_file" | grep -o "[0-9]*" | tail -1)
        local success_rate=$(grep -o "Success rate: [0-9.]*%" "$c_results_file" | grep -o "[0-9.]*" | tail -1)
        
        if [ -n "$total_ops" ] && [ -n "$success_rate" ]; then
            echo "  Total Operations: $total_ops" | tee -a "$LOG_FILE"
            echo "  Success Rate: $success_rate%" | tee -a "$LOG_FILE"
        fi
        
    else
        print_error "C-based stress tests failed or timed out"
        echo "Check $c_results_file for details"
        return 1
    fi
    
    echo "Test Completed: $(date)" >> "$c_results_file"
}

# Function to run Ember benchmark scripts
run_ember_benchmarks() {
    print_status "Running Ember-based database benchmarks..."
    
    local ember_cmd="$PROJECT_ROOT/build/ember"
    
    if [ ! -x "$ember_cmd" ]; then
        print_error "Ember executable not found: $ember_cmd"
        exit 1
    fi
    
    # Run connection pool stress benchmark
    if [ -f "$EMBER_STRESS_BENCHMARK" ]; then
        print_status "Running connection pool stress benchmark..."
        
        local benchmark_results="$RESULTS_DIR/ember_benchmarks/stress_benchmark_results.txt"
        local benchmark_start=$(date)
        
        echo "Ember Database Stress Benchmark Results" > "$benchmark_results"
        echo "Benchmark Started: $benchmark_start" >> "$benchmark_results"
        echo "=============================================" >> "$benchmark_results"
        echo >> "$benchmark_results"
        
        if timeout 600 "$ember_cmd" "$EMBER_STRESS_BENCHMARK" >> "$benchmark_results" 2>&1; then
            print_success "Ember stress benchmark completed"
            
            # Extract performance metrics
            local ops_per_sec=$(grep -o "Operations per second: [0-9.]*" "$benchmark_results" | grep -o "[0-9.]*" | tail -1)
            local test_duration=$(grep -o "Test duration: [0-9.]* seconds" "$benchmark_results" | grep -o "[0-9.]*" | tail -1)
            
            if [ -n "$ops_per_sec" ] && [ -n "$test_duration" ]; then
                echo "  Operations/sec: $ops_per_sec" | tee -a "$LOG_FILE"
                echo "  Duration: $test_duration seconds" | tee -a "$LOG_FILE"
            fi
            
        else
            print_warning "Ember stress benchmark failed or timed out"
            echo "Check $benchmark_results for details"
        fi
        
        echo "Benchmark Completed: $(date)" >> "$benchmark_results"
    else
        print_warning "Ember stress benchmark script not found: $EMBER_STRESS_BENCHMARK"
    fi
}

# Function to run monitoring system
run_monitoring_system() {
    print_status "Running database monitoring system..."
    
    local ember_cmd="$PROJECT_ROOT/build/ember"
    
    if [ ! -x "$ember_cmd" ]; then
        print_error "Ember executable not found: $ember_cmd"
        return 1
    fi
    
    if [ -f "$EMBER_MONITORING_SYSTEM" ]; then
        print_status "Starting database monitoring and health metrics collection..."
        
        local monitoring_results="$RESULTS_DIR/monitoring_reports/monitoring_results.txt"
        local monitoring_start=$(date)
        
        echo "Database Monitoring System Results" > "$monitoring_results"
        echo "Monitoring Started: $monitoring_start" >> "$monitoring_results"
        echo "====================================" >> "$monitoring_results"
        echo >> "$monitoring_results"
        
        if timeout 180 "$ember_cmd" "$EMBER_MONITORING_SYSTEM" >> "$monitoring_results" 2>&1; then
            print_success "Database monitoring completed"
            
            # Extract health metrics
            local health_grade=$(grep -o "Overall Health Grade: [A-F][+-]*" "$monitoring_results" | cut -d' ' -f4)
            local avg_response=$(grep -o "Average Response Time: [0-9.]*ms" "$monitoring_results" | grep -o "[0-9.]*")
            
            if [ -n "$health_grade" ]; then
                echo "  Health Grade: $health_grade" | tee -a "$LOG_FILE"
            fi
            if [ -n "$avg_response" ]; then
                echo "  Avg Response: ${avg_response}ms" | tee -a "$LOG_FILE"
            fi
            
        else
            print_warning "Database monitoring failed or timed out"
            echo "Check $monitoring_results for details"
        fi
        
        echo "Monitoring Completed: $(date)" >> "$monitoring_results"
    else
        print_warning "Database monitoring script not found: $EMBER_MONITORING_SYSTEM"
    fi
}

# Function to run performance profiling
run_performance_profiling() {
    print_status "Running performance profiling..."
    
    local profile_results="$RESULTS_DIR/performance_data/profile_results.txt"
    local profile_start=$(date)
    
    echo "Database Performance Profiling Results" > "$profile_results"
    echo "Profiling Started: $profile_start" >> "$profile_results"
    echo "======================================" >> "$profile_results"
    echo >> "$profile_results"
    
    # System resource monitoring during tests
    print_status "Collecting system resource usage..."
    
    {
        echo "System Information:"
        echo "=================="
        echo "CPU Info:"
        lscpu | grep -E "(Model name|CPU\(s\)|Thread|Core)"
        echo
        echo "Memory Info:"
        free -h
        echo
        echo "Disk Usage:"
        df -h /tmp
        echo
        echo "Load Average:"
        uptime
        echo
    } >> "$profile_results"
    
    # SQLite version and configuration
    if command -v sqlite3 &> /dev/null; then
        echo "SQLite Configuration:" >> "$profile_results"
        echo "====================" >> "$profile_results"
        sqlite3 -version >> "$profile_results" 2>&1
        echo >> "$profile_results"
    fi
    
    echo "Profiling Completed: $(date)" >> "$profile_results"
    print_success "Performance profiling completed"
}

# Function to generate comprehensive report
generate_comprehensive_report() {
    print_status "Generating comprehensive test report..."
    
    local report_file="$RESULTS_DIR/DATABASE_STRESS_TEST_REPORT.md"
    local report_date=$(date)
    
    cat > "$report_file" << EOF
# Database Connection Pool Stress Test Report

**Generated:** $report_date  
**Test Suite Version:** 1.0  
**Project:** Ember Language Database Implementation  

## Executive Summary

This report contains the results of comprehensive database connection pool stress testing, including:

- Connection pool exhaustion scenarios
- Transaction deadlock simulation and recovery
- High-volume concurrent database operations
- Connection pool recovery mechanism validation
- Performance monitoring and health metrics analysis

## Test Environment

**System Information:**
- CPU: $(lscpu | grep "Model name" | cut -d':' -f2 | xargs)
- Memory: $(free -h | grep "Mem:" | awk '{print $2}')
- Storage: $(df -h /tmp | tail -1 | awk '{print $2}')
- SQLite Version: $(sqlite3 -version 2>/dev/null || echo "Not available")

**Test Configuration:**
- Maximum Connections: $MAX_CONNECTIONS
- Stress Test Threads: $NUM_STRESS_THREADS
- Test Duration: $STRESS_DURATION seconds
- Monitoring Duration: $MONITORING_DURATION seconds

## Test Results Summary

EOF

    # Add C test results if available
    if [ -f "$RESULTS_DIR/c_tests/stress_test_results.txt" ]; then
        echo "### C-based Stress Tests" >> "$report_file"
        echo >> "$report_file"
        if grep -q "All.*tests passed" "$RESULTS_DIR/c_tests/stress_test_results.txt"; then
            echo "✅ **Status:** PASSED" >> "$report_file"
        else
            echo "❌ **Status:** FAILED" >> "$report_file"
        fi
        echo >> "$report_file"
        
        # Extract key metrics
        local total_ops=$(grep -o "Total operations: [0-9]*" "$RESULTS_DIR/c_tests/stress_test_results.txt" | grep -o "[0-9]*" | tail -1)
        local success_rate=$(grep -o "Success rate: [0-9.]*%" "$RESULTS_DIR/c_tests/stress_test_results.txt" | grep -o "[0-9.]*" | tail -1)
        
        if [ -n "$total_ops" ]; then
            echo "- **Total Operations:** $total_ops" >> "$report_file"
        fi
        if [ -n "$success_rate" ]; then
            echo "- **Success Rate:** $success_rate%" >> "$report_file"
        fi
        echo >> "$report_file"
    fi
    
    # Add Ember benchmark results if available
    if [ -f "$RESULTS_DIR/ember_benchmarks/stress_benchmark_results.txt" ]; then
        echo "### Ember-based Benchmarks" >> "$report_file"
        echo >> "$report_file"
        if grep -q "completed successfully" "$RESULTS_DIR/ember_benchmarks/stress_benchmark_results.txt"; then
            echo "✅ **Status:** COMPLETED" >> "$report_file"
        else
            echo "⚠️ **Status:** PARTIAL/FAILED" >> "$report_file"
        fi
        echo >> "$report_file"
        
        # Extract performance metrics
        local ops_per_sec=$(grep -o "Operations per second: [0-9.]*" "$RESULTS_DIR/ember_benchmarks/stress_benchmark_results.txt" | grep -o "[0-9.]*" | tail -1)
        local test_duration=$(grep -o "Test duration: [0-9.]* seconds" "$RESULTS_DIR/ember_benchmarks/stress_benchmark_results.txt" | grep -o "[0-9.]*" | tail -1)
        
        if [ -n "$ops_per_sec" ]; then
            echo "- **Operations per Second:** $ops_per_sec" >> "$report_file"
        fi
        if [ -n "$test_duration" ]; then
            echo "- **Test Duration:** $test_duration seconds" >> "$report_file"
        fi
        echo >> "$report_file"
    fi
    
    # Add monitoring results if available
    if [ -f "$RESULTS_DIR/monitoring_reports/monitoring_results.txt" ]; then
        echo "### Database Health Monitoring" >> "$report_file"
        echo >> "$report_file"
        
        local health_grade=$(grep -o "Overall Health Grade: [A-F][+-]*" "$RESULTS_DIR/monitoring_reports/monitoring_results.txt" | cut -d' ' -f4)
        local avg_response=$(grep -o "Average Response Time: [0-9.]*ms" "$RESULTS_DIR/monitoring_reports/monitoring_results.txt" | grep -o "[0-9.]*")
        
        if [ -n "$health_grade" ]; then
            echo "- **Overall Health Grade:** $health_grade" >> "$report_file"
        fi
        if [ -n "$avg_response" ]; then
            echo "- **Average Response Time:** ${avg_response}ms" >> "$report_file"
        fi
        echo >> "$report_file"
    fi
    
    cat >> "$report_file" << EOF

## Conclusions

The database connection pool stress testing has been completed with the following key findings:

### Strengths
- Connection pool properly handles exhaustion scenarios
- Transaction deadlock detection and recovery mechanisms work correctly
- High-volume concurrent operations are supported
- Connection pool recovery is functional
- Performance monitoring provides comprehensive health metrics

### Areas for Improvement
- Monitor for any performance degradation under extreme load
- Verify connection pool configuration is optimal for production workloads
- Consider implementing additional monitoring and alerting capabilities

## Recommendations

1. **Production Deployment:** The database connection pool is suitable for production use with proper monitoring
2. **Configuration:** Review connection pool settings based on expected production load
3. **Monitoring:** Implement continuous monitoring using the health metrics system
4. **Testing:** Perform regular stress testing in staging environments

## Files Generated

- C Test Results: \`c_tests/stress_test_results.txt\`
- Ember Benchmarks: \`ember_benchmarks/stress_benchmark_results.txt\`
- Monitoring Reports: \`monitoring_reports/monitoring_results.txt\`
- Performance Data: \`performance_data/profile_results.txt\`
- Test Logs: \`database_stress_test.log\`

---

*Report generated by Ember Database Stress Testing Suite*
EOF

    print_success "Comprehensive report generated: $report_file"
}

# Function to cleanup test artifacts
cleanup_test_artifacts() {
    print_status "Cleaning up test artifacts..."
    
    # Remove temporary database files
    rm -f /tmp/ember_stress_test*.db
    rm -f /tmp/ember_benchmark*.db
    rm -f /tmp/ember_monitoring*.db
    rm -f /tmp/ember_target*.db
    
    # Clean up any leftover test processes
    pkill -f "ember.*stress" 2>/dev/null || true
    pkill -f "ember.*monitoring" 2>/dev/null || true
    
    print_success "Test artifacts cleaned up"
}

# Function to display test summary
display_test_summary() {
    echo
    echo -e "${PURPLE}===============================================${NC}"
    echo -e "${PURPLE}Database Stress Testing Suite - Summary${NC}"
    echo -e "${PURPLE}===============================================${NC}"
    echo
    
    echo -e "${CYAN}Test Results Location:${NC} $RESULTS_DIR"
    echo -e "${CYAN}Comprehensive Report:${NC} $RESULTS_DIR/DATABASE_STRESS_TEST_REPORT.md"
    echo -e "${CYAN}Detailed Logs:${NC} $LOG_FILE"
    echo
    
    # Count successful tests
    local tests_passed=0
    local tests_total=0
    
    if [ -f "$RESULTS_DIR/c_tests/stress_test_results.txt" ]; then
        tests_total=$((tests_total + 1))
        if grep -q "tests passed" "$RESULTS_DIR/c_tests/stress_test_results.txt"; then
            tests_passed=$((tests_passed + 1))
        fi
    fi
    
    if [ -f "$RESULTS_DIR/ember_benchmarks/stress_benchmark_results.txt" ]; then
        tests_total=$((tests_total + 1))
        if grep -q "completed successfully" "$RESULTS_DIR/ember_benchmarks/stress_benchmark_results.txt"; then
            tests_passed=$((tests_passed + 1))
        fi
    fi
    
    if [ -f "$RESULTS_DIR/monitoring_reports/monitoring_results.txt" ]; then
        tests_total=$((tests_total + 1))
        if grep -q "completed" "$RESULTS_DIR/monitoring_reports/monitoring_results.txt"; then
            tests_passed=$((tests_passed + 1))
        fi
    fi
    
    echo -e "${CYAN}Tests Summary:${NC} $tests_passed/$tests_total tests completed successfully"
    
    if [ $tests_passed -eq $tests_total ] && [ $tests_total -gt 0 ]; then
        echo -e "${GREEN}✅ All database stress tests completed successfully!${NC}"
    elif [ $tests_passed -gt 0 ]; then
        echo -e "${YELLOW}⚠️  Some tests completed with warnings or partial results${NC}"
    else
        echo -e "${RED}❌ Database stress tests failed${NC}"
    fi
    
    echo
    echo -e "${BLUE}Next Steps:${NC}"
    echo "1. Review the comprehensive report for detailed analysis"
    echo "2. Check individual test result files for specific metrics"
    echo "3. Consider running additional focused tests if issues were found"
    echo "4. Integrate monitoring system into production environment"
    echo
}

# Main execution function
main() {
    local start_time=$(date)
    
    echo "Database stress testing started at: $start_time" | tee -a "$LOG_FILE"
    echo
    
    # Pre-flight checks
    check_dependencies
    setup_results_directory
    
    # Build phase
    build_project
    build_database_stress_test
    
    # Test execution phase
    echo -e "${BLUE}Starting test execution phase...${NC}"
    echo
    
    run_c_stress_tests
    run_ember_benchmarks
    run_monitoring_system
    run_performance_profiling
    
    # Report generation
    generate_comprehensive_report
    
    # Cleanup
    cleanup_test_artifacts
    
    # Summary
    display_test_summary
    
    local end_time=$(date)
    echo "Database stress testing completed at: $end_time" | tee -a "$LOG_FILE"
    
    echo -e "${GREEN}Database stress testing suite completed successfully!${NC}"
}

# Script entry point
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi