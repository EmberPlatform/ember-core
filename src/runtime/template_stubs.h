/**
 * Template and HTML Function Stubs for Ember Core
 * Provides stub declarations to resolve linking issues in container builds
 */

#ifndef EMBER_TEMPLATE_STUBS_H
#define EMBER_TEMPLATE_STUBS_H

#include "../vm.h"

// Helper function
const char* ember_get_string_value(ember_value value);

// Template and HTML function stubs
ember_value ember_html_render(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_html_render_file(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_markdown_render(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_markdown_render_file(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_template_render(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_template_render_file(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_html_escape(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_url_encode(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_replace_all(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_template_truncate(ember_vm* vm, int argc, ember_value* argv);

// Datetime function stubs
ember_value ember_native_datetime_minute(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_second(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_weekday(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_yearday(ember_vm* vm, int argc, ember_value* argv);

#endif // EMBER_TEMPLATE_STUBS_H