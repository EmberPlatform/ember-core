#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

// ANSI color codes
#define COLOR_RESET     "\033[0m"
#define COLOR_BOLD      "\033[1m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_GRAY      "\033[90m"
#define COLOR_BLUE      "\033[34m"

static void print_help(void) {
    printf("%s⚡ emberc%s - Ember Language Compiler\n\n", COLOR_BOLD COLOR_YELLOW, COLOR_RESET);
    
    printf("%sUSAGE:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  %semberc%s %s<file>%s           Compile and test source file\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("  %semberc%s %s--help%s          Show this help message\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    printf("  %semberc%s %s--version%s       Show version information\n", COLOR_CYAN, COLOR_RESET, COLOR_YELLOW, COLOR_RESET);
    
    printf("\n%sEXAMPLE:%s\n", COLOR_BOLD, COLOR_RESET);
    printf("  ./emberc program.ember\n");
    
    printf("\n%sNote:%s This tool compiles Ember source code and runs basic tests\n", COLOR_YELLOW, COLOR_RESET);
    printf("to validate the compilation was successful.\n");
}

static void print_version(void) {
    printf("%sEmberc v%s%s\n", COLOR_BOLD COLOR_CYAN, EMBER_VERSION, COLOR_RESET);
    printf("Ember Language Compiler and Tester\n");
    printf("Built on %s at %s\n", __DATE__, __TIME__);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "%sError:%s Missing source file argument\n\n", COLOR_RED, COLOR_RESET);
        print_help();
        return 1;
    }
    
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_help();
        return 0;
    }
    
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        print_version();
        return 0;
    }
    
    FILE* file = fopen(argv[1], "r");
    if (!file) {
        fprintf(stderr, "%sError:%s Could not open file '%s%s%s'\n", 
                COLOR_RED, COLOR_RESET, COLOR_YELLOW, argv[1], COLOR_RESET);
        return 1;
    }

    // Read the file content
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* source = (char*)malloc(file_size + 1);
    if (!source) {
        fprintf(stderr, "%sError:%s Out of memory\n", COLOR_RED, COLOR_RESET);
        fclose(file);
        return 1;
    }
    fread(source, 1, file_size, file);
    source[file_size] = '\0';
    fclose(file);

    printf("%s⚡ Ember Compiler v%s%s\n", COLOR_BOLD COLOR_YELLOW, EMBER_VERSION, COLOR_RESET);
    printf("%sCompiling:%s %s%s%s\n\n", COLOR_GRAY, COLOR_RESET, COLOR_CYAN, argv[1], COLOR_RESET);
    
    // Use the VM to compile and execute the source
    ember_vm* vm = ember_new_vm();
    if (!vm) {
        fprintf(stderr, "%sError:%s Could not create VM\n", COLOR_RED, COLOR_RESET);
        free(source);
        return 1;
    }
    
    printf("%s[INFO]%s Compiling and testing source...\n", COLOR_BLUE, COLOR_RESET);
    int result = ember_eval(vm, source);
    
    if (result == 0) {
        printf("%s[SUCCESS]%s Compilation successful! ✓\n", COLOR_GREEN, COLOR_RESET);
        ember_value top = ember_peek_stack_top(vm);
        if (top.type == EMBER_VAL_NUMBER) {
            printf("%sResult:%s %g\n", COLOR_CYAN, COLOR_RESET, top.as.number_val);
        } else if (top.type == EMBER_VAL_BOOL) {
            printf("%sResult:%s %s\n", COLOR_CYAN, COLOR_RESET, top.as.bool_val ? "true" : "false");
        }
    } else {
        printf("%s[ERROR]%s Compilation failed with error code: %d\n", COLOR_RED, COLOR_RESET, result);
    }
    
    ember_free_vm(vm);
    free(source);
    return result;
}