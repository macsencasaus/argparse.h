// argparse.h -- command-line argument parsing
//
//    Inspired by Python argparse module and github.com/tsoding/flag.h
//
// Macros API:
// - ARGP_FLAG_CAP - how many flags you can define
// - ARGP_POS_CAP - how many positional arguments you can define
// - ARGP_COMMAND_CAP - how many commands you can define
// - ARGP_PRINT_WIDTH - width at which description of argument is printed in argp_print_usage function
// - ARGP_ASSERT - assert function
// - ARGP_REALLOC - realloc function
// - ARGP_LIST_INIT_CAP - initial capacity of argp list
#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef enum {
    ARGP_OPTIONAL = false,
    ARGP_REQUIRED = true,
    ARGP_APPEAR_REQUIRED,
} Argp_Required;

typedef struct {
    char **items;
    size_t size;
    size_t _cap;
} Argp_List;

typedef struct {
    const char *desc;
    bool help;
} Argp_Opt;

typedef struct {
    const char *desc;
    bool help;
    const bool *command;
} Argp_Command_Opt;

typedef struct {
    const char *desc;
    const char *meta_var;
    const bool *command;
} Argp_Flag_Opt;

typedef struct {
    const char *desc;
    Argp_Required req;
    const bool *command;
} Argp_Pos_Opt;

#define argp_init(argc, argv, ...) \
    argp_init_(argc, argv, (Argp_Opt){.help = true, __VA_ARGS__})
void argp_init_(int argc, char **argv, Argp_Opt opt);

// returns name of flag given its return value
const char *argp_name(void *val);

#define argp_command(name, ...) \
    argp_command_(name, (Argp_Command_Opt){.help = true, __VA_ARGS__})
bool *argp_command_(const char *name, Argp_Command_Opt opt);

#define argp_flag_bool(short_name, long_name, ...) \
    argp_flag_bool_(short_name, long_name, (Argp_Flag_Opt){__VA_ARGS__})
bool *argp_flag_bool_(const char *short_name, const char *long_name, Argp_Flag_Opt opt);

#define argp_flag_uint(short_name, long_name, def, ...) \
    argp_flag_uint_(short_name, long_name, def, (Argp_Flag_Opt){__VA_ARGS__})
uint64_t *argp_flag_uint_(const char *short_name, const char *long_name, uint64_t def,
                          Argp_Flag_Opt opt);

#define argp_flag_str(short_name, long_name, def, ...) \
    argp_flag_str_(short_name, long_name, def, (Argp_Flag_Opt){__VA_ARGS__})
char **argp_flag_str_(const char *short_name, const char *long_name, char *def,
                      Argp_Flag_Opt opt);

#define argp_flag_enum(short_name, long_name, options, option_count, def, ...) \
    argp_flag_enum_(short_name, long_name, options, option_count,              \
                    def, (Argp_Flag_Opt){__VA_ARGS__})
size_t *argp_flag_enum_(const char *short_name, const char *long_name, const char *options[],
                        size_t option_count, size_t def, Argp_Flag_Opt opt);

#define argp_flag_list(short_name, long_name, ...) \
    argp_flag_list_(short_name, long_name, (Argp_Flag_Opt){__VA_ARGS__})
Argp_List *argp_flag_list_(const char *short_name, const char *long_name, Argp_Flag_Opt opt);

#define argp_pos_uint(name, def, ...) \
    argp_pos_uint_(name, def, (Argp_Pos_Opt){__VA_ARGS__})
uint64_t *argp_pos_uint_(const char *name, uint64_t def, Argp_Pos_Opt opt);

#define argp_pos_str(name, def, ...) \
    argp_pos_str_(name, def, (Argp_Pos_Opt){__VA_ARGS__})
char **argp_pos_str_(const char *name, char *def, Argp_Pos_Opt opt);

#define argp_pos_enum(name, options, option_count, def, ...) \
    argp_pos_enum_(name, options, option_count, def, (Argp_Pos_Opt){__VA_ARGS__})
size_t *argp_pos_enum_(const char *name, const char *options[], size_t option_count,
                       size_t def, Argp_Pos_Opt opt);

// NOTE: must be placed as the last positional argument
#define argp_pos_list(name, ...) \
    argp_pos_list_(name, (Argp_Pos_Opt){__VA_ARGS__})
Argp_List *argp_pos_list_(const char *name, Argp_Pos_Opt opt);

void argp_free_list(Argp_List *list);

bool argp_parse_args(void);

void argp_print_usage(FILE *stream);
void argp_print_error(FILE *stream);

#endif  // ARGPARSE_H

#ifdef ARGPARSE_IMPLEMENTATION

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#ifndef ARGP_FLAG_CAP
#define ARGP_FLAG_CAP 128
#endif

#ifndef ARGP_POS_CAP
#define ARGP_POS_CAP 128
#endif

#ifndef ARGP_COMMAND_CAP
#define ARGP_COMMAND_CAP 64
#endif

#ifndef ARGP_PRINT_WIDTH
#define ARGP_PRINT_WIDTH 24
#endif

#ifndef ARGP_LIST_INIT_CAP
#define ARGP_LIST_INIT_CAP 6
#endif

#ifndef ARGP_ASSERT
#include <assert.h>
#define ARGP_ASSERT assert
#endif

#ifndef ARGP_REALLOC
#include <stdlib.h>
#define ARGP_REALLOC realloc
#endif

#ifndef ARGP_FREE
#include <stdlib.h>
#define ARGP_FREE free
#endif

typedef enum {
    ARGP_BOOL,
    ARGP_UINT,
    ARGP_STR,
    ARGP_ENUM,
    ARGP_LIST,
    ARGP_TYPE_COUNT,
} Argp_Type;

typedef enum {
    ARGP_NO_ERROR = 0,
    ARGP_ERROR_UNKNOWN,
    ARGP_ERROR_UNKNOWN_ENUM,
    ARGP_ERROR_NO_VALUE,
    ARGP_ERROR_INVALID_NUMBER,
    ARGP_ERROR_INTEGER_OVERFLOW,
    ARGP_ERROR_ALLOC,
    ARGP_ERROR_COUNT,
} Argp_Error;

typedef union {
    bool as_bool;
    uint64_t as_uint;
    char *as_str;
    size_t as_enum;
    Argp_List as_list;
} Argp_Value;

typedef struct Argp_Flag Argp_Flag;
typedef struct Argp_Pos Argp_Pos;
typedef struct Argp_Command Argp_Command;

struct Argp_Command {
    bool val;
    const char *name;
    const char *desc;
    Argp_Flag *help_flag;
    const Argp_Command *parent_command;

    size_t pos_count;
    size_t flag_count;
    size_t command_count;

    size_t cur_pos;
};

struct Argp_Flag {
    Argp_Value val;
    Argp_Value def;

    Argp_Type type;
    const char *short_name;
    const char *long_name;

    const char *meta_var;
    const char *desc;

    const char **enum_options;
    size_t option_count;

    const Argp_Command *command;
};

struct Argp_Pos {
    Argp_Value val;
    Argp_Value def;

    Argp_Type type;
    const char *name;
    const char *desc;
    Argp_Required req;

    const char **enum_options;
    size_t option_count;

    const Argp_Command *command;
    bool seen;
};

typedef struct {
    size_t flag_capacity;
    Argp_Flag flags[ARGP_FLAG_CAP];

    size_t pos_capacity;
    Argp_Pos poss[ARGP_POS_CAP];

    size_t command_capacity;
    Argp_Command commands[ARGP_COMMAND_CAP];

    // const char *program_name;
    // const char *program_desc;

    // Argp_Flag *help_flag;

    Argp_Error err;
    Argp_Flag *err_flag;
    Argp_Pos *err_pos;
    const char *unknown_option;

    int rest_argc;
    char **rest_argv;

    Argp_Command *program_command;
    Argp_Command *command_ctx;
} Argp_Ctx;

static Argp_Ctx argp_global_ctx;

static char *shift_args(void) {
    Argp_Ctx *c = &argp_global_ctx;
    if (c->rest_argc == 0) return NULL;
    char *res = c->rest_argv[0];
    --c->rest_argc;
    ++c->rest_argv;
    return res;
}

static Argp_Flag *argp_new_flag(Argp_Type type, const char *short_name, const char *long_name,
                                const char *meta_var, const char *desc, Argp_Command *command) {
    ARGP_ASSERT(short_name != NULL || long_name != NULL);
    ARGP_ASSERT(argp_global_ctx.flag_capacity < ARGP_FLAG_CAP);

    Argp_Ctx *c = &argp_global_ctx;
    Argp_Flag *flag = c->flags + (c->flag_capacity++);
    command = command ? command : c->program_command;

    *flag = (Argp_Flag){
        .type = type,
        .short_name = short_name,
        .long_name = long_name,
        .meta_var = meta_var,
        .desc = desc,
        .command = command,
    };

    ++command->flag_count;
    return flag;
}

static Argp_Pos *argp_new_pos(Argp_Type type, const char *name, const char *desc,
                              Argp_Required req, Argp_Command *command) {
    ARGP_ASSERT(name != NULL);

    Argp_Ctx *c = &argp_global_ctx;
    ARGP_ASSERT(c->pos_capacity < ARGP_POS_CAP);

    Argp_Pos *pos = c->poss + (c->pos_capacity++);
    command = command ? command : c->program_command;

    *pos = (Argp_Pos){
        .type = type,
        .name = name,
        .desc = desc,
        .req = req,
        .command = command,
    };

    ++command->pos_count;
    return pos;
}

bool *argp_command_(const char *name, Argp_Command_Opt opt) {
    ARGP_ASSERT(name != NULL);

    Argp_Ctx *c = &argp_global_ctx;
    ARGP_ASSERT(c->command_capacity < ARGP_COMMAND_CAP);

    Argp_Command *command = c->commands + (c->command_capacity++);
    Argp_Command *parent_command = opt.command ? (Argp_Command *)opt.command : c->program_command;

    *command = (Argp_Command){
        .name = name,
        .desc = opt.desc,
        .parent_command = parent_command,
    };
    if (opt.help)
        command->help_flag = argp_new_flag(ARGP_BOOL, "h", "help", NULL, "show this help message and exit", command);

    if (parent_command)
        ++parent_command->command_count;

    return &command->val;
}

static void print_full_command_name(FILE *stream, const Argp_Command *command) {
    if (command->parent_command)
        print_full_command_name(stream, command->parent_command);
    fprintf(stream, " %s", command->name);
}

void argp_init_(int argc, char **argv, Argp_Opt opt) {
    Argp_Ctx *c = &argp_global_ctx;

    *c = (Argp_Ctx){0};

    c->rest_argc = argc;
    c->rest_argv = argv;

    c->program_command = (Argp_Command *)argp_command_(argv[0], (Argp_Command_Opt){.desc = opt.desc, .help = opt.help});
    c->command_ctx = NULL;
}

bool *argp_flag_bool_(const char *short_name, const char *long_name, Argp_Flag_Opt opt) {
    Argp_Flag *flag = argp_new_flag(
        ARGP_BOOL,
        short_name,
        long_name,
        NULL,
        opt.desc,
        (Argp_Command *)opt.command);
    return &flag->val.as_bool;
}

uint64_t *argp_flag_uint_(const char *short_name, const char *long_name, uint64_t def, Argp_Flag_Opt opt) {
    Argp_Flag *flag = argp_new_flag(ARGP_UINT, short_name, long_name,
                                    opt.meta_var, opt.desc, (Argp_Command *)opt.command);
    flag->val.as_uint = def;
    flag->def.as_uint = def;
    return &flag->val.as_uint;
}

char **argp_flag_str_(const char *short_name, const char *long_name, char *def, Argp_Flag_Opt opt) {
    Argp_Flag *flag = argp_new_flag(ARGP_STR, short_name, long_name,
                                    opt.meta_var, opt.desc, (Argp_Command *)opt.command);
    flag->val.as_str = def;
    flag->def.as_str = def;
    return &flag->val.as_str;
}

size_t *argp_flag_enum_(const char *short_name, const char *long_name, const char *options[],
                        size_t option_count, size_t def, Argp_Flag_Opt opt) {
    Argp_Flag *flag = argp_new_flag(ARGP_ENUM, short_name, long_name, NULL, opt.desc,
                                    (Argp_Command *)opt.command);
    flag->val.as_enum = def;
    flag->def.as_enum = def;
    flag->enum_options = options;
    flag->option_count = option_count;
    return &flag->val.as_enum;
}

Argp_List *argp_flag_list_(const char *short_name, const char *long_name, Argp_Flag_Opt opt) {
    Argp_Flag *flag = argp_new_flag(ARGP_LIST, short_name, long_name,
                                    opt.meta_var, opt.desc, (Argp_Command *)opt.command);
    flag->val.as_list = (Argp_List){0};
    flag->def.as_list = (Argp_List){0};
    return &flag->val.as_list;
}

uint64_t *argp_pos_uint_(const char *name, uint64_t def, Argp_Pos_Opt opt) {
    Argp_Pos *pos = argp_new_pos(ARGP_UINT, name, opt.desc, opt.req,
                                 (Argp_Command *)opt.command);
    pos->val.as_uint = def;
    pos->def.as_uint = def;
    return &pos->val.as_uint;
}

char **argp_pos_str_(const char *name, char *def, Argp_Pos_Opt opt) {
    Argp_Pos *pos = argp_new_pos(ARGP_STR, name, opt.desc, opt.req,
                                 (Argp_Command *)opt.command);
    pos->val.as_str = def;
    pos->def.as_str = def;
    return &pos->val.as_str;
}

size_t *argp_pos_enum_(const char *name, const char *options[], size_t option_count,
                       size_t def, Argp_Pos_Opt opt) {
    Argp_Pos *pos = argp_new_pos(ARGP_ENUM, name, opt.desc, opt.req,
                                 (Argp_Command *)opt.command);
    pos->val.as_enum = def;
    pos->def.as_enum = def;
    pos->enum_options = options;
    pos->option_count = option_count;
    return &pos->val.as_enum;
}

Argp_List *argp_pos_list_(const char *name, Argp_Pos_Opt opt) {
    Argp_Pos *pos = argp_new_pos(ARGP_LIST, name, opt.desc, opt.req,
                                 (Argp_Command *)opt.command);
    pos->val.as_list = (Argp_List){0};
    pos->def.as_list = (Argp_List){0};
    return &pos->val.as_list;
}

void argp_print_usage(FILE *stream) {
    Argp_Ctx *c = &argp_global_ctx;

    size_t command_count = c->command_ctx->command_count;
    size_t pos_count = c->command_ctx->pos_count;
    size_t flag_count = c->command_ctx->flag_count;

    fprintf(stream, "usage:");
    print_full_command_name(stream, c->command_ctx);
    if (command_count)
        fprintf(stream, " [command]");
    if (flag_count)
        fprintf(stream, " [options]");

    for (size_t i = 0; i < c->pos_capacity; ++i) {
        const Argp_Pos *pos = c->poss + i;

        if (pos->command != c->command_ctx) continue;

        if (pos->req == ARGP_OPTIONAL) {
            if (pos->type == ARGP_LIST)
                fprintf(stream, " [%s...]", pos->name);
            else
                fprintf(stream, " [%s]", pos->name);
        } else {
            if (pos->type == ARGP_LIST)
                fprintf(stream, " %s [%s...]", pos->name, pos->name);
            else
                fprintf(stream, " %s", pos->name);
        }
    }
    fprintf(stream, "\n\n");

    if (c->command_ctx->desc) {
        fprintf(stream, "%s\n\n", c->command_ctx->desc);
    }

    if (command_count) {
        fprintf(stream, "commands:\n");
        for (size_t i = 0; i < c->command_capacity; ++i) {
            const Argp_Command *command = c->commands + i;

            if (command->parent_command != c->command_ctx) continue;

            int width = 0;
            width += fprintf(stream, "  %s", command->name);

            if (width >= ARGP_PRINT_WIDTH) {
                fprintf(stream, "\n");
                width = 0;
            }

            int n = ARGP_PRINT_WIDTH - width + (int)strlen(command->desc);
            fprintf(stream, "%*s\n", n, command->desc);
        }
        fprintf(stream, "\n");
    }

    if (pos_count) {
        fprintf(stream, "positional arguments:\n");
        for (size_t i = 0; i < c->pos_capacity; ++i) {
            const Argp_Pos *pos = c->poss + i;

            if (pos->command != c->command_ctx) continue;

            int width = 0;
            width += fprintf(stream, "  %s", pos->name);

            if (pos->type == ARGP_ENUM) {
                width += fprintf(stream, " {");
                for (size_t i = 0; i < pos->option_count; ++i) {
                    const char *option = pos->enum_options[i];
                    if (!option) continue;
                    width += fprintf(stream, "%s", option);
                    if (i < pos->option_count - 1)
                        width += fprintf(stream, ",");
                }
                width += fprintf(stream, "}");
            }

            if (width >= ARGP_PRINT_WIDTH) {
                fprintf(stream, "\n");
                width = 0;
            }

            int n = ARGP_PRINT_WIDTH - width + (int)strlen(pos->desc);
            fprintf(stream, "%*s\n", n, pos->desc);
        }
        fprintf(stream, "\n");
    }

    if (flag_count) {
        fprintf(stream, "options:\n");
        for (size_t i = 0; i < c->flag_capacity; ++i) {
            const Argp_Flag *flag = c->flags + i;

            if (flag->command != c->command_ctx) continue;

            int width = 0;
            if (flag->short_name && flag->long_name) {
                width += fprintf(stream, "  -%s, --%s", flag->short_name, flag->long_name);
            } else if (flag->short_name) {
                width += fprintf(stream, "  -%s", flag->short_name);
            } else if (flag->long_name) {
                width += fprintf(stream, "  --%s", flag->long_name);
            } else {
                assert(false && "At least one name of a flag must be non null");
            }

            if (flag->meta_var) {
                width += fprintf(stream, " %s", flag->meta_var);
            } else if (flag->type == ARGP_ENUM) {
                width += fprintf(stream, " {");
                for (size_t i = 0; i < flag->option_count; ++i) {
                    const char *option = flag->enum_options[i];
                    if (!option) continue;
                    width += fprintf(stream, "%s", option);
                    if (i < flag->option_count - 1)
                        width += fprintf(stream, ",");
                }
                width += fprintf(stream, "}");
            }

            if (width >= ARGP_PRINT_WIDTH) {
                fprintf(stream, "\n");
                width = 0;
            }

            int n = ARGP_PRINT_WIDTH - width + (int)strlen(flag->desc);
            fprintf(stream, "%*s\n", n, flag->desc);
        }
    }
}

void argp_print_error(FILE *stream) {
    Argp_Ctx *c = &argp_global_ctx;
    switch (c->err) {
        case ARGP_NO_ERROR: {
            fprintf(stream, "No errors parsing arguments\n");
            return;
        } break;
        case ARGP_ERROR_UNKNOWN: {
            fprintf(stream, "Error: Unknown option %s\n", c->unknown_option);
            return;
        } break;
        case ARGP_ERROR_UNKNOWN_ENUM: {
            fprintf(stream, "Error: Unknown enum option");
        } break;
        case ARGP_ERROR_NO_VALUE: {
            fprintf(stream, "Error: No value provided");
        } break;
        case ARGP_ERROR_INVALID_NUMBER: {
            fprintf(stream, "Error: Invalid number");
        } break;
        case ARGP_ERROR_INTEGER_OVERFLOW: {
            fprintf(stream, "Error: Integer overflow");
        } break;
        case ARGP_ERROR_ALLOC: {
            fprintf(stream, "Error: Allocating");
        } break;
        case ARGP_ERROR_COUNT:
            ARGP_ASSERT(false && "Unreachable");
    }

    Argp_Type type;
    const char **enum_option;
    size_t option_count;
    if (c->err_flag) {
        const Argp_Flag *flag = c->err_flag;
        if (flag->long_name)
            fprintf(stream, " for flag --%s", flag->long_name);
        else
            fprintf(stream, " for flag -%s", flag->short_name);

        type = c->err_flag->type;
        enum_option = c->err_flag->enum_options;
        option_count = c->err_flag->option_count;
    } else {
        fprintf(stream, " for positional argument %s", c->err_pos->name);

        type = c->err_pos->type;
        enum_option = c->err_pos->enum_options;
        option_count = c->err_pos->option_count;
    }

    if (c->unknown_option)
        fprintf(stream, " got '%s'", c->unknown_option);

    if (type == ARGP_ENUM) {
        fprintf(stream, " expected {");
        for (size_t i = 0; i < option_count; ++i) {
            const char *option = enum_option[i];
            fprintf(stream, "%s", option);

            if (i < option_count - 1)
                fprintf(stream, ",");
        }
        fprintf(stream, "}");
    }

    fprintf(stream, "\n");
}

static Argp_Flag *try_short_name(const char *arg, size_t n) {
    Argp_Ctx *c = &argp_global_ctx;
    if (n < 1) return NULL;
    if (arg[0] != '-') return NULL;
    const char *short_name = arg + 1;

    for (size_t i = 0; i < c->flag_capacity; ++i) {
        Argp_Flag *flag = c->flags + i;
        if (flag->command != c->command_ctx || !flag->short_name) continue;
        if (strcmp(short_name, flag->short_name) == 0)
            return flag;
    }

    return NULL;
}

static Argp_Flag *try_long_name(const char *arg, size_t n) {
    Argp_Ctx *c = &argp_global_ctx;
    if (n < 2) return NULL;
    if (arg[0] != '-' || arg[1] != '-')
        return NULL;
    const char *long_name = arg + 2;

    for (size_t i = 0; i < c->flag_capacity; ++i) {
        Argp_Flag *flag = c->flags + i;
        if (flag->command != c->command_ctx || flag->long_name) continue;
        if (strcmp(long_name, flag->long_name) == 0)
            return flag;
    }

    return NULL;
}

static bool argp_parse_uint(char *arg, uint64_t *v) {
    Argp_Ctx *c = &argp_global_ctx;

    if (!arg) {
        c->err = ARGP_ERROR_NO_VALUE;
        return false;
    }
    char *endptr;
    uint64_t result = strtoull(arg, &endptr, 10);

    if (*endptr != 0) {
        c->err = ARGP_ERROR_INVALID_NUMBER;
        c->unknown_option = arg;
        return false;
    }
    if (result == ULLONG_MAX && errno == ERANGE) {
        c->err = ARGP_ERROR_INTEGER_OVERFLOW;
        c->unknown_option = arg;
        return false;
    }
    *v = result;
    return true;
}

static bool argp_parse_str(char *arg, char **v) {
    Argp_Ctx *c = &argp_global_ctx;

    if (!arg) {
        c->err = ARGP_ERROR_NO_VALUE;
        return false;
    }
    *v = arg;
    return true;
}

static bool argp_parse_enum(char *arg, size_t *v, const char **enum_options, size_t option_count) {
    Argp_Ctx *c = &argp_global_ctx;

    if (!arg) {
        c->err = ARGP_ERROR_NO_VALUE;
        return false;
    }

    for (size_t i = 0; i < option_count; ++i) {
        const char *option = enum_options[i];
        if (!option) continue;
        if (strcmp(arg, option) == 0) {
            *v = i;
            return true;
        }
    }

    c->err = ARGP_ERROR_UNKNOWN_ENUM;
    c->unknown_option = arg;
    return false;
}

static bool argp_parse_list_entry(char *arg, Argp_List *list) {
    if (list->_cap == 0 || list->size == list->_cap) {
        list->_cap = list->_cap ? list->_cap << 1 : ARGP_LIST_INIT_CAP;

        char **temp = list->items;
        list->items = (char **)ARGP_REALLOC(NULL, list->_cap * sizeof(char *));
        if (list->items == NULL) {
            ARGP_FREE(temp);
            return false;
        }
    }

    list->items[list->size++] = arg;
    return true;
}

static bool argp_parse_flag(Argp_Flag *flag) {
    Argp_Ctx *c = &argp_global_ctx;
    switch (flag->type) {
        case ARGP_BOOL: {
            flag->val.as_bool = true;
        } break;
        case ARGP_UINT: {
            char *arg = shift_args();
            if (!argp_parse_uint(arg, &flag->val.as_uint)) {
                c->err_flag = flag;
                return false;
            }
        } break;
        case ARGP_STR: {
            char *arg = shift_args();
            if (!argp_parse_str(arg, &flag->val.as_str)) {
                c->err_flag = flag;
                return false;
            }
        } break;
        case ARGP_ENUM: {
            char *arg = shift_args();
            if (!argp_parse_enum(arg, &flag->val.as_enum, flag->enum_options, flag->option_count)) {
                c->err_flag = flag;
                return false;
            }
        } break;
        case ARGP_LIST: {
            char *arg = shift_args();
            if (!argp_parse_list_entry(arg, &flag->val.as_list)) {
                c->err_flag = flag;
                return false;
            }
        } break;
        default:
            ARGP_ASSERT(false && "Unreachable");
    }
    return true;
}

static bool argp_parse_pos(char *arg, Argp_Pos *pos) {
    Argp_Ctx *c = &argp_global_ctx;
    switch (pos->type) {
        case ARGP_UINT: {
            if (!argp_parse_uint(arg, &pos->val.as_uint)) {
                c->err_pos = pos;
                return false;
            }
        } break;
        case ARGP_STR: {
            if (!argp_parse_str(arg, &pos->val.as_str)) {
                c->err_pos = pos;
                return false;
            }
        } break;
        case ARGP_ENUM: {
            if (!argp_parse_enum(arg, &pos->val.as_enum, pos->enum_options, pos->option_count)) {
                c->err_pos = pos;
                return false;
            }
        } break;
        case ARGP_LIST: {
            if (!argp_parse_list_entry(arg, &pos->val.as_list)) {
                c->err_pos = pos;
                return false;
            }
        } break;
        default:
            ARGP_ASSERT(false && "Unreachable");
    }
    return true;
}

bool argp_parse_args(void) {
    Argp_Ctx *c = &argp_global_ctx;

    char *arg;
    while ((arg = shift_args())) {
        size_t n = strlen(arg);

        Argp_Flag *flag = NULL;
        if (!(flag = try_short_name(arg, n)))
            flag = try_long_name(arg, n);

        if (flag) {
            if (flag == c->command_ctx->help_flag) {
                argp_print_usage(stdout);
                exit(0);
            }
            if (!argp_parse_flag(flag))
                return false;
            continue;
        }

        Argp_Command *selected_command = NULL;

        for (size_t i = 0; i < c->command_capacity; ++i) {
            Argp_Command *command = c->commands + i;
            if (command->parent_command == c->command_ctx &&
                strcmp(arg, command->name) == 0) {
                selected_command = command;
                break;
            }
        }

        if (selected_command) {
            selected_command->val = true;
            c->command_ctx = selected_command;
            continue;
        }

        Argp_Pos *selected_pos = NULL;

        for (size_t i = 0; i < c->pos_capacity; ++i) {
            Argp_Pos *pos = c->poss + i;
            if (pos->command == c->command_ctx &&
                (pos->type == ARGP_LIST || !pos->seen)) {
                selected_pos = pos;
                break;
            }
        }

        if (selected_pos == NULL) {
            c->err = ARGP_ERROR_UNKNOWN;
            c->unknown_option = arg;
            return false;
        }

        if (!argp_parse_pos(arg, selected_pos))
            return false;

        selected_pos->seen = true;
    }

    for (size_t i = 0; i < c->pos_capacity; ++i) {
        Argp_Pos *pos = c->poss + i;
        if (pos->command != c->command_ctx) continue;
        if (pos->req == ARGP_REQUIRED && !pos->seen) {
            c->err = ARGP_ERROR_NO_VALUE;
            c->err_pos = pos;
            return false;
        }
    }

    return true;
}

void argp_free_list(Argp_List *list) { ARGP_FREE(list->items); }

const char *argp_name(void *val) {
    Argp_Ctx *c = &argp_global_ctx;
    for (size_t i = 0; i < ARGP_FLAG_CAP; ++i) {
        const Argp_Flag *flag = c->flags + i;
        if (&flag->val == val) {
            if (flag->long_name)
                return flag->long_name;
            else
                return flag->short_name;
        }
    }
    for (size_t i = 0; i < ARGP_POS_CAP; ++i) {
        const Argp_Pos *pos = c->poss + i;
        if (&pos->val == val)
            return pos->name;
    }
    return NULL;
}

#endif  // ARGPARSE_IMPLEMENTATION

// Copyright 2025 Macsen Casaus <macsencasaus@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is furnished to do
// so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
