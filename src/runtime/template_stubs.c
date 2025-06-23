/**
 * Template and HTML Function Stubs for Ember Core
 * Provides stub implementations to resolve linking issues in container builds
 * Real implementations are in ember-stdlib
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../vm.h"

/**
 * Helper function to get string value from ember_value
 * This should match the implementation expected by builtins
 */
const char* ember_get_string_value(ember_value value) {
    if (value.type == EMBER_VAL_STRING) {
        // Check for new GC-managed string representation first
        if (value.as.obj_val && value.as.obj_val->type == OBJ_STRING) {
            ember_string* str = AS_STRING(value);
            return str->chars;
        }
        // Fall back to old string representation
        else if (value.as.string_val) {
            return value.as.string_val;
        }
    }
    return NULL;
}

/**
 * Stub HTML render function
 */
ember_value ember_html_render(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[TEMPLATE] WARNING: HTML rendering not available - using stub implementation\n");
    printf("[TEMPLATE] Template functions require building with ember-stdlib\n");
    return ember_make_string("");
}

/**
 * Stub HTML render file function
 */
ember_value ember_html_render_file(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[TEMPLATE] WARNING: HTML file rendering not available - using stub implementation\n");
    printf("[TEMPLATE] Template functions require building with ember-stdlib\n");
    return ember_make_string("");
}

/**
 * Stub markdown render function
 */
ember_value ember_markdown_render(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[TEMPLATE] WARNING: Markdown rendering not available - using stub implementation\n");
    printf("[TEMPLATE] Template functions require building with ember-stdlib\n");
    return ember_make_string("");
}

/**
 * Stub markdown render file function
 */
ember_value ember_markdown_render_file(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[TEMPLATE] WARNING: Markdown file rendering not available - using stub implementation\n");
    printf("[TEMPLATE] Template functions require building with ember-stdlib\n");
    return ember_make_string("");
}

/**
 * Stub template render function
 */
ember_value ember_template_render(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[TEMPLATE] WARNING: Template rendering not available - using stub implementation\n");
    printf("[TEMPLATE] Template functions require building with ember-stdlib\n");
    return ember_make_string("");
}

/**
 * Stub template render file function
 */
ember_value ember_template_render_file(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[TEMPLATE] WARNING: Template file rendering not available - using stub implementation\n");
    printf("[TEMPLATE] Template functions require building with ember-stdlib\n");
    return ember_make_string("");
}

/**
 * Stub HTML escape function
 */
ember_value ember_html_escape(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        printf("[TEMPLATE] WARNING: HTML escape not available - using stub implementation\n");
        return ember_make_string("");
    }
    
    // Simple passthrough for stub - real implementation would escape HTML entities
    const char* input = ember_get_string_value(argv[0]);
    return ember_make_string(input ? input : "");
}

/**
 * Stub URL encode function
 */
ember_value ember_url_encode(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        printf("[TEMPLATE] WARNING: URL encode not available - using stub implementation\n");
        return ember_make_string("");
    }
    
    // Simple passthrough for stub - real implementation would URL encode
    const char* input = ember_get_string_value(argv[0]);
    return ember_make_string(input ? input : "");
}

/**
 * Stub replace all function
 */
ember_value ember_replace_all(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    
    if (argc != 3) {
        printf("[TEMPLATE] WARNING: String replace not available - using stub implementation\n");
        return ember_make_string("");
    }
    
    // Simple passthrough for stub - real implementation would do string replacement
    const char* input = ember_get_string_value(argv[0]);
    return ember_make_string(input ? input : "");
}

/**
 * Stub template truncate function
 */
ember_value ember_template_truncate(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    
    if (argc < 2) {
        printf("[TEMPLATE] WARNING: String truncate not available - using stub implementation\n");
        return ember_make_string("");
    }
    
    // Simple passthrough for stub - real implementation would truncate string
    const char* input = ember_get_string_value(argv[0]);
    return ember_make_string(input ? input : "");
}

// ============================================================================
// DATETIME STUB FUNCTIONS
// ============================================================================

/**
 * Stub datetime minute function
 */
ember_value ember_native_datetime_minute(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[DATETIME] WARNING: Datetime minute not available - using stub implementation\n");
    printf("[DATETIME] Datetime functions require building with ember-stdlib\n");
    return ember_make_number(0);
}

/**
 * Stub datetime second function
 */
ember_value ember_native_datetime_second(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[DATETIME] WARNING: Datetime second not available - using stub implementation\n");
    printf("[DATETIME] Datetime functions require building with ember-stdlib\n");
    return ember_make_number(0);
}

/**
 * Stub datetime weekday function
 */
ember_value ember_native_datetime_weekday(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[DATETIME] WARNING: Datetime weekday not available - using stub implementation\n");
    printf("[DATETIME] Datetime functions require building with ember-stdlib\n");
    return ember_make_number(0);
}

/**
 * Stub datetime yearday function
 */
ember_value ember_native_datetime_yearday(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    
    printf("[DATETIME] WARNING: Datetime yearday not available - using stub implementation\n");
    printf("[DATETIME] Datetime functions require building with ember-stdlib\n");
    return ember_make_number(0);
}

// Additional datetime stubs
ember_value ember_native_datetime_add(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(0); }
ember_value ember_native_datetime_diff(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(0); }
ember_value ember_native_datetime_to_utc(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(0); }
ember_value ember_native_datetime_from_utc(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(0); }
ember_value ember_native_datetime_year(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(2024); }
ember_value ember_native_datetime_month(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(1); }
ember_value ember_native_datetime_day(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(1); }
ember_value ember_native_datetime_hour(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(0); }
ember_value ember_native_datetime_timestamp(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(0); }
ember_value ember_native_datetime_format(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_string(""); }
ember_value ember_native_datetime_parse(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(0); }
ember_value ember_native_bcrypt_hash(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_string(""); }
ember_value ember_native_bcrypt_verify(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_bool(0); }
ember_value ember_native_datetime_now(ember_vm* vm, int argc, ember_value* argv) { (void)vm; (void)argc; (void)argv; return ember_make_number(0); }