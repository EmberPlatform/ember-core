#define _POSIX_C_SOURCE 200112L
#define _GNU_SOURCE
#include "ember.h"
#include "../../src/runtime/package/package.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>

// Conditionally include readline if available
#ifdef HAVE_READLINE
#include <readline/readline.h>
#include <readline/history.h>
#define USE_READLINE 1
#else
#define USE_READLINE 0
#endif

// ANSI color codes for help and error output
#define COLOR_RESET     "\033[0m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_GRAY      "\033[90m"

// REPL configuration
#define REPL_HISTORY_FILE ".ember_history"
#define MAX_INPUT_LENGTH 4096

// Keywords and built-in functions for tab completion
static const char* ember_keywords[] = {
    "if", "else", "while", "for", "fn", "return", "import", "break", "continue",
    "true", "false", "and", "or", "not", NULL
};

static const char* ember_builtins[] = {
    "print", "abs", "sqrt", "type", "len", "max", "min", "floor", "ceil", "round", "pow",
    "substr", "split", "join", "starts_with", "ends_with",
    "read_file", "write_file", "append_file", "file_exists",
    "str", "num", "int", "bool", NULL
};

// Multi-line editing state
typedef struct {
    char* buffer;
    size_t buffer_size;
    size_t buffer_used;
    int brace_depth;
    int paren_depth;
    int bracket_depth;
    int in_string;
    int needs_continuation;
} repl_state;

// Portable strdup implementation
#ifndef _GNU_SOURCE
static char* ember_strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* copy = malloc(len);
    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}
#define strdup ember_strdup
#endif

#if USE_READLINE
// Tab completion generator for keywords and builtins
static char* completion_generator(const char* text, int state) {
    static int list_index, len;
    static int in_keywords;
    const char* name;
    
    if (!state) {
        list_index = 0;
        len = strlen(text);
        in_keywords = 1;
    }
    
    // First, try keywords
    if (in_keywords) {
        while ((name = ember_keywords[list_index])) {
            list_index++;
            if (strncmp(name, text, len) == 0) {
                return strdup(name);
            }
        }
        in_keywords = 0;
        list_index = 0;
    }
    
    // Then try built-in functions
    while ((name = ember_builtins[list_index])) {
        list_index++;
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }
    
    return NULL;
}

// Tab completion function
static char** ember_completion(const char* text, int start, int end) {
    char** matches = NULL;
    (void)end; // Suppress unused parameter warning
    
    // Only complete if we're at the beginning of a word or after certain characters
    if (start == 0 || rl_line_buffer[start-1] == ' ' || rl_line_buffer[start-1] == '(' || 
        rl_line_buffer[start-1] == ',' || rl_line_buffer[start-1] == '\t') {
        matches = rl_completion_matches(text, completion_generator);
    }
    
    return matches;
}

// Initialize readline
static void init_readline(void) {
    rl_readline_name = "ember";
    rl_attempted_completion_function = ember_completion;
    rl_basic_word_break_characters = " \t\n\"\\`@$><=;|&{(,";
    
    // Load history
    read_history(REPL_HISTORY_FILE);
}

// Cleanup readline
static void cleanup_readline(void) {
    write_history(REPL_HISTORY_FILE);
    clear_history();
}
#endif

// Initialize REPL state
static void init_repl_state(repl_state* state) {
    state->buffer_size = MAX_INPUT_LENGTH;
    state->buffer = malloc(state->buffer_size);
    state->buffer_used = 0;
    state->brace_depth = 0;
    state->paren_depth = 0;
    state->bracket_depth = 0;
    state->in_string = 0;
    state->needs_continuation = 0;
    if (state->buffer) {
        state->buffer[0] = '\0';
    }
}

// Clean up REPL state
static void cleanup_repl_state(repl_state* state) {
    if (state->buffer) {
        free(state->buffer);
        state->buffer = NULL;
    }
}

// Check if input needs continuation (unclosed braces, parens, etc.)
static int needs_continuation(const char* input, repl_state* state) {
    state->brace_depth = 0;
    state->paren_depth = 0;
    state->bracket_depth = 0;
    state->in_string = 0;
    
    for (const char* p = input; *p; p++) {
        if (state->in_string) {
            if (*p == '"' && (p == input || *(p-1) != '\\')) {
                state->in_string = 0;
            }
            continue;
        }
        
        switch (*p) {
            case '"':
                state->in_string = 1;
                break;
            case '{':
                state->brace_depth++;
                break;
            case '}':
                state->brace_depth--;
                break;
            case '(':
                state->paren_depth++;
                break;
            case ')':
                state->paren_depth--;
                break;
            case '[':
                state->bracket_depth++;
                break;
            case ']':
                state->bracket_depth--;
                break;
        }
    }
    
    return state->brace_depth > 0 || state->paren_depth > 0 || 
           state->bracket_depth > 0 || state->in_string;
}

// Get input line with optional readline support
static char* get_input_line(const char* prompt, repl_state* state) {
    (void)state; // Suppress unused parameter warning in readline case
#if USE_READLINE
    char* line = readline(prompt);
    if (line && *line) {
        add_history(line);
    }
    return line;
#else
    static char buffer[MAX_INPUT_LENGTH];
    printf("%s", prompt);
    fflush(stdout);
    
    if (fgets(buffer, sizeof(buffer), stdin)) {
        // Remove newline
        buffer[strcspn(buffer, "\n")] = 0;
        return strdup(buffer);
    }
    return NULL;
#endif
}

// Append to multi-line buffer
static int append_to_buffer(repl_state* state, const char* line) {
    size_t line_len = strlen(line);
    size_t needed = state->buffer_used + line_len + 2; // +2 for newline and null terminator
    
    if (needed > state->buffer_size) {
        size_t new_size = state->buffer_size * 2;
        while (new_size < needed) {
            new_size *= 2;
        }
        
        char* new_buffer = realloc(state->buffer, new_size);
        if (!new_buffer) {
            return -1; // Out of memory
        }
        
        state->buffer = new_buffer;
        state->buffer_size = new_size;
    }
    
    if (state->buffer_used > 0) {
        strcat(state->buffer, "\n");
        state->buffer_used++;
    }
    
    strcat(state->buffer, line);
    state->buffer_used += line_len;
    
    return 0;
}

static void print_help(void) {
    printf("%s🔥 Ember%s - Lightweight Embedded Scripting Language\n\n", COLOR_BOLD COLOR_YELLOW, COLOR_RESET);
    
    printf("%sUSAGE:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %sember%s                           Start interactive REPL\n", COLOR_CYAN, COLOR_RESET);
    printf("  %sember%s %s<file>%s                   Execute script file\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("  %sember%s %s--mount <vfs:host> <file>%s Execute with VFS mount\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("  %sember%s %sinstall <name> <path>%s    Install library\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("  %sember%s %s--help%s                   Show this help message\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("  %sember%s %s--version%s                Show version information\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    
    printf("\n%sPERFORMANCE OPTIONS:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %sEMBER_PROFILE_STARTUP=1%s         Enable startup profiling\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sEMBER_LAZY_STDLIB=0%s             Disable lazy stdlib loading\n", COLOR_YELLOW, COLOR_RESET);
    printf("  %sEMBER_BYTECODE_CACHE=dir%s        Set bytecode cache directory\n", COLOR_YELLOW, COLOR_RESET);
    
    printf("\n%sEXAMPLES:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %s# Interactive mode with readline support%s\n", COLOR_GRAY, COLOR_RESET);
    printf("  ./ember\n");
    printf("  %s> 5 + 3 * 2%s  %s# Use Tab for completion, Up/Down for history%s\n", COLOR_GRAY, COLOR_RESET, COLOR_GRAY, COLOR_RESET);
    printf("  %s11%s\n", COLOR_GREEN, COLOR_RESET);
    
    printf("\n  %s# Execute from file%s\n", COLOR_GRAY, COLOR_RESET);
    printf("  echo \"x = 42\" > test.ember\n");
    printf("  ./ember test.ember\n");
    
    printf("\n  %s# Execute with VFS mounts%s\n", COLOR_GRAY, COLOR_RESET);
    printf("  ./ember --mount \"/app:/home/user/project\" script.ember\n");
    printf("  ./ember --mount \"/data:/tmp:ro\" script.ember\n");
    
    printf("\n  %s# Pipe input%s\n", COLOR_GRAY, COLOR_RESET);
    printf("  echo \"if 10 > 5 42 else 24\" | ./ember\n");
    
    printf("\n%sSUPPORTED FEATURES:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  • Arithmetic: %s+ - * / () unary -%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Variables: %sx = 42 (local and global scope)%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Comparisons: %s== != < > <= >=%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Logical: %sand or not (with proper precedence)%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Conditionals: %sif condition value else value%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Loops: %swhile condition expression%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Function calls: %sabs(-5) sqrt(16) print(42)%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Return statements: %sreturn 42, return%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Functions: %sfn name() {} (definition syntax)%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Modules: %simport module_name%s\n", COLOR_CYAN, COLOR_RESET);
    printf("  • Built-ins: %sprint() abs() sqrt() type()%s\n", COLOR_CYAN, COLOR_RESET);
    
    printf("\n%sSECURITY FEATURES:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  • Virtual filesystem with Docker-style bind mounts\n");
    printf("  • Default mounts: /app (cwd), /tmp (system tmp)\n");
    printf("  • Custom mounts: %s--mount \"/vfs:/host:rw\"%s\n", COLOR_YELLOW, COLOR_RESET);
    printf("  • Environment: %sEMBER_MOUNTS=\"/app:/project,/data:/tmp:ro\"%s\n", COLOR_YELLOW, COLOR_RESET);
    
    printf("\n%sIN DEVELOPMENT:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  • User-defined function calls with parameters\n");
    printf("  • For loops and break/continue statements\n");
    printf("  • Advanced data structures (lists, maps)\n");
    printf("  • Enhanced standard library with file I/O\n");
    
    printf("\n%sFor more information, visit:%s https://github.com/exec/ember\n", COLOR_GRAY, COLOR_RESET);
}

static void print_version(void) {
    printf("%sEmber v%s%s\n", COLOR_BOLD COLOR_CYAN, EMBER_VERSION, COLOR_RESET);
    printf("A lightweight embedded scripting language in C\n");
    printf("Built on %s at %s\n", __DATE__, __TIME__);
}


int main(int argc, char* argv[]) {
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    
    const char* mount_spec = NULL;
    const char* script_file = NULL;
    
    // Handle help and version flags first
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
            print_help();
            return 0;
        }
        if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
            print_version();
            return 0;
        }
        if (strcmp(argv[1], "install") == 0) {
            if (argc < 4) {
                fprintf(stderr, "%sError:%s Install command requires library name and source path\n", COLOR_RED, COLOR_RESET);
                fprintf(stderr, "Usage: ember install <library_name> <source_path>\n");
                return 1;
            }
            int result = ember_install_library(argv[2], argv[3]);
            return result == 0 ? 0 : 1;
        }
        
        // Handle --mount argument
        if (strcmp(argv[1], "--mount") == 0) {
            if (argc < 4) {
                fprintf(stderr, "%sError:%s --mount requires mount specification and script file\n", COLOR_RED, COLOR_RESET);
                fprintf(stderr, "Usage: ember --mount \"/vfs:/host\" script.ember\n");
                fprintf(stderr, "       ember --mount \"/vfs:/host:ro\" script.ember\n");
                return 1;
            }
            mount_spec = argv[2];
            script_file = argv[3];
        } else if (argc > 1) {
            script_file = argv[1];
        }
    }

    // Initialize package management system (temporarily disabled)
    /*
    if (!ember_package_system_init()) {
        fprintf(stderr, "%sError:%s Failed to initialize package management system\n", COLOR_RED, COLOR_RESET);
        return 1;
    }
    */
    
    // Create VM with standard configuration
    ember_vm* vm = ember_new_vm();
    if (!vm) {
        fprintf(stderr, "%sError:%s Failed to initialize Ember VM\n", COLOR_RED, COLOR_RESET);
        // ember_package_system_cleanup();  // Disabled
        return 1;
    }
    
    // Calculate and display startup time
    gettimeofday(&end_time, NULL);
    double startup_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
                         (end_time.tv_usec - start_time.tv_usec) / 1000.0;
    
    // Check for profiling environment variable
    if (getenv("EMBER_PROFILE_STARTUP")) {
        printf("%s[STARTUP PROFILE]%s Total startup time: %.2f ms\n", 
               COLOR_CYAN, COLOR_RESET, startup_time);
        // Startup profiling not available in this build
    }
    
    // Add custom mount if specified
    if (mount_spec) {
        // Parse mount specification: "/vfs:/host" or "/vfs:/host:ro"
        char* spec_copy = malloc(strlen(mount_spec) + 1);
        strcpy(spec_copy, mount_spec);
        
        char* colon1 = strchr(spec_copy, ':');
        if (!colon1) {
            fprintf(stderr, "%sError:%s Invalid mount specification '%s'\n", COLOR_RED, COLOR_RESET, mount_spec);
            fprintf(stderr, "Expected format: /vfs:/host or /vfs:/host:ro\n");
            free(spec_copy);
            ember_free_vm(vm);
            return 1;
        }
        
        *colon1 = '\0';
        char* virtual_path = spec_copy;
        char* host_part = colon1 + 1;
        
        int flags = EMBER_MOUNT_RW; // default
        char* colon2 = strchr(host_part, ':');
        if (colon2) {
            *colon2 = '\0';
            if (strcmp(colon2 + 1, "ro") == 0) {
                flags = EMBER_MOUNT_RO;
            } else if (strcmp(colon2 + 1, "rw") == 0) {
                flags = EMBER_MOUNT_RW;
            } else {
                fprintf(stderr, "%sError:%s Invalid mount flag '%s' (use 'ro' or 'rw')\n", 
                        COLOR_RED, COLOR_RESET, colon2 + 1);
                free(spec_copy);
                ember_free_vm(vm);
                return 1;
            }
        }
        
        if (ember_vfs_mount(vm, virtual_path, host_part, flags) != 0) {
            fprintf(stderr, "%sError:%s Failed to mount '%s' -> '%s'\n", 
                    COLOR_RED, COLOR_RESET, virtual_path, host_part);
            free(spec_copy);
            ember_free_vm(vm);
            return 1;
        }
        
        printf("%sInfo:%s Mounted %s%s%s -> %s%s%s (%s)\n", 
               COLOR_GREEN, COLOR_RESET, 
               COLOR_CYAN, virtual_path, COLOR_RESET,
               COLOR_YELLOW, host_part, COLOR_RESET,
               flags == EMBER_MOUNT_RO ? "read-only" : "read-write");
        
        free(spec_copy);
    }

    // Native functions are automatically registered by ember_new_vm() via register_builtin_functions()
    // This includes all math, string, file I/O, JSON, crypto, and type conversion functions

    // Check if we have a script file to execute
    if (script_file) {
        // File execution mode
        FILE* file = fopen(script_file, "r");
        if (!file) {
            fprintf(stderr, "%sError:%s Cannot open file '%s%s%s'\n", 
                    COLOR_RED, COLOR_RESET, COLOR_YELLOW, script_file, COLOR_RESET);
            ember_free_vm(vm);
            return 1;
        }

        // Read entire file
        fseek(file, 0, SEEK_END);
        long length = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        char* source = malloc(length + 1);
        if (!source) {
            fprintf(stderr, "%sError:%s Out of memory\n", COLOR_RED, COLOR_RESET);
            fclose(file);
            ember_free_vm(vm);
            return 1;
        }
        
        if (fread(source, 1, length, file) != (size_t)length) {
            printf("Error: Failed to read file completely\n");
            free(source);
            fclose(file);
            ember_free_vm(vm);
            return 1;
        }
        source[length] = '\0';
        fclose(file);

        // Skip shebang line if present
        char* exec_source = source;
        if (length >= 2 && source[0] == '#' && source[1] == '!') {
            // Find the end of the first line
            char* newline = strchr(source, '\n');
            if (newline) {
                exec_source = newline + 1; // Skip past the newline
            }
        }
        
        // AUTO-RESOLVE IMPORTS: Temporarily disabled due to issues
        // TODO: Re-enable once package system is stabilized
        /*
        if (strlen(exec_source) > 0) {
            printf("%s[AUTO-IMPORT]%s Checking for import statements...\n", COLOR_CYAN, COLOR_RESET);
            
            // Initialize package system
            if (!ember_package_system_init()) {
                printf("%s[WARNING]%s Could not initialize package system, imports may fail\n", COLOR_YELLOW, COLOR_RESET);
            } else {
                // Create temporary project to scan imports
                EmberProject* temp_project = ember_project_init("temp_script", "1.0.0");
                if (temp_project) {
                    // Create temporary file for scanning
                    FILE* temp_file = fopen("/tmp/temp_script.ember", "w");
                    if (temp_file) {
                        fprintf(temp_file, "%s", exec_source);
                        fclose(temp_file);
                        
                        // Scan for imports
                        if (ember_project_scan_imports("/tmp/temp_script.ember", temp_project)) {
                            if (temp_project->dependency_count > 0) {
                                printf("%s[AUTO-IMPORT]%s Found %zu import(s), installing...\n", 
                                       COLOR_CYAN, COLOR_RESET, temp_project->dependency_count);
                                
                                // Install all dependencies automatically
                                ember_project_install_dependencies(temp_project);
                                printf("%s[AUTO-IMPORT]%s Import resolution complete!\n", COLOR_GREEN, COLOR_RESET);
                            } else {
                                printf("%s[AUTO-IMPORT]%s No imports found\n", COLOR_GRAY, COLOR_RESET);
                            }
                        }
                        
                        // Clean up temp file
                        unlink("/tmp/temp_script.ember");
                    }
                    ember_project_cleanup(temp_project);
                }
                ember_package_system_cleanup();
            }
        }
        */
        
        // Execute the entire file as one unit
        int result = 0;
        if (strlen(exec_source) > 0) {
            if (ember_eval(vm, exec_source) != 0) {
                result = 1;
            } else {
                // Print the result (skip nil values) - only if there's something on the stack
                if (vm->stack_top > 0) {
                    ember_value res = ember_peek_stack_top(vm);
                    if (res.type != EMBER_VAL_NIL) {
                        ember_print_value(res);
                        printf("\n");
                    }
                }
            }
        }
        
        free(source);
        ember_free_vm(vm);
        return result;
    }

    // Check if stdin is a terminal (interactive mode) or pipe/redirect
    int is_interactive = isatty(fileno(stdin));
    
    if (is_interactive) {
        // Interactive REPL mode with enhanced features
        printf("Ember v%s REPL\n", EMBER_VERSION);
        printf("Type 'exit' to quit\n");
        
#if USE_READLINE
        printf("Features: readline support, tab completion, command history, multi-line editing\n");
        printf("Press Tab to complete keywords/functions, Up/Down arrows for history\n");
        init_readline();
#else
        printf("Features: multi-line editing (readline not available)\n");
        printf("Note: Install readline development libraries and recompile for enhanced features\n");
#endif
        printf("Supports: arithmetic expressions with +, -, *, /, (), unary -\n");
    }

    // Initialize REPL state for multi-line editing
    repl_state state;
    init_repl_state(&state);
    
    while (1) {
        char* line = NULL;
        const char* prompt = is_interactive ? (state.buffer_used > 0 ? "... " : "> ") : "";
        
        // Get input line
        line = get_input_line(prompt, &state);
        if (!line) {
            break; // EOF or error
        }

        // Skip empty lines when not in multi-line mode
        if (strlen(line) == 0 && state.buffer_used == 0) {
            free(line);
            continue;
        }

        // Check for exit command
        if (strcmp(line, "exit") == 0) {
            free(line);
            break;
        }
        
        // Check for special commands
        if (state.buffer_used == 0 && strncmp(line, "clear", 5) == 0) {
            if (is_interactive) {
#if USE_READLINE
                rl_clear_screen(0, 0);
#else
                system("clear");
#endif
            }
            free(line);
            continue;
        }
        
        // Add line to buffer
        if (append_to_buffer(&state, line) != 0) {
            fprintf(stderr, "%sError:%s Out of memory\n", COLOR_RED, COLOR_RESET);
            free(line);
            break;
        }
        
        // Check if we need continuation
        if (needs_continuation(state.buffer, &state)) {
            state.needs_continuation = 1;
            free(line);
            continue;
        }
        
        // We have a complete expression, execute it
        char* complete_input = strdup(state.buffer);
        
        // Reset state for next input
        state.buffer_used = 0;
        state.buffer[0] = '\0';
        state.needs_continuation = 0;
        
        // AUTO-RESOLVE IMPORTS: Check if this is an import statement
        if (strncmp(complete_input, "import ", 7) == 0) {
            if (is_interactive) {
                printf("%s[AUTO-IMPORT]%s Installing package...\n", COLOR_CYAN, COLOR_RESET);
            }
            
            // Initialize package system for single import
            if (ember_package_system_init()) {
                EmberProject* temp_project = ember_project_init("repl_session", "1.0.0");
                if (temp_project) {
                    // Create temporary file for this import
                    FILE* temp_file = fopen("/tmp/repl_import.ember", "w");
                    if (temp_file) {
                        fprintf(temp_file, "%s\n", complete_input);
                        fclose(temp_file);
                        
                        // Scan and install the import
                        if (ember_project_scan_imports("/tmp/repl_import.ember", temp_project)) {
                            ember_project_install_dependencies(temp_project);
                            if (is_interactive && temp_project->dependency_count > 0) {
                                printf("%s[AUTO-IMPORT]%s Package installed!\n", COLOR_GREEN, COLOR_RESET);
                            }
                        }
                        
                        unlink("/tmp/repl_import.ember");
                    }
                    ember_project_cleanup(temp_project);
                }
                ember_package_system_cleanup();
            }
        }

        // Execute the complete input
        if (ember_eval(vm, complete_input) != 0) {
            // Errors already printed by parser
            if (!is_interactive) {
                free(complete_input);
                free(line);
                break;
            }
        } else {
            // Print the result (skip nil values) - only if there's something on the stack
            if (vm->stack_top > 0) {
                ember_value result = ember_peek_stack_top(vm);
                if (result.type != EMBER_VAL_NIL) {
                    ember_print_value(result);
                    printf("\n");
                }
            }
        }
        
        free(complete_input);
        free(line);
    }

    // Cleanup
    cleanup_repl_state(&state);
    
#if USE_READLINE
    if (is_interactive) {
        cleanup_readline();
    }
#endif

    ember_free_vm(vm);
    ember_package_system_cleanup();
    return 0;
}