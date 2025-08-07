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
#include <stdbool.h>
#include <stddef.h>
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
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
    FMT_TEXT = 3,
} FileFormat;

typedef struct {
    a_string in_path;
    a_string out_path;
    a_string var_name;  // appvar
    a_string file_name; // appvar
    FileFormat in_fmt;
    FileFormat out_fmt;
    bool verbose;
    bool help;
    bool license;
} Args;

static const struct option LONG_OPTS[] = {
    {"outfile", required_argument, 0, 'o'},
    {"input-format", required_argument, 0, 'f'},
    {"target-format", required_argument, 0, 't'},
    {"varname", required_argument, 0, 'N'},
    {"filename", required_argument, 0, 'F'},
    {"verbose", no_argument, 0, 'v'},
    {"help", no_argument, 0, 'h'},
    {"license", no_argument, 0, 'l'},
    {0},
};
static Args args;

// === function decls ===

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

Args args_new(void) {
    return (Args){
        .in_path = a_string_with_capacity(25),
        .out_path = a_string_with_capacity(25),
        .var_name = a_string_with_capacity(25),
        .file_name = a_string_with_capacity(25),
    };
}

void args_deinit(Args* args) {
    a_string_free(&args->in_path);
    a_string_free(&args->out_path);
    a_string_free(&args->var_name);
    a_string_free(&args->file_name);
}

FileFormat get_format_from_string(const char* ext) {
    if (ext == NULL)
        return FMT_INVALID;

    if (!strcasecmp(ext, "py") || !strcasecmp(ext, "python"))
        return FMT_PY;
    else if (!strcasecmp(ext, "8xv") || !strcasecmp(ext, "appvar"))
        return FMT_APPVAR;
    else if (!strcasecmp(ext, "txt") || !strcasecmp(ext, "text"))
        return FMT_TEXT;

    return FMT_INVALID;
}

FileFormat get_format_from_path(const char* path) {
    const char* ext = get_file_extension(path);
    return get_format_from_string(ext);
}

void help(void) {
    puts(HELP);
    exit(0);
}

void license(void) {
    puts(LICENSE);
    exit(0);
}

bool parse_args(int argc, char** argv) {
    args = args_new();

    int c;
    while ((c = getopt_long(argc, argv, "o:f:N:F:t:Vvhl", LONG_OPTS, NULL)) !=
           -1) {
        switch (c) {
            case 'o': {
                a_string_copy_cstr(&args.out_path, optarg);
            } break;
            case 'f': {
                args.in_fmt = get_format_from_string(optarg);
            } break;
            case 'N': {
                a_string_copy_cstr(&args.var_name, optarg);
            } break;
            case 'F': {
                a_string_copy_cstr(&args.file_name, optarg);
            } break;
            case 't': {
                args.out_fmt = get_format_from_string(optarg);
            } break;
            case 'V': {
                printf("version " VERSION "\n");
                exit(0);
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

// guess the output file format based on the input format
FileFormat guess_output_file_format(FileFormat in_fmt) {
    FileFormat out_fmt = get_format_from_path(args.out_path.data);
    if (out_fmt != FMT_INVALID)
        return out_fmt;

    if (in_fmt == FMT_PY || in_fmt == FMT_TEXT)
        return FMT_APPVAR;

    if (in_fmt == FMT_APPVAR && (out_fmt == FMT_PY || out_fmt == FMT_TEXT)) {
        fatal("could not infer the output file type! please specify an output "
              "file type or a file path.");
    }

    return out_fmt;
}

// guesses the output path of a Ti_PyFile
a_string guess_python_file_path(const Ti_PyFile* pyfile) {
    if (args.out_path.len != 0)
        return a_string_dupe(&args.out_path);

    a_string res = astr("./");

    if (pyfile->file_name) {
        a_string_append(&res, pyfile->file_name);
        return res;
    }

    if (strlen(pyfile->var_name) == 0)
        a_string_append(&res, "PYTHON01");
    else
        a_string_append(&res, pyfile->var_name);
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

bool convert_appvar(const a_string* in_file, FileFormat out_fmt) {
    if (out_fmt == FMT_TEXT) {
        not_implemented;
    } else {
        Ti_ParseResult res = {0};
        Ti_PyFile pyfile = ti_pyfile_parse(in_file->data, in_file->len, &res);

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

        ti_pyfile_free(&pyfile);
        a_string_free(&out_path);
    }

    return true;
}

bool convert_text(const a_string* in_file, FileFormat out_fmt) {
    not_implemented;
}

bool convert_py(const a_string* in_file, FileFormat out_fmt) {
    const char* file_name = basename(args.in_path.data);
    char* finfo = NULL; // TODO: implement
    char* var_name;
    if (args.var_name.len == 0)
        var_name = get_var_name_from_path(args.in_path.data);
    else
        var_name = strdup(args.var_name.data);

    Ti_PyFile pyfile =
        ti_pyfile_new_with_metadata_full(in_file->data, in_file->len, file_name,
                                         strlen(file_name), finfo, var_name);

    char* buf = NULL;
    usize len = ti_pyfile_dump(&pyfile, &buf);
    if (len == -1) {
        free(var_name);
        ti_pyfile_free(&pyfile);
        error("failed to dump new AppVar to binary format");
        return false;
    }

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

    free(buf);
    fclose(fp);
    free(var_name);
    ti_pyfile_free(&pyfile);
    a_string_free(&out_path);

    return true;
}

bool convert(FileFormat in_fmt, FileFormat out_fmt) {
    a_string in_file = a_string_read_file(args.in_path.data);
    if (!a_string_valid(&in_file)) {
        error("failed to read input file: \"%s\"",
              strerror(errno)) return false;
    }

    switch (in_fmt) {
        case FMT_APPVAR: {
            if (!convert_appvar(&in_file, out_fmt))
                return false;
        } break;
        case FMT_TEXT: {
            if (out_fmt == FMT_PY)
                error("will not convert from a Python file to a text file!");

            if (!convert_text(&in_file, out_fmt))
                return false;
        } break;
        case FMT_PY: {
            if (out_fmt == FMT_TEXT)
                error("will not convert from a Python file to a text file!");

            if (!convert_py(&in_file, out_fmt))
                return false;
        } break;
        case FMT_INVALID: {
            panic("unreachable");
        } break;
    }

    a_string_free(&in_file);
    return true;
}

int main(int argc, char** argv) {
    if (!parse_args(argc, argv))
        return EXIT_FAILURE;

    FileFormat in_fmt = args.in_fmt;
    if (args.in_fmt == FMT_INVALID)
        in_fmt = get_format_from_path(args.in_path.data);

    FileFormat out_fmt = args.out_fmt;
    if (args.out_fmt == FMT_INVALID)
        out_fmt = guess_output_file_format(in_fmt);

    if (in_fmt == FMT_INVALID)
        fatal("unrecognized input file format");

    if (out_fmt == FMT_INVALID)
        fatal("unrecognized output file format");

    if (in_fmt == out_fmt) {
        warn("input and output formats are the same, no conversion done");
        return EXIT_FAILURE;
    }

    if (!convert(in_fmt, out_fmt)) {
        fatal("error occurred during conversion!");
    }

    args_deinit(&args);
    return EXIT_SUCCESS;
}
