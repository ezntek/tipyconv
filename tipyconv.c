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
#include <ctype.h>
#include <stdlib.h>
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
            log_warn(__VA_ARGS__);                                             \
    }
#define error(...)                                                             \
    {                                                                          \
        log_error(__VA_ARGS__);                                                \
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
static Ti_PyFile appvar_to_py(const a_string* in);
static usize py_to_appvar(const a_string* in, char** data);
static usize appvar_to_txt(const a_string* in);
static usize txt_to_appvar(const a_string* in);

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

    check_args();

    return false;
}

static void check_args(void) {
    if (args.infile.len == 0)
        fatal("no input file provided");
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

static Ti_PyFile appvar_to_py(const a_string* in) {
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

    return pyfile;
}

static usize py_to_appvar(const a_string* in, char** data) {
    char* file_name = NULL;
    usize file_name_len = 0;
    char var_name[8] = {0};

    if (args.filename.len != 0) {
        file_name = args.filename.data;
        file_name_len = args.filename.len;
    }

    if (args.varname.len != 0) {
        strncpy(var_name, args.varname.data, 8);
    } else {
        // FIXME: doesnt work on full paths
        ptrdiff_t diff = strrchr(args.infile.data, '.') - args.infile.data;
        if (diff > 8)
            diff = 8;
        for (size_t i = 0; i < diff; i++) {
            var_name[i] = toupper(args.infile.data[i]);
        }
    }

    // TODO: implement
    char* finfo = NULL;

    Ti_PyFile f = ti_pyfile_new_with_metadata_full(
        in->data, in->len, file_name, file_name_len, finfo, var_name);

    char* buf = NULL;
    usize len = ti_pyfile_dump(&f, &buf);
    if (len == -1)
        return -1;

    ti_pyfile_free(&f);
    *data = buf;
    return len;
}

static usize appvar_to_txt(const a_string* in) { not_implemented; }

static usize txt_to_appvar(const a_string* in) { not_implemented; }

static char* get_file_extension(const char* src) {
    char* ext;
    const char* dot = strrchr(src, '.');
    if (!dot || dot == src)
        ext = NULL;
    else
        ext = (char*)dot + 1;
    return ext;
}

static char* determine_output_path(void) {
    a_string res = a_string_with_capacity(20);

    if (args.outfile.len != 0) {
        a_string_append_astr(&res, &args.outfile);
    } else if (args.varname.len != 0) {
        char buf[9] = {0}; // FIXME: add fn in a_string
        strncpy(buf, args.varname.data, 8);
        // a_string_appendf?
        a_string_append_cstr(&res, "./");
        a_string_append_cstr(&res, buf);
        a_string_append_cstr(&res, ".8xv");
    } else {
        unreachable; // FIXME: remove
    }

    return res.data;
}

static bool write_appvar(const char* buf, usize buf_len, const char* path) {
    FILE* fp = fopen(path, "w");
    if (!fp)
        return false;

    usize bytes_written = fwrite(buf, 1, buf_len, fp);
    if (bytes_written != buf_len)
        return false;

    fclose(fp);
    return true;
}

static bool convert(const a_string* in, FileFormat infmt, FileFormat outfmt) {
    // disasm(in->data, in->len);

    switch (infmt) {
        case FMT_PY: {
            if (outfmt == FMT_TEXT) {
                warn("will not convert from a Python file to a text file!");
            } else {
                char* out = NULL;
                usize len = py_to_appvar(in, &out);
                if (len == -1)
                    return false;
                char* out_path = determine_output_path();
                if (out_path == NULL)
                    return false;
                if (!write_appvar(out, len, out_path)) {
                    free(out);
                    free(out_path);
                    return false;
                }
                free(out);
                free(out_path);
            }
        } break;
        case FMT_TEXT: {
            if (outfmt == FMT_PY) {
                warn("will not convert from a text file to a Python file!");
            } else {
                not_implemented;
            }
        } break;
        case FMT_APPVAR: {
            if (outfmt == FMT_PY) {
                disasm(in->data, in->len);
                Ti_PyFile pyf = appvar_to_py(in);
                const char* path = NULL;
                if (args.outfile.len != 0)
                    path = args.outfile.data;

                if (!ti_pyfile_write_file(&pyf, path)) {
                    ti_pyfile_free(&pyf);
                    fatal("failed to write output file: \"%s\"",
                          strerror(errno));
                    return false;
                }

                ti_pyfile_free(&pyf);
            } else if (outfmt == FMT_TEXT) {
                not_implemented;
            } else {
                fatal("could not infer output file format, please specify it "
                      "with a file path or -t");
            }
        } break;
        default:
            unreachable;
    }
    return true;
}

int main(int argc, char** argv) {
    if (parse_args(argc, argv))
        return -1;

    // get input file format
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

    if (infmt == outfmt) {
        warn("input and output formats are the same, no conversion done");
        return 1;
    }

    if (!convert(&infile_contents, infmt, outfmt)) {
        fatal("error occurred during conversion!");
    }

    a_string_free(&infile_contents);
    deinit();
    return 0;
}
