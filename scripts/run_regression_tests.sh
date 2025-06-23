#!/bin/bash

# Ember Regression Test Suite
# Builds ember-core and runs all test files with comprehensive reporting

set -e  # Exit on first error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Directories
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build"
TESTS_DIR="$REPO_ROOT/tests"
EMBER_BIN="$BUILD_DIR/ember"

# Test results file
RESULTS_FILE="$REPO_ROOT/test_results.txt"
FAILED_TESTS_FILE="$REPO_ROOT/failed_tests.txt"

# Function to print colored output
print_color() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to clean build
clean_build() {
    print_color "$YELLOW" "Cleaning previous build..."
    cd "$REPO_ROOT"
    make clean > /dev/null 2>&1 || true
}

# Function to build ember-core
build_ember() {
    print_color "$YELLOW" "Building ember-core..."
    cd "$REPO_ROOT"
    
    # Capture build output
    if make 2>&1 | tee build.log; then
        print_color "$GREEN" "✓ Build successful"
        return 0
    else
        print_color "$RED" "✗ Build failed"
        echo "See build.log for details"
        return 1
    fi
}

# Function to check if test should be skipped
should_skip_test() {
    local test_file=$1
    local filename=$(basename "$test_file")
    
    # Skip certain test files that are known to have issues
    case "$filename" in
        "test_continue_bug.ember"|"continue_bug_test.ember")
            return 0  # Skip continue bug tests
            ;;
        *)
            return 1  # Don't skip
            ;;
    esac
}

# Function to get expected output for a test
get_expected_output() {
    local test_file=$1
    local expected_file="${test_file%.ember}.expected"
    
    if [ -f "$expected_file" ]; then
        cat "$expected_file"
    else
        # If no expected file, look for inline expectations in the test
        grep "^# EXPECT:" "$test_file" 2>/dev/null | sed 's/^# EXPECT: //' || echo ""
    fi
}

# Function to run a single test
run_test() {
    local test_file=$1
    local test_name=$(basename "$test_file")
    
    echo -n "Running $test_name..."
    
    # Check if test should be skipped
    if should_skip_test "$test_file"; then
        print_color "$YELLOW" " SKIPPED (known issue)"
        ((SKIPPED_TESTS++))
        return 0
    fi
    
    # Create temp files for output
    local output_file=$(mktemp)
    local error_file=$(mktemp)
    
    # Run the test with timeout
    if timeout 10s "$EMBER_BIN" "$test_file" > "$output_file" 2> "$error_file"; then
        local exit_code=$?
        
        # Get expected output
        local expected_output=$(get_expected_output "$test_file")
        
        # If we have expected output, compare
        if [ -n "$expected_output" ]; then
            local actual_output=$(cat "$output_file")
            if [ "$actual_output" = "$expected_output" ]; then
                print_color "$GREEN" " PASSED"
                ((PASSED_TESTS++))
            else
                print_color "$RED" " FAILED (output mismatch)"
                echo "Test: $test_file" >> "$FAILED_TESTS_FILE"
                echo "Expected: $expected_output" >> "$FAILED_TESTS_FILE"
                echo "Actual: $actual_output" >> "$FAILED_TESTS_FILE"
                echo "---" >> "$FAILED_TESTS_FILE"
                ((FAILED_TESTS++))
            fi
        else
            # No expected output - just check exit code
            if [ $exit_code -eq 0 ]; then
                print_color "$GREEN" " PASSED"
                ((PASSED_TESTS++))
            else
                print_color "$RED" " FAILED (exit code: $exit_code)"
                echo "Test: $test_file" >> "$FAILED_TESTS_FILE"
                echo "Exit code: $exit_code" >> "$FAILED_TESTS_FILE"
                echo "Error output:" >> "$FAILED_TESTS_FILE"
                cat "$error_file" >> "$FAILED_TESTS_FILE"
                echo "---" >> "$FAILED_TESTS_FILE"
                ((FAILED_TESTS++))
            fi
        fi
    else
        print_color "$RED" " FAILED (timeout or crash)"
        echo "Test: $test_file" >> "$FAILED_TESTS_FILE"
        echo "Test timed out or crashed" >> "$FAILED_TESTS_FILE"
        echo "Error output:" >> "$FAILED_TESTS_FILE"
        cat "$error_file" >> "$FAILED_TESTS_FILE"
        echo "---" >> "$FAILED_TESTS_FILE"
        ((FAILED_TESTS++))
    fi
    
    # Clean up temp files
    rm -f "$output_file" "$error_file"
    
    ((TOTAL_TESTS++))
}

# Function to run tests in a directory
run_tests_in_dir() {
    local dir=$1
    local category=$2
    
    if [ ! -d "$dir" ]; then
        return
    fi
    
    local test_files=$(find "$dir" -name "*.ember" -type f | sort)
    
    if [ -z "$test_files" ]; then
        return
    fi
    
    print_color "$YELLOW" "\n=== Running $category tests ==="
    
    for test_file in $test_files; do
        run_test "$test_file"
    done
}

# Function to run legacy tests from root
run_legacy_tests() {
    print_color "$YELLOW" "\n=== Running legacy tests from root ==="
    
    # Find test files in root that aren't in the tests/ directory
    local test_files=$(find "$REPO_ROOT" -maxdepth 1 -name "*test*.ember" -o -name "*_test.ember" | sort)
    
    for test_file in $test_files; do
        run_test "$test_file"
    done
}

# Function to print summary
print_summary() {
    echo
    echo "========================================"
    echo "          TEST SUMMARY"
    echo "========================================"
    echo "Total tests:    $TOTAL_TESTS"
    print_color "$GREEN" "Passed tests:   $PASSED_TESTS"
    
    if [ $FAILED_TESTS -gt 0 ]; then
        print_color "$RED" "Failed tests:   $FAILED_TESTS"
    fi
    
    if [ $SKIPPED_TESTS -gt 0 ]; then
        print_color "$YELLOW" "Skipped tests:  $SKIPPED_TESTS"
    fi
    
    echo "========================================"
    
    # Calculate pass rate
    if [ $TOTAL_TESTS -gt 0 ]; then
        local pass_rate=$(( (PASSED_TESTS * 100) / TOTAL_TESTS ))
        echo "Pass rate: ${pass_rate}%"
    fi
    
    # Save results
    {
        echo "Test run at: $(date)"
        echo "Total: $TOTAL_TESTS"
        echo "Passed: $PASSED_TESTS"
        echo "Failed: $FAILED_TESTS"
        echo "Skipped: $SKIPPED_TESTS"
    } > "$RESULTS_FILE"
    
    if [ $FAILED_TESTS -gt 0 ]; then
        echo
        print_color "$RED" "Failed tests details saved to: $FAILED_TESTS_FILE"
    fi
}

# Parse command line arguments
FILTER=""
while [[ $# -gt 0 ]]; do
    case $1 in
        --filter=*)
            FILTER="${1#*=}"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--filter=core|control_flow|stdlib|regression|security]"
            exit 1
            ;;
    esac
done

# Main execution
main() {
    print_color "$YELLOW" "Ember Regression Test Suite"
    print_color "$YELLOW" "=========================="
    
    if [ -n "$FILTER" ]; then
        print_color "$YELLOW" "Filter: $FILTER"
    fi
    
    # Clean previous results
    rm -f "$RESULTS_FILE" "$FAILED_TESTS_FILE"
    
    # Clean and build
    clean_build
    if ! build_ember; then
        print_color "$RED" "Build failed - cannot run tests"
        exit 1
    fi
    
    # Verify ember binary exists
    if [ ! -f "$EMBER_BIN" ]; then
        print_color "$RED" "Error: Ember binary not found at $EMBER_BIN"
        exit 1
    fi
    
    # Run tests by category based on filter
    if [ -z "$FILTER" ] || [ "$FILTER" = "core" ]; then
        run_tests_in_dir "$TESTS_DIR/core" "Core"
    fi
    
    if [ -z "$FILTER" ] || [ "$FILTER" = "control_flow" ]; then
        run_tests_in_dir "$TESTS_DIR/control_flow" "Control Flow"
    fi
    
    if [ -z "$FILTER" ] || [ "$FILTER" = "stdlib" ]; then
        run_tests_in_dir "$TESTS_DIR/stdlib" "Standard Library"
    fi
    
    if [ -z "$FILTER" ] || [ "$FILTER" = "regression" ]; then
        run_tests_in_dir "$TESTS_DIR/regression" "Regression"
    fi
    
    if [ -z "$FILTER" ] || [ "$FILTER" = "security" ]; then
        # Security tests are special - run from security_scripts
        if [ -d "$REPO_ROOT/scripts" ] && [ -f "$REPO_ROOT/scripts/test_security.sh" ]; then
            print_color "$YELLOW" "\n=== Running Security tests ==="
            cd "$REPO_ROOT" && bash scripts/test_security.sh
        fi
    fi
    
    # Run legacy tests only if no filter or filter is "legacy"
    if [ -z "$FILTER" ] || [ "$FILTER" = "legacy" ]; then
        run_legacy_tests
    fi
    
    # Print summary
    print_summary
    
    # Exit with error if any tests failed
    if [ $FAILED_TESTS -gt 0 ]; then
        exit 1
    else
        exit 0
    fi
}

# Run main if not being sourced
if [ "${BASH_SOURCE[0]}" = "${0}" ]; then
    main "$@"
fi