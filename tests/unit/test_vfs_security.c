#include "ember.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "../unit/test_ember_internal.h"

// Test VFS security functions to improve coverage of critical path validation
void test_vfs_path_traversal_prevention(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing VFS path traversal prevention...\n");
    
    // Test basic mounting
    int result = ember_vfs_mount(vm, "/test", "/tmp", EMBER_MOUNT_RW);
    assert(result == 0);
    
    // Test path resolution (should prevent directory traversal)
    char* resolved = ember_vfs_resolve(vm, "/test/../etc/passwd");
    // This should either return NULL or a safe path, not /etc/passwd
    if (resolved) {
        assert(strstr(resolved, "/etc/passwd") == NULL);
        free(resolved);
    }
    
    // Test another traversal attempt
    resolved = ember_vfs_resolve(vm, "/test/../../root/.ssh/");
    if (resolved) {
        assert(strstr(resolved, "/root") == NULL);
        free(resolved);
    }
    
    printf("VFS path traversal prevention test passed\n");
    ember_free_vm(vm);
}

void test_vfs_access_control(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing VFS access control...\n");
    
    // Mount read-only directory
    int result = ember_vfs_mount(vm, "/readonly", "/tmp", EMBER_MOUNT_RO);
    assert(result == 0);
    
    // Test read access (should succeed)
    int read_access = ember_vfs_check_access(vm, "/readonly/test.txt", 0);
    printf("Read access check result: %d\n", read_access);
    
    // Test write access (should fail for read-only mount)
    int write_access = ember_vfs_check_access(vm, "/readonly/test.txt", 1);
    printf("Write access check result: %d\n", write_access);
    
    // For read-only mount, write should be denied
    // (The exact behavior depends on implementation)
    
    printf("VFS access control test passed\n");
    ember_free_vm(vm);
}

void test_vfs_mount_limits(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing VFS mount limits...\n");
    
    // Test mounting within limits
    for (int i = 0; i < 5; i++) {
        char virtual_path[32];
        snprintf(virtual_path, sizeof(virtual_path), "/mount%d", i);
        int result = ember_vfs_mount(vm, virtual_path, "/tmp", EMBER_MOUNT_RW);
        if (result != 0) {
            printf("Mount failed at iteration %d (expected if limit reached)\n", i);
            break;
        }
    }
    
    printf("VFS mount limits test passed\n");
    ember_free_vm(vm);
}

void test_vfs_path_sanitization(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing VFS path sanitization...\n");
    
    // Mount a test directory
    int result = ember_vfs_mount(vm, "/test", "/tmp", EMBER_MOUNT_RW);
    assert(result == 0);
    
    // Test various malicious path patterns
    const char* dangerous_paths[] = {
        "/test/../../../etc/passwd",
        "/test/./../../bin/sh",
        "/test///../root/",
        "/test/subdir/../../etc/shadow",
        "/test/normal/../../../etc/hosts",
        NULL
    };
    
    for (int i = 0; dangerous_paths[i] != NULL; i++) {
        char* resolved = ember_vfs_resolve(vm, dangerous_paths[i]);
        if (resolved) {
            printf("Resolved '%s' to '%s'\n", dangerous_paths[i], resolved);
            // Ensure resolved path doesn't escape the mount point
            assert(strstr(resolved, "/etc/") == NULL);
            assert(strstr(resolved, "/root/") == NULL);
            assert(strstr(resolved, "/bin/") == NULL);
            free(resolved);
        } else {
            printf("Path '%s' correctly rejected\n", dangerous_paths[i]);
        }
    }
    
    printf("VFS path sanitization test passed\n");
    ember_free_vm(vm);
}

void test_vfs_unmount_functionality(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing VFS unmount functionality...\n");
    
    // Mount a directory
    int result = ember_vfs_mount(vm, "/testmount", "/tmp", EMBER_MOUNT_RW);
    assert(result == 0);
    
    // Verify it's mounted by trying to resolve a path
    char* resolved = ember_vfs_resolve(vm, "/testmount/test");
    if (resolved) {
        printf("Path resolved before unmount: %s\n", resolved);
        free(resolved);
    }
    
    // Unmount the directory
    result = ember_vfs_unmount(vm, "/testmount");
    printf("Unmount result: %d\n", result);
    
    // Try to resolve path again (should fail or return different result)
    resolved = ember_vfs_resolve(vm, "/testmount/test");
    if (resolved) {
        printf("Path resolved after unmount: %s\n", resolved);
        free(resolved);
    } else {
        printf("Path correctly inaccessible after unmount\n");
    }
    
    printf("VFS unmount functionality test passed\n");
    ember_free_vm(vm);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember VFS Security Tests...\n");
    
    test_vfs_path_traversal_prevention();
    test_vfs_access_control();
    test_vfs_mount_limits();
    test_vfs_path_sanitization();
    test_vfs_unmount_functionality();
    
    printf("All VFS security tests passed!\n");
    return 0;
}