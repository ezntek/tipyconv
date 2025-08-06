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

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
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
        if (args.verbose)                                                      \
            log_info(__VA_ARGS__);                                             \
    }
#define error(...)                                                             \
    {                                                                          \
        log_info(__VA_ARGS__);                                                 \
    }
#define fatal(...)                                                             \
    {                                                                          \
        log_fatal(__VA_ARGS__);                                                \
    }

void disasm_bytes(const char* title, const char* data, usize len) {
    printf("\033[1m%s: \033[0m", title);
    for (usize i = 0; i < len; i++) {
        unsigned char byte = data[i];
        printf("%02x", byte);
        // printf("%02d", (char)data[i]);
        if (i % 2 == 1)
            putchar(' ');
    }
    putchar('\n');
}

void disasm_string(const char* title, const char* data, usize len) {
    printf("\033[1m%s: \033[0m\"%.*s\"\n", title, (int)len, data);
}

u16 disasm_num(const char* title, const char* data) {
    u16 res = (u16)((u8)(data[0]) | (u8)(data[1] << 8));
    printf("\033[1m%s: \033[0m%d\n", title, res);
    return res;
}

void disasm(const char* data, usize len) {
    // header
    disasm_bytes("hdr", &data[0], 11);
    disasm_string("finfo", &data[0xB], 42);
    disasm_num("dsize", &data[0x35]);
    disasm_bytes("psize", &data[0x39], 2);
    disasm_num("psize", &data[0x39]);
    disasm_bytes("vid", &data[0x3B], 1);
    disasm_string("vname", &data[0x3C], 8);
    disasm_num("psize", &data[0x46]);
    u16 plen = disasm_num("plen", &data[0x48]);
    disasm_string("pyfmt", &data[0x4A], 4);
    plen -= 5; // skip past PYCD and \0 because it will always be present

    usize pstart = 0x4F; // start of payload
    // there might be a filename here used to transfer stuff back on disk
    if (data[0x4E] != '\0') {
        // contains null terminator, likely a full buffer length
        u8 filename_len = data[0x4E];
        // 0x4F should be SOH
        disasm_string("fname", &data[0x50],
                      filename_len); // should stop at nullterm anyway
        pstart = 0x50 + filename_len;
        plen -= 1 + filename_len; // SOH + filename length (incl \0)
    }

    disasm_string("payload", &data[pstart], plen);
    const size_t after_payload = pstart + plen;
    disasm_bytes("checksum", &data[after_payload], len - after_payload);
}

typedef enum {
    FMT_INVALID = 0,
    FMT_APPVAR = 1,
    FMT_PY = 2,
    FMT_TEXT = 3,
} FileFormat;

typedef struct {
    a_string infile;
    a_string outfile;
    a_string varname;
    a_string filename;
    FileFormat format;
    FileFormat target_format;
    bool verbose;
    bool help;
    bool license;
} Args;

static bool parse_args(int argc, char** argv);
static void license(void);
static void help(void);
static void deinit(void);
static usize appvar_to_py(const a_string* in, char** out);
static usize py_to_appvar(const a_string* in, char** out);
static usize appvar_to_txt(const a_string* in, char** out);
static usize txt_to_appvar(const a_string* in, char** out);

static Args args_new(void);
static void args_deinit(Args* args);

static char* get_file_extension(const char* src);
static FileFormat get_format_from_string(const char* ext);
static void check_args(void);

static const struct option LONG_OPTS[] = {
    {"outfile", required_argument, 0, 'o'},
    {"format", required_argument, 0, 'f'},
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

static Args args_new(void) {
    return (Args){
        .infile = a_string_with_capacity(25),
        .outfile = a_string_with_capacity(25),
        .varname = a_string_with_capacity(25),
        .filename = a_string_with_capacity(25),
    };
}

static void args_deinit(Args* args) {
    a_string_free(&args->infile);
    a_string_free(&args->outfile);
    a_string_free(&args->varname);
    a_string_free(&args->filename);
}

bool parse_args(int argc, char** argv) {
    args = args_new();

    int c;
    while ((c = getopt_long(argc, argv, "o:f:N:F:t:vhl", LONG_OPTS, NULL)) !=
           -1) {
        switch (c) {
            case 'o': {
                a_string_copy_cstr(&args.outfile, optarg);
            } break;
            case 'f': {
                args.format = get_format_from_string(optarg);
            } break;
            case 'N': {
                a_string_copy_cstr(&args.varname, optarg);
            } break;
            case 'F': {
                a_string_copy_cstr(&args.filename, optarg);
            } break;
            case 't': {
                args.target_format = get_format_from_string(optarg);
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
        return true;
    }
    a_string_copy_cstr(&args.infile, argv[optind]);

    return false;
}

static void help(void) {
    puts(HELP);
    exit(0);
}

static void license(void) {
    puts(LICENSE);
    exit(0);
}

static void deinit(void) { args_deinit(&args); }

static FileFormat get_format_from_string(const char* ext) {
    if (ext == NULL)
        return FMT_INVALID;

    if (!strcasecmp(ext, "py"))
        return FMT_PY;
    else if (!strcasecmp(ext, "8xv"))
        return FMT_APPVAR;
    else if (!strcasecmp(ext, "appvar"))
        return FMT_APPVAR;
    else if (!strcasecmp(ext, "txt"))
        return FMT_TEXT;

    return FMT_INVALID;
}

static FileFormat get_format_from_path(const char* path) {
    const char* ext = get_file_extension(path);
    return get_format_from_string(ext);
}

static usize appvar_to_py(const a_string* in, char** out) {
    Ti_ParseResult res = {0};
    Ti_PyFile pyfile = ti_pyfile_parse(in->data, in->len, &res);

    switch (res) {
        case TI_PARSE_OK: {
            info("successfully parsed");
        } break;
        case TI_PARSE_ERROR: {
            fatal("failed to parse");
        } break;
        case TI_INVALID_FORMAT: {
            fatal("invalid file format");
        } break;
        case TI_CHECKSUM_INCORRECT: {
            fatal("incorrect checksum");
        } break;
    }

    *out = strdup(pyfile.src);
    puts(*out);
    usize len = pyfile.src_len;
    ti_pyfile_free(&pyfile);
    return len;
}

static usize py_to_appvar(const a_string* in, char** out) { not_implemented; }

static usize appvar_to_txt(const a_string* in, char** out) { not_implemented; }

static usize txt_to_appvar(const a_string* in, char** out) { not_implemented; }

static char* get_file_extension(const char* src) {
    char* ext;
    const char* dot = strrchr(src, '.');
    if (!dot || dot == src)
        ext = NULL;
    else
        ext = (char*)dot + 1;
    return ext;
}

static void check_args(void) {
    if (args.infile.len == 0)
        fatal("no input file provided");

    if (args.outfile.len == 0)
        fatal("no output file provided");
}

int main(int argc, char** argv) {
    if (parse_args(argc, argv))
        return -1;

    check_args();

    FileFormat infmt = args.format;
    if (args.format == FMT_INVALID)
        infmt = get_format_from_path(args.infile.data);

    a_string infile_contents = a_string_read_file(args.infile.data);
    if (!a_string_valid(&infile_contents)) {
        deinit();
        fatal("failed to read input file: %s", strerror(errno));
    }

    if (infmt == FMT_INVALID)
        if (ti_is_appvar(infile_contents.data))
            infmt = FMT_APPVAR;

    // get the output file format
    FileFormat outfmt = args.target_format;
    if (args.target_format == FMT_INVALID)
        outfmt = get_format_from_path(args.outfile.data);

    if (infmt == FMT_INVALID)
        fatal("unrecognized input file format");

    if (outfmt == FMT_INVALID)
        fatal("unrecognized output file format");

    if (infmt == outfmt) {
        warn("input and output formats are the same, no conversion done");
        return 1;
    }

    char* out = NULL;
    usize out_len = 0;
    switch (infmt) {
        case FMT_PY: {
            out_len = py_to_appvar(&infile_contents, &out);
        } break;
        case FMT_TEXT: {
            out_len = txt_to_appvar(&infile_contents, &out);
        } break;
        case FMT_APPVAR: {
            if (outfmt == FMT_PY)
                out_len = appvar_to_py(&infile_contents, &out);
            else if (outfmt == FMT_TEXT)
                out_len = appvar_to_txt(&infile_contents, &out);
            else
                unreachable;
        } break;
        default:
            unreachable;
    }

    free(out);
    a_string_free(&infile_contents);
    deinit();
    return 0;
}
