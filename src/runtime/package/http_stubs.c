/**
 * HTTP Function Stubs for Ember Core
 * Provides stub implementations to resolve linking issues in container builds
 * Real implementations are in ember-stdlib
 */

#include <stdio.h>
#include <stdlib.h>
#include "../../vm.h"

// HTTP response structure stub
typedef struct {
    char* data;
    size_t size;
    long response_code;
    char* content_type;
    double download_time;
} http_response_t;

/**
 * Stub HTTP initialization
 */
int ember_http_init(void) {
    printf("[HTTP] WARNING: Using stub HTTP implementation - functionality disabled\n");
    return 0;
}

/**
 * Stub HTTP cleanup
 */
void ember_http_cleanup(void) {
    // No-op for stub
}

/**
 * Stub HTTP GET request
 */
int ember_http_get(const char* url, const char* auth_token, http_response_t* response) {
    (void)url;
    (void)auth_token;
    (void)response;
    
    printf("[HTTP] WARNING: HTTP GET not available - using stub implementation\n");
    printf("[HTTP] Package operations require building with ember-stdlib\n");
    return -1;
}

/**
 * Stub HTTP file download
 */
int ember_http_download_file(const char* url, const char* local_path, const char* auth_token) {
    (void)url;
    (void)local_path;
    (void)auth_token;
    
    printf("[HTTP] WARNING: HTTP download not available - using stub implementation\n");
    printf("[HTTP] Package operations require building with ember-stdlib\n");
    return -1;
}

/**
 * Stub HTTP file upload
 */
int ember_http_upload_file(const char* url, const char* file_path, const char* auth_token) {
    (void)url;
    (void)file_path;
    (void)auth_token;
    
    printf("[HTTP] WARNING: HTTP upload not available - using stub implementation\n");
    printf("[HTTP] Package operations require building with ember-stdlib\n");
    return -1;
}

/**
 * Stub HTTP response cleanup
 */
void http_response_cleanup(http_response_t* response) {
    if (response) {
        if (response->data) {
            free(response->data);
            response->data = NULL;
        }
        if (response->content_type) {
            free(response->content_type);
            response->content_type = NULL;
        }
        response->size = 0;
    }
}

// ============================================================================
// REQUEST/RESPONSE STUB FUNCTIONS FOR WEB INTEGRATION
// ============================================================================

/**
 * Stub request get query function
 */
ember_value ember_native_request_get_query(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: Request query not available - using stub implementation\n");
    printf("[HTTP] Web request functions require building with ember-stdlib\n");
    return ember_make_string("");
}

/**
 * Stub request get header function
 */
ember_value ember_native_request_get_header(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: Request header not available - using stub implementation\n");
    printf("[HTTP] Web request functions require building with ember-stdlib\n");
    return ember_make_string("");
}

/**
 * Stub request get body function
 */
ember_value ember_native_request_get_body(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: Request body not available - using stub implementation\n");
    printf("[HTTP] Web request functions require building with ember-stdlib\n");
    return ember_make_string("");
}

/**
 * Stub response set status function
 */
ember_value ember_native_response_set_status(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: Response status not available - using stub implementation\n");
    printf("[HTTP] Web response functions require building with ember-stdlib\n");
    return ember_make_bool(0);
}

/**
 * Stub response set header function
 */
ember_value ember_native_response_set_header(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: Response header not available - using stub implementation\n");
    printf("[HTTP] Web response functions require building with ember-stdlib\n");
    return ember_make_bool(0);
}

/**
 * Stub response write function
 */
ember_value ember_native_response_write(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: Response write not available - using stub implementation\n");
    printf("[HTTP] Web response functions require building with ember-stdlib\n");
    return ember_make_bool(0);
}

/**
 * Stub response end function
 */
ember_value ember_native_response_end(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: Response end not available - using stub implementation\n");
    printf("[HTTP] Web response functions require building with ember-stdlib\n");
    return ember_make_bool(0);
}

/**
 * Stub session registration function
 */
void ember_register_session_native(ember_vm* vm) {
    (void)vm;
    
    printf("[HTTP] WARNING: Session management not available - using stub implementation\n");
    printf("[HTTP] Session functions require building with ember-stdlib\n");
}

/**
 * Stub HTTP server listen and serve function
 */
ember_value ember_native_http_listen_and_serve(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: HTTP server not available - using stub implementation\n");
    printf("[HTTP] Server functions require building with ember-stdlib\n");
    return ember_make_bool(0);
}

/**
 * Stub HTTP server stop function
 */
ember_value ember_native_http_stop_server(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: HTTP server stop not available - using stub implementation\n");
    printf("[HTTP] Server functions require building with ember-stdlib\n");
    return ember_make_bool(0);
}

/**
 * Stub request get method function
 */
ember_value ember_native_request_get_method(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: Request method not available - using stub implementation\n");
    printf("[HTTP] Web request functions require building with ember-stdlib\n");
    return ember_make_string("GET");
}

/**
 * Stub request get path function
 */
ember_value ember_native_request_get_path(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[HTTP] WARNING: Request path not available - using stub implementation\n");
    printf("[HTTP] Web request functions require building with ember-stdlib\n");
    return ember_make_string("/");
}