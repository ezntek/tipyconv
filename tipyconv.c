/*
 * tipyconv: Tools to convert to and from the TI Python file format.
 *
 * Copyright (c) Eason Qin (eason@ezntek.com), 2025.
 *
 * This source code form is licensed under the BSD 3-clause License. You may
 * find the full text of the license in the root of the project directory at
 * `LICENSE.md`. Alternatively, find an online copy at
 * https://spdx.org/licenses/BSD-3-Clause.html.
 */
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "3rdparty/asv/a_string.h"
#include "3rdparty/include/a_string.h"
#include "common.h"

#define _TIPYCONV_IMPLEMENTATION
#include "tipyconv.h"

#define info(...)                                                              \
    {                                                                          \
        if (args.verbose)                                                      \
            log_info(__VA_ARGS__);                                             \
    }
#define warn(...)                                                              \
    {                                                                          \
        log_warn(__VA_ARGS__);                                                 \
    }
#define error(...)                                                             \
    {                                                                          \
        log_error(__VA_ARGS__);                                                \
    }
#define fatal(...)                                                             \
    {                                                                          \
        log_fatal(__VA_ARGS__);                                                \
    }

typedef enum {
    FMT_INVALID = 0,
    FMT_APPVAR = 1,
    FMT_PY = 2,
} Format;

typedef struct {
    a_string in_path;
    a_string out_path;
    a_string var_name; // appvar
    bool verbose;
    bool help;
    bool license;
} Args;

static const struct option LONG_OPTS[] = {
    {"outfile", required_argument, 0, 'o'},
    {"varname", required_argument, 0, 'N'},
    {"version", required_argument, 0, 'V'},
    {"verbose", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {"license", no_argument, 0, 'l'},
    {0},
};
static Args args;

// === function decls ===
char* get_file_name(const char* src);
char* get_file_extension(const char* src);
bool file_exists(const char* path);
Format get_format_from_string(const char* ext);
Format get_format_from_path(const char* path);

Args args_new(void);
void args_deinit(Args* args);
void version(void);
void help(void);
void license(void);
bool parse_args(int argc, char** argv);

Format get_output_format(Format in_fmt);
char* get_python_file_path(const Ti_PyFile* pyfile);
char* get_var_name_from_path(const char* path);
bool convert_appvar(const a_string* in_file);
bool convert_py(const a_string* in_file);
bool convert(Format in_fmt);

// returns a heap allocated char*
char* get_file_name(const char* src) {
    const char* base = basename(src);
    const char* dot = strrchr(base, '.');
    ptrdiff_t diff = dot - base;
    char* res = calloc(diff + 1, 1);
    check_alloc(res);
    strncpy(res, base, diff);
    return res;
}

char* get_file_extension(const char* src) {
    const char* base = basename(src);
    char* ext;
    const char* dot = strrchr(base, '.');
    if (!dot || dot == base)
        ext = NULL;
    else
        ext = (char*)dot + 1;
    return ext;
}

bool file_exists(const char* path) {
    FILE* fp = fopen(path, "r");
    if (!fp) {
        return false;
    }
    fclose(fp);
    return true;
}

Format get_format_from_string(const char* ext) {
    if (ext == NULL)
        return FMT_INVALID;

    if (!strcasecmp(ext, "py") || !strcasecmp(ext, "python"))
        return FMT_PY;
    else if (!strcasecmp(ext, "8xv") || !strcasecmp(ext, "appvar"))
        return FMT_APPVAR;

    return FMT_INVALID;
}

Format get_format_from_path(const char* path) {
    const char* ext = get_file_extension(path);
    return get_format_from_string(ext);
}

Args args_new(void) {
    return (Args){
        .in_path = a_string_with_capacity(25),
        .out_path = a_string_with_capacity(25),
        .var_name = a_string_with_capacity(25),
    };
}

void args_deinit(Args* args) {
    a_string_free(&args->in_path);
    a_string_free(&args->out_path);
    a_string_free(&args->var_name);
}

void version(void) {
    puts(S_BOLD VERSION_TXT S_BOLD);
    exit(EXIT_SUCCESS);
}

void help(void) {
    puts(HELP);
    exit(EXIT_SUCCESS);
}

void license(void) {
    puts(LICENSE);
    exit(EXIT_SUCCESS);
}

bool parse_args(int argc, char** argv) {
    args = args_new();

    int c;
    while ((c = getopt_long(argc, argv, "o:N:Vvhl", LONG_OPTS, NULL)) != -1) {
        switch (c) {
            case 'o': {
                a_string_copy_cstr(&args.out_path, optarg);
            } break;
            case 'N': {
                a_string_copy_cstr(&args.var_name, optarg);
            } break;
            case 'V': {
                version();
            } break;
            case 'v': {
                args.verbose = true;
            } break;
            case 'h': {
                help();
            } break;
            case 'l': {
                license();
            } break;
            case '?': {
                help();
                return true;
            } break;
        }
    }

    // positional arg: input file
    if (optind >= argc) {
        error("must supply input file as positional argument!");
        help();
        return false;
    }
    a_string_copy_cstr(&args.in_path, argv[optind]);

    if (args.in_path.len == 0)
        fatal("no input file provided");

    return true;
}

Format get_output_format(Format in_fmt) {
    Format out_fmt = get_format_from_path(args.out_path.data);
    if (out_fmt != FMT_INVALID)
        return out_fmt;

    if (in_fmt == FMT_PY) {
        return FMT_APPVAR;
    } else if (in_fmt == FMT_APPVAR) {
        return FMT_PY;
    } else {
        fatal("could not infer output file format!");
    }
}

a_string guess_python_file_path(const Ti_PyFile* pyfile) {
    if (args.out_path.len != 0)
        return a_string_dupe(&args.out_path);

    a_string res = astr("./");

    if (pyfile->file_name) {
        a_string_append(&res, pyfile->file_name);
        return res;
    }

    if (strlen(pyfile->var_name) == 0) {
        char* var_name = get_var_name_from_path(args.in_path.data);
        a_string_append(&res, var_name);
        free(var_name);
    } else {
        a_string_append(&res, pyfile->var_name);
    }

    a_string_append(&res, ".py");

    return res;
}

char* get_var_name_from_path(const char* path) {
    char* res = calloc(9, 1);
    check_alloc(res);
    char* name = get_file_name(path);
    for (usize i = 0; i < 8; i++) {
        if (name[i] == '\0')
            break;

        res[i] = toupper(name[i]);
    }
    free(name);
    return res;
}

// guesses the output path of an AppVar
a_string guess_appvar_path(const Ti_PyFile* pyfile) {
    if (args.out_path.len != 0)
        return a_string_dupe(&args.out_path);

    a_string res = astr("./");
    if (strlen(pyfile->var_name) > 0) {
        a_string_append(&res, pyfile->var_name);
    } else {
        warn("AppVar does not have a variable name!");
        char* var_name = get_var_name_from_path(args.in_path.data);
        a_string_append(&res, var_name);
        free(var_name);
    }
    a_string_append(&res, ".8xv");

    return res;
}

bool convert_appvar(const a_string* in_file) {
    Ti_ParseResult res = {0};
    Ti_PyFile pyfile = ti_pyfile_parse(in_file->data, &res);

    switch (res) {
        case TI_PARSE_OK: {
            info("successfully parsed");
        } break;
        case TI_PARSE_ERROR: {
            fatal("failed to parse AppVar!");
        } break;
        case TI_INVALID_FORMAT: {
            fatal("AppVar has an incorrect file format!");
        } break;
        case TI_CHECKSUM_INCORRECT: {
            fatal("AppVar checksum verification failed");
        } break;
    }

    a_string out_path = guess_python_file_path(&pyfile);

    if (file_exists(out_path.data))
        warn("file %s already exists on disk, overwriting", out_path.data);

    FILE* out_fp = fopen(out_path.data, "w");
    if (!out_fp)
        fatal("could not open output path for writing: \"%s\"",
              strerror(errno));

    usize bytes_written = fwrite(pyfile.src, 1, pyfile.src_len, out_fp);
    if (bytes_written < pyfile.src_len)
        fatal("short write-out on Python file at \"%s\"", out_path.data);
    info("file written to \"%s\"", out_path.data);

    ti_pyfile_free(&pyfile);
    a_string_free(&out_path);
    return true;
}

bool convert_py(const a_string* in_file) {
    char* var_name;
    if (args.var_name.len == 0)
        var_name = get_var_name_from_path(args.in_path.data);
    else
        var_name = strdup(args.var_name.data);

    Ti_PyFile pyfile = ti_pyfile_new_with_metadata_full(
        in_file->data, in_file->len, NULL, 0, NULL, var_name);

    char* buf = NULL;
    usize len = ti_pyfile_dump(&pyfile, &buf);

    a_string out_path = guess_appvar_path(&pyfile);
    if (file_exists(out_path.data))
        warn("AppVar at path \"%s\" already exists, overwriting",
             out_path.data);

    FILE* fp = fopen(out_path.data, "w");
    if (!fp)
        fatal("could not open AppVar for writing: \"%s\"", strerror(errno));

    usize bytes_written = fwrite(buf, 1, len, fp);
    if (bytes_written < len)
        panic("short write-out on AppVar!");
    info("file written to \"%s\"", out_path.data);

    free(buf);
    fclose(fp);
    free(var_name);
    ti_pyfile_free(&pyfile);
    a_string_free(&out_path);

    return true;
}

bool convert(Format in_fmt) {
    a_string in_file = a_string_read_file(args.in_path.data);
    if (!a_string_valid(&in_file)) {
        error("failed to read input file: \"%s\"",
              strerror(errno)) return false;
    }
    info("loaded file \"%s\"", args.in_path.data);

    switch (in_fmt) {
        case FMT_APPVAR: {
            info("converting from AppVar to Python");
            if (!convert_appvar(&in_file))
                return false;
        } break;
        case FMT_PY: {
            info("converting from Python to AppVar");
            if (!convert_py(&in_file))
                return false;
        } break;
        default:
            break;
    }

    a_string_free(&in_file);
    return true;
}

int main(int argc, char** argv) {
    if (!parse_args(argc, argv))
        return EXIT_FAILURE;

    info(VERSION_TXT);

    Format in_fmt = get_format_from_path(args.in_path.data);
    Format out_fmt = get_output_format(in_fmt);

    if (in_fmt == FMT_INVALID)
        fatal("unknown input file format");

    if (out_fmt == FMT_INVALID)
        fatal("unknown output file format");

    if (in_fmt == out_fmt) {
        warn("input and output formats are the same, no conversion done");
        return EXIT_FAILURE;
    }

    if (!convert(in_fmt)) {
        fatal("error occurred during conversion!");
    }

    args_deinit(&args);
    return EXIT_SUCCESS;
}
