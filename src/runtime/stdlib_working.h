/**
 * Working stdlib function declarations
 */
#ifndef STDLIB_WORKING_H
#define STDLIB_WORKING_H

#include "ember.h"

// Crypto functions (working implementations)
ember_value ember_native_sha256_working(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_sha512_working(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_secure_random_working(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_hmac_sha256_working(ember_vm* vm, int argc, ember_value* argv);

// JSON functions (working implementations)
ember_value ember_json_parse_working(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_json_stringify_working(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_json_validate_working(ember_vm* vm, int argc, ember_value* argv);

// File I/O functions (working implementations)
ember_value ember_native_file_exists_working(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_read_file_working(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_write_file_working(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_append_file_working(ember_vm* vm, int argc, ember_value* argv);

#endif /* STDLIB_WORKING_H */