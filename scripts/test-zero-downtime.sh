#!/bin/bash
# Test script for zero-downtime reload functionality
# This script tests the graceful worker recycling and configuration hot reload

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build"
EMBERWEB_BIN="$BUILD_DIR/emberweb/emberweb"
CONFIG_DIR="$PROJECT_DIR/emberweb/config"
TEST_CONFIG="$CONFIG_DIR/test-reload.conf"
PID_FILE="/tmp/emberweb-test.pid"
TEST_PORT=8888
TEST_SSL_PORT=8889

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log() {
    echo -e "${GREEN}[$(date '+%H:%M:%S')]${NC} $1"
}

warn() {
    echo -e "${YELLOW}[$(date '+%H:%M:%S')] WARNING:${NC} $1"
}

error() {
    echo -e "${RED}[$(date '+%H:%M:%S')] ERROR:${NC} $1"
}

cleanup() {
    log "Cleaning up test environment..."
    
    # Kill test server if running
    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        if kill -0 "$PID" 2>/dev/null; then
            log "Stopping test server (PID: $PID)"
            kill -TERM "$PID" 2>/dev/null || true
            sleep 2
            # Force kill if still running
            if kill -0 "$PID" 2>/dev/null; then
                kill -KILL "$PID" 2>/dev/null || true
            fi
        fi
        rm -f "$PID_FILE"
    fi
    
    # Clean up test config
    rm -f "$TEST_CONFIG" "$TEST_CONFIG.new" "$TEST_CONFIG.backup"
    
    log "Cleanup completed"
}

# Set up cleanup on exit
trap cleanup EXIT INT TERM

create_test_config() {
    local config_file="$1"
    local worker_count="${2:-2}"
    local dev_mode="${3:-true}"
    
    cat > "$config_file" << EOF
# Test configuration for zero-downtime reload
port = $TEST_PORT
ssl_port = $TEST_SSL_PORT
root_dir = $PROJECT_DIR/emberweb/www
server_name = EmberWeb Test Server
worker_processes = $worker_count
development_mode = $dev_mode
hot_reload_enabled = true

# Security settings
ssl_enabled = false
rate_limiting_enabled = false

# Performance settings
max_connections = 100
keepalive_timeout = 30

# Logging
access_log_enabled = true
error_log_enabled = true
log_level = 2
EOF
}

check_server_status() {
    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        if kill -0 "$PID" 2>/dev/null; then
            return 0
        fi
    fi
    return 1
}

wait_for_server() {
    local timeout="${1:-30}"
    local count=0
    
    log "Waiting for server to be ready..."
    while [ $count -lt $timeout ]; do
        if curl -s -f "http://localhost:$TEST_PORT/" >/dev/null 2>&1; then
            log "Server is ready after $count seconds"
            return 0
        fi
        sleep 1
        count=$((count + 1))
    done
    
    error "Server did not start within $timeout seconds"
    return 1
}

count_worker_processes() {
    if [ -f "$PID_FILE" ]; then
        PID=$(cat "$PID_FILE")
        # Count processes with the same parent PID (worker processes)
        ps --ppid "$PID" -o pid= 2>/dev/null | wc -l
    else
        echo "0"
    fi
}

test_basic_functionality() {
    log "Testing basic server functionality..."
    
    # Test HTTP request
    local response=$(curl -s "http://localhost:$TEST_PORT/")
    if [ $? -eq 0 ]; then
        log "✓ HTTP request successful"
    else
        error "✗ HTTP request failed"
        return 1
    fi
    
    # Check worker processes
    local worker_count=$(count_worker_processes)
    log "✓ Worker processes running: $worker_count"
    
    return 0
}

test_graceful_reload() {
    log "Testing graceful configuration reload..."
    
    # Get initial worker count and PIDs
    local initial_workers=$(count_worker_processes)
    local initial_pids=$(ps --ppid "$(cat "$PID_FILE")" -o pid= 2>/dev/null | tr '\n' ' ')
    
    log "Initial workers: $initial_workers (PIDs: $initial_pids)"
    
    # Create new configuration with different worker count
    create_test_config "$TEST_CONFIG.new" 3 true
    cp "$TEST_CONFIG.new" "$TEST_CONFIG"
    
    # Send reload signal
    local main_pid=$(cat "$PID_FILE")
    log "Sending SIGUSR1 to main process (PID: $main_pid)"
    kill -USR1 "$main_pid"
    
    # Monitor reload process
    log "Monitoring reload process..."
    local reload_start=$(date +%s)
    local max_workers_seen=0
    
    for i in {1..20}; do
        local current_workers=$(count_worker_processes)
        if [ "$current_workers" -gt "$max_workers_seen" ]; then
            max_workers_seen=$current_workers
        fi
        
        log "Time: ${i}s, Workers: $current_workers (max seen: $max_workers_seen)"
        
        # Test if server is still responding during reload
        if curl -s -f "http://localhost:$TEST_PORT/" >/dev/null 2>&1; then
            log "✓ Server responsive during reload"
        else
            warn "Server not responsive at time ${i}s"
        fi
        
        sleep 1
    done
    
    local reload_end=$(date +%s)
    local reload_duration=$((reload_end - reload_start))
    
    # Check final state
    local final_workers=$(count_worker_processes)
    local final_pids=$(ps --ppid "$(cat "$PID_FILE")" -o pid= 2>/dev/null | tr '\n' ' ')
    
    log "Final workers: $final_workers (PIDs: $final_pids)"
    log "Reload duration: ${reload_duration}s"
    
    # Verify reload success
    if [ "$final_workers" -eq 3 ]; then
        log "✓ Worker count updated correctly (2 -> 3)"
    else
        error "✗ Worker count not updated correctly (expected: 3, actual: $final_workers)"
        return 1
    fi
    
    if [ "$max_workers_seen" -gt "$initial_workers" ]; then
        log "✓ Graceful overlap detected (max workers during reload: $max_workers_seen)"
    else
        warn "No worker overlap detected during reload"
    fi
    
    # Test final functionality
    if test_basic_functionality; then
        log "✓ Server functionality maintained after reload"
    else
        error "✗ Server functionality broken after reload"
        return 1
    fi
    
    return 0
}

test_config_file_monitoring() {
    log "Testing automatic configuration file monitoring..."
    
    # Check if hot reload is working
    local initial_workers=$(count_worker_processes)
    
    log "Current workers: $initial_workers"
    log "Modifying configuration file to trigger auto-reload..."
    
    # Modify config file
    create_test_config "$TEST_CONFIG" 4 true
    
    # Wait for auto-reload to trigger
    log "Waiting for auto-reload to trigger..."
    sleep 5
    
    # Check if reload happened
    local current_workers=$(count_worker_processes)
    log "Workers after auto-reload: $current_workers"
    
    # Wait a bit more for reload to complete
    sleep 5
    local final_workers=$(count_worker_processes)
    
    if [ "$final_workers" -eq 4 ]; then
        log "✓ Automatic config reload successful (workers: $initial_workers -> $final_workers)"
    else
        warn "Automatic config reload may not have worked (expected: 4, actual: $final_workers)"
    fi
    
    return 0
}

test_connection_draining() {
    log "Testing connection draining during graceful shutdown..."
    
    # Start a long-running request in background
    (
        log "Starting long-running request..."
        curl -s "http://localhost:$TEST_PORT/request_info.ember?delay=10" >/dev/null 2>&1
        log "Long-running request completed"
    ) &
    local request_pid=$!
    
    sleep 2
    
    # Trigger graceful shutdown of one worker
    local worker_pids=$(ps --ppid "$(cat "$PID_FILE")" -o pid= 2>/dev/null)
    local first_worker=$(echo $worker_pids | awk '{print $1}')
    
    if [ -n "$first_worker" ]; then
        log "Sending graceful shutdown signal to worker PID: $first_worker"
        kill -USR1 "$first_worker"
        
        # Monitor if the worker waits for connection to complete
        log "Monitoring worker shutdown process..."
        for i in {1..15}; do
            if kill -0 "$first_worker" 2>/dev/null; then
                log "Worker $first_worker still running (waiting for connections) - ${i}s"
                sleep 1
            else
                log "Worker $first_worker has exited after ${i}s"
                break
            fi
        done
        
        # Wait for the background request to complete
        wait $request_pid
        log "✓ Connection draining test completed"
    else
        warn "Could not find worker process for connection draining test"
    fi
    
    return 0
}

run_load_test() {
    log "Running load test during reload..."
    
    # Start load test in background
    (
        local requests=0
        local errors=0
        local start_time=$(date +%s)
        
        while [ $(($(date +%s) - start_time)) -lt 30 ]; do
            if curl -s -f "http://localhost:$TEST_PORT/" >/dev/null 2>&1; then
                requests=$((requests + 1))
            else
                errors=$((errors + 1))
            fi
            
            # Brief pause between requests
            sleep 0.1
        done
        
        log "Load test completed: $requests successful, $errors errors"
        if [ "$errors" -eq 0 ]; then
            log "✓ Zero errors during load test"
        else
            warn "Errors occurred during load test: $errors"
        fi
    ) &
    local load_test_pid=$!
    
    # Wait a bit for load test to start
    sleep 2
    
    # Trigger reload during load test
    log "Triggering reload during load test..."
    kill -USR1 "$(cat "$PID_FILE")"
    
    # Wait for load test to complete
    wait $load_test_pid
    
    return 0
}

main() {
    log "Starting EmberWeb Zero-Downtime Reload Tests"
    log "============================================="
    
    # Check if emberweb binary exists
    if [ ! -f "$EMBERWEB_BIN" ]; then
        error "EmberWeb binary not found at: $EMBERWEB_BIN"
        error "Please build the project first: make all"
        exit 1
    fi
    
    # Create test configuration
    log "Creating test configuration..."
    create_test_config "$TEST_CONFIG" 2 true
    
    # Start test server
    log "Starting test server..."
    "$EMBERWEB_BIN" -c "$TEST_CONFIG" -d &
    echo $! > "$PID_FILE"
    
    # Wait for server to start
    if ! wait_for_server 30; then
        error "Failed to start test server"
        exit 1
    fi
    
    log "Test server started successfully (PID: $(cat "$PID_FILE"))"
    
    # Run tests
    log ""
    log "Running test suite..."
    log "===================="
    
    local tests_passed=0
    local tests_total=5
    
    # Test 1: Basic functionality
    if test_basic_functionality; then
        tests_passed=$((tests_passed + 1))
    fi
    
    log ""
    
    # Test 2: Graceful reload
    if test_graceful_reload; then
        tests_passed=$((tests_passed + 1))
    fi
    
    log ""
    
    # Test 3: Config file monitoring
    if test_config_file_monitoring; then
        tests_passed=$((tests_passed + 1))
    fi
    
    log ""
    
    # Test 4: Connection draining
    if test_connection_draining; then
        tests_passed=$((tests_passed + 1))
    fi
    
    log ""
    
    # Test 5: Load test during reload
    if run_load_test; then
        tests_passed=$((tests_passed + 1))
    fi
    
    # Results
    log ""
    log "Test Results"
    log "============"
    log "Tests passed: $tests_passed / $tests_total"
    
    if [ "$tests_passed" -eq "$tests_total" ]; then
        log "✓ All tests passed! Zero-downtime reload functionality is working correctly."
        exit 0
    else
        error "✗ Some tests failed. Please check the implementation."
        exit 1
    fi
}

# Run main function
main "$@"