/**
 * HTTP Function Stubs for Ember Core
 * Provides stub declarations to resolve linking issues in container builds
 */

#ifndef EMBER_HTTP_STUBS_H
#define EMBER_HTTP_STUBS_H

#include <stddef.h>

// Forward declarations
struct ember_vm;
struct ember_value;

// HTTP response structure
typedef struct {
    char* data;
    size_t size;
    long response_code;
    char* content_type;
    double download_time;
} http_response_t;

// HTTP function stubs
int ember_http_init(void);
void ember_http_cleanup(void);
int ember_http_get(const char* url, const char* auth_token, http_response_t* response);
int ember_http_download_file(const char* url, const char* local_path, const char* auth_token);
int ember_http_upload_file(const char* url, const char* file_path, const char* auth_token);
void http_response_cleanup(http_response_t* response);

// Request/response stub functions for web integration
struct ember_value ember_native_request_get_query(struct ember_vm* vm, int argc, struct ember_value* argv);
struct ember_value ember_native_request_get_header(struct ember_vm* vm, int argc, struct ember_value* argv);
struct ember_value ember_native_request_get_body(struct ember_vm* vm, int argc, struct ember_value* argv);
struct ember_value ember_native_response_set_status(struct ember_vm* vm, int argc, struct ember_value* argv);
struct ember_value ember_native_response_set_header(struct ember_vm* vm, int argc, struct ember_value* argv);
struct ember_value ember_native_response_write(struct ember_vm* vm, int argc, struct ember_value* argv);
struct ember_value ember_native_response_end(struct ember_vm* vm, int argc, struct ember_value* argv);
void ember_register_session_native(struct ember_vm* vm);

// Additional HTTP server functions
struct ember_value ember_native_http_listen_and_serve(struct ember_vm* vm, int argc, struct ember_value* argv);
struct ember_value ember_native_http_stop_server(struct ember_vm* vm, int argc, struct ember_value* argv);
struct ember_value ember_native_request_get_method(struct ember_vm* vm, int argc, struct ember_value* argv);
struct ember_value ember_native_request_get_path(struct ember_vm* vm, int argc, struct ember_value* argv);

#endif // EMBER_HTTP_STUBS_H