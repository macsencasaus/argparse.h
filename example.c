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
              "Example program demonstrating argparse usage",
              /* default_help */ true);

    // Define arguments
    bool *verbose = argp_flag_bool("v", "verbose", "enable verbose output");
    uint64_t *retries = argp_flag_uint("r", "retries", "N", 3, "number of retries");
    char **output_file = argp_flag_str("o", "output", "FILE", "default.txt", "output file name");
    Argp_List *linker_args = argp_flag_list("L", NULL, "LIB", "Linker argument");

    uint64_t *id = argp_pos_uint("id", 0, /* opt */ ARGP_OPT_APPEAR_REQUIRED, "the ID to process");
    char **name = argp_pos_str("name", "Yorgos Lanthimos", /* opt */ true, "the name to use");
    size_t *mode = argp_pos_enum("mode", mode_options, ARRAY_SIZE(mode_options), MODE_AUTO, /* opt */ true, "mode to use");
    Argp_List *files = argp_pos_list("files", ARGP_OPT_OPTIONAL, "Files");

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
