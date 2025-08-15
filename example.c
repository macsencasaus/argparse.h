#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define ARGPARSE_IMPLEMENTATION
#include "argparse.h"

typedef enum {
    MODE_FAST,
    MODE_SLOW,
    MODE_AUTO,
    MODE_COUNT,
} Mode;

const char *mode_options[] = {
    [MODE_FAST] = "fast",
    [MODE_SLOW] = "slow",
    [MODE_AUTO] = "auto",
};

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

int main(int argc, char **argv) {
    // Initialize parser
    argp_init(argc, argv,
              .desc = "Example program demonstrating argparse usage");

    // Define arguments

    // Flag arguments
    bool *verbose = argp_flag_bool("v", "verbose", .desc = "enable verbose output");
    uint64_t *retries = argp_flag_uint("r", "retries", 3,
                                       .meta_var = "N", .desc = "number of retries");
    char **output_file = argp_flag_str("o", "output", "default.txt",
                                       .meta_var = "FILE", .desc = "output file name");
    Argp_List *linker_args = argp_flag_list("L", NULL, .meta_var = "LIB",
                                            .desc = "Linker argument");

    // Positional Arguments
    uint64_t *id = argp_pos_uint("id", 0, .req = ARGP_APPEAR_REQUIRED,
                                 .desc = "the ID to process");
    char **name = argp_pos_str("name", "Yorgos Lanthimos", .desc = "the name to use");
    size_t *mode = argp_pos_enum("mode", mode_options, ARRAY_SIZE(mode_options), MODE_AUTO,
                                 .desc = "mode to use");
    Argp_List *files = argp_pos_list("files", .desc = "Files");

    // Commands
    bool *connect = argp_command("connect", .desc = "connect to something");

    bool *build = argp_command("build", .desc = "build program");
    bool *build_verbose = argp_flag_bool("v", "verbose",
                                         .desc = "verbose mode",
                                         .command = build);
    char **build_file = argp_pos_str("file", NULL, .desc = "file to build",
                                     .req = true,
                                     .command = build);

    if (!argp_parse_args()) {
        argp_print_error(stderr);
        return 1;
    }

    // Print parsed values
    printf("Verbose: %s\n", *verbose ? "true" : "false");
    printf("Retries: %llu\n", (unsigned long long)*retries);
    printf("Output file: %s\n", *output_file);
    printf("ID: %llu\n", (unsigned long long)*id);
    printf("Name: %s\n", *name);
    printf("Mode: %s\n", mode_options[*mode]);

    printf("Linker: ");
    for (size_t i = 0; i < linker_args->size; ++i) {
        printf("%s", linker_args->items[i]);

        if (i < linker_args->size - 1) {
            printf(", ");
        }
    }
    printf("\n");

    printf("Files: ");
    for (size_t i = 0; i < files->size; ++i) {
        printf("%s", files->items[i]);

        if (i < files->size - 1) {
            printf(", ");
        }
    }
    printf("\n");

    argp_free_list(files);
    argp_free_list(linker_args);

    return 0;
}
