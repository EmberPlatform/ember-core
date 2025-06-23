/**
 * Enhanced HTTP implementation for Ember runtime
 * Provides full HTTP client functionality with libcurl integration
 */

#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <time.h>

// Response buffer structure for libcurl callbacks
typedef struct {
    char* memory;
    size_t size;
} http_memory_struct_t;

// HTTP response structure 
typedef struct {
    char* data;
    size_t size;
    long response_code;
    char* content_type;
    double download_time;
    char* headers;
    size_t headers_size;
} http_response_t;

static int g_http_initialized = 0;

// libcurl callback for writing response data
static size_t write_memory_callback(void* contents, size_t size, size_t nmemb, http_memory_struct_t* mem) {
    size_t realsize = size * nmemb;
    
    char* ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        // Out of memory
        printf("HTTP: Not enough memory (realloc returned NULL)\n");
        return 0;
    }
    
    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;
    
    return realsize;
}

// Initialize HTTP subsystem
int ember_http_init(void) {
    if (g_http_initialized) {
        return 0; // Already initialized
    }
    
    CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (res != CURLE_OK) {
        printf("HTTP: Failed to initialize libcurl: %s\n", curl_easy_strerror(res));
        return -1;
    }
    
    g_http_initialized = 1;
    printf("HTTP: libcurl initialized successfully\n");
    return 0;
}

// Cleanup HTTP subsystem
void ember_http_cleanup(void) {
    if (g_http_initialized) {
        curl_global_cleanup();
        g_http_initialized = 0;
        printf("HTTP: libcurl cleanup completed\n");
    }
}

// Generic HTTP request function
static http_response_t* http_request(const char* method, const char* url, const char* data, 
                                   const char* headers[], const char* auth_token) {
    if (!g_http_initialized) {
        if (ember_http_init() != 0) {
            return NULL;
        }
    }
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }
    
    http_response_t* response = malloc(sizeof(http_response_t));
    if (!response) {
        curl_easy_cleanup(curl);
        return NULL;
    }
    
    memset(response, 0, sizeof(http_response_t));
    
    http_memory_struct_t chunk = {0};
    http_memory_struct_t header_chunk = {0};
    
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    // Set method
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (data) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
        }
    } else if (strcmp(method, "PUT") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
        if (data) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
        }
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (strcmp(method, "PATCH") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
        if (data) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(data));
        }
    }
    // GET is default, no special handling needed
    
    // Set response callbacks
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&chunk);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*)&header_chunk);
    
    // Security settings
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "EmberWeb/2.0.0 (Ember HTTP Client)");
    
    // Set headers
    struct curl_slist* header_list = NULL;
    
    // Add authorization header if provided
    if (auth_token) {
        char auth_header[1024];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", auth_token);
        header_list = curl_slist_append(header_list, auth_header);
    }
    
    // Add custom headers
    if (headers) {
        for (int i = 0; headers[i] != NULL; i++) {
            header_list = curl_slist_append(header_list, headers[i]);
        }
    }
    
    // Set content type for POST/PUT/PATCH with data
    if (data && (strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0 || strcmp(method, "PATCH") == 0)) {
        header_list = curl_slist_append(header_list, "Content-Type: application/json");
    }
    
    if (header_list) {
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_list);
    }
    
    // Perform request
    CURLcode res = curl_easy_perform(curl);
    
    if (res != CURLE_OK) {
        printf("HTTP: Request failed: %s\n", curl_easy_strerror(res));
        free(response);
        free(chunk.memory);
        free(header_chunk.memory);
        curl_slist_free_all(header_list);
        curl_easy_cleanup(curl);
        return NULL;
    }
    
    // Get response info
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response->response_code);
    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &response->download_time);
    
    char* content_type = NULL;
    curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type);
    if (content_type) {
        response->content_type = strdup(content_type);
    }
    
    // Store response data
    response->data = chunk.memory;
    response->size = chunk.size;
    response->headers = header_chunk.memory;
    response->headers_size = header_chunk.size;
    
    // Cleanup
    curl_slist_free_all(header_list);
    curl_easy_cleanup(curl);
    
    printf("HTTP: %s %s -> %ld (%.2fs, %zu bytes)\n", 
           method, url, response->response_code, response->download_time, response->size);
    
    return response;
}

// Free HTTP response
static void free_http_response(http_response_t* response) {
    if (response) {
        free(response->data);
        free(response->content_type);
        free(response->headers);
        free(response);
    }
}

// Convert HTTP response to Ember object
static ember_value http_response_to_ember_object(ember_vm* vm, http_response_t* response) {
    if (!response) {
        return ember_make_nil();
    }
    
    // For now, return a simple string with the response data
    // TODO: Return a proper object with status, headers, data
    if (response->data && response->size > 0) {
        ember_value result = ember_make_string_gc(vm, response->data);
        free_http_response(response);
        return result;
    }
    
    free_http_response(response);
    return ember_make_string_const("");
}

// HTTP GET binding
ember_value ember_native_http_get(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* url_str = AS_STRING(argv[0]);
    const char* auth_token = NULL;
    
    if (argc >= 2 && argv[1].type == EMBER_VAL_STRING) {
        ember_string* auth_str = AS_STRING(argv[1]);
        auth_token = auth_str->chars;
    }
    
    http_response_t* response = http_request("GET", url_str->chars, NULL, NULL, auth_token);
    return http_response_to_ember_object(vm, response);
}

// HTTP POST binding
ember_value ember_native_http_post(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* url_str = AS_STRING(argv[0]);
    ember_string* data_str = AS_STRING(argv[1]);
    const char* auth_token = NULL;
    
    if (argc >= 3 && argv[2].type == EMBER_VAL_STRING) {
        ember_string* auth_str = AS_STRING(argv[2]);
        auth_token = auth_str->chars;
    }
    
    http_response_t* response = http_request("POST", url_str->chars, data_str->chars, NULL, auth_token);
    return http_response_to_ember_object(vm, response);
}

// HTTP PUT binding
ember_value ember_native_http_put(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* url_str = AS_STRING(argv[0]);
    ember_string* data_str = AS_STRING(argv[1]);
    const char* auth_token = NULL;
    
    if (argc >= 3 && argv[2].type == EMBER_VAL_STRING) {
        ember_string* auth_str = AS_STRING(argv[2]);
        auth_token = auth_str->chars;
    }
    
    http_response_t* response = http_request("PUT", url_str->chars, data_str->chars, NULL, auth_token);
    return http_response_to_ember_object(vm, response);
}

// HTTP DELETE binding
ember_value ember_native_http_delete(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* url_str = AS_STRING(argv[0]);
    const char* auth_token = NULL;
    
    if (argc >= 2 && argv[1].type == EMBER_VAL_STRING) {
        ember_string* auth_str = AS_STRING(argv[1]);
        auth_token = auth_str->chars;
    }
    
    http_response_t* response = http_request("DELETE", url_str->chars, NULL, NULL, auth_token);
    return http_response_to_ember_object(vm, response);
}

// HTTP PATCH binding
ember_value ember_native_http_patch(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* url_str = AS_STRING(argv[0]);
    ember_string* data_str = AS_STRING(argv[1]);
    const char* auth_token = NULL;
    
    if (argc >= 3 && argv[2].type == EMBER_VAL_STRING) {
        ember_string* auth_str = AS_STRING(argv[2]);
        auth_token = auth_str->chars;
    }
    
    http_response_t* response = http_request("PATCH", url_str->chars, data_str->chars, NULL, auth_token);
    return http_response_to_ember_object(vm, response);
}

// Simple HTTP download function  
ember_value ember_native_http_download(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* url_str = AS_STRING(argv[0]);
    ember_string* file_str = AS_STRING(argv[1]);
    
    http_response_t* response = http_request("GET", url_str->chars, NULL, NULL, NULL);
    if (!response || response->response_code != 200) {
        free_http_response(response);
        return ember_make_bool(0);
    }
    
    // Write to file
    FILE* file = fopen(file_str->chars, "wb");
    if (!file) {
        free_http_response(response);
        return ember_make_bool(0);
    }
    
    size_t written = fwrite(response->data, 1, response->size, file);
    fclose(file);
    
    int success = (written == response->size);
    free_http_response(response);
    
    return ember_make_bool(success);
}