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
    // Initialize parser with optional description
    argp_init(argc, argv,
              .desc = "Example program demonstrating argparse usage");

    // Define arguments

    // Flag arguments
    // Arguments that require a flag, prefixed with - or --
    // See Argp_Flag_Opt for options

    // bool only requires the existence of the flag, no arguments
    //
    // specifies short name prefixed by -
    // specifies long name prefixed by --
    // one of the names may be left NULL but not both
    //
    // adds an optional description
    bool *verbose = argp_flag_bool(/* short name */ "v", /* long name */ "verbose",
                                   .desc = "enable verbose output");

    // uint and str require an argument following the flag with a space
    // -r 1   (Good)
    // -r1    (Bad)

    // specify default argument value
    // specify meta variable (variable used in help print out)
    uint64_t *retries = argp_flag_uint("r", "retries", /* default */ 3,
                                       .meta_var = "N", .desc = "number of retries");

    char **output_file = argp_flag_str("o", "output", "default.txt",
                                       .meta_var = "FILE", .desc = "output file name");

    // list
    // example: -L x -L y -L z
    // creates the list ["x", "y", "z"]
    Argp_List *linker_args = argp_flag_list("L", NULL, .meta_var = "LIB",
                                            .desc = "Linker argument");

    // All functions for flags are:
    //
    //  argp_flag_bool
    //  argp_flag_uint
    //  argp_flag_str
    //  argp_flag_enum
    //  argp_flag_list

    // Positional Arguments
    // Arguments that dont't require a flag and are derived based on their position
    // The order which they are specified is the order the arguments are expected
    //
    // See Argp_Pos_Opt for options

    // Specifies name for help print out.
    // Specifies default argument value.
    // .req is a field to specify whether a positional argument is required.
    // Options are ARGP_OPTIONAL, ARGP_REQUIRED, ARGP_APPEAR_REQUIRED.
    // The parser will error if a positional argument that is ARGP_REQUIRED is not present.
    // ARGP_OPTIONAL changes the syntax in the help printout to [<name>].
    // If you want it appear required in the print out and handle errors on your own,
    // use ARGP_APPEAR_REQUIRED.
    uint64_t *id = argp_pos_uint(/* name */ "id", /* default */ 0,
                                 .req = ARGP_APPEAR_REQUIRED, .desc = "the ID to process");

    char **name = argp_pos_str("name", "Yorgos Lanthimos", .desc = "the name to use");

    // enum allows you to specify a mapping from strings to a numerical value.
    // It will display the allowed strings and error if string is not recognized.
    // It will skip any NULL values in the provided array.
    size_t *mode = argp_pos_enum("mode", mode_options, ARRAY_SIZE(mode_options), MODE_AUTO,
                                 .desc = "mode to use");

    // Positional list arguments must be the last positional arguments specified
    Argp_List *files = argp_pos_list("files", .desc = "Files");

    // All functions for positional arguments are
    //
    // argp_pos_uint
    // argp_pos_str
    // argp_pos_enum
    // argp_pos_list

    // Commands
    // Arguments that can hold other arguments (think of git clone).
    // By default, help is enabled (-h, --help).
    // See Argp_Command_Opt for options.

    bool *connect = argp_command("connect", .desc = "connect to something");

    bool *build = argp_command("build", .desc = "build program");

    // Bind an argument to a command with the .command field
    bool *build_verbose = argp_flag_bool("v", "verbose",
                                         .desc = "verbose mode",
                                         .command = build);

    char **build_file = argp_pos_str("file", NULL, .desc = "file to build",
                                     .req = true,
                                     .command = build);

    // parse and handle errors
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
