/*
 * tipyconv: Tools to conver to and from the TI Python file format.
 *
 * Copyright (c) Eason Qin (eason@ezntek.com), 2025.
 *
 * This source code form is licensed under the BSD 3-clause License. You may
 * find the full text of the license in the root of the project directory at
 * `LICENSE.md`. Alternatively, find an online copy at
 * https://spdx.org/licenses/BSD-3-Clause.html.
 */

#ifndef _TIPYCONV_H
#define _TIPYCONV_H

#include "3rdparty/asv/a_string.h"
#include "common.h"

#include <stdbool.h>
#include <string.h>

#define VAR_NAME_SZ  8
#define FILE_INFO_SZ 42

typedef struct {
    // python source code (null terminated)
    const char* src;
    // file name (within payload) (null terminated)
    const char* file_name;
    // length of source code
    u16 src_len;
    // length of file name (within payload)
    u8 file_name_len;
    // file info metadata (null-termination not guaranteed!)
    char file_info[42];
    // var name (null-termination not guaranteed!)
    char var_name[8];
} TiPyFile;

/**
 * Creates a new TiPyFile with minimal metadata.
 *
 * var_name will be truncated to 8 bytes if necessary. Leaving it null will give
 * it a default name.
 *
 * @param src source code
 * @param src_len source code length
 * @param var_name variable name in calculator
 */
TiPyFile tipyfile_new(const char* src, usize src_len, const char* var_name);

/**
 * Creates a new TiPyFile with all metadata. Strings have to be null_terminated,
 * except for the file_info and var_name fields.
 *
 * file_info and var_name will be truncated if no null terminator is found.
 *
 * @param src source code
 * @param file_name long form file name
 * @param file_info file info
 * @param var_name variable name in calculator
 */
TiPyFile tipyfile_new_with_metadata(const char* src, const char* file_name,
                                    const char* file_info,
                                    const char* var_name);

/**
 * Creates a new TiPyFile with all metadata. Allows string lengths to be passed
 * in.
 *
 * Truncates var_name and file_info if necessary.
 *
 * @param src source code
 * @param src_len source len
 * @param file_name long form file name
 * @param file_name_len file name length
 * @param file_info file info
 * @param var_name variable name in calculator
 */
TiPyFile tipyfile_new_with_metadata_full(const char* src, u16 src_len,
                                         const char* file_name,
                                         u8 file_name_len,
                                         const char* file_info,
                                         const char* var_name);

TiPyFile tipyfile_new_invalid(void);

/**
 * Dumps the `TiPyFile` into a malloc'ed buffer.
 *
 * @param dest destination buffer
 * @param buf_len buffer length
 * @return length of buffer, -1 on error
 */
usize tipyfile_dump(TiPyFile* f, char* dest);

bool tipyfile_valid(const TiPyFile* f);
void tipyfile_free(TiPyFile* f);

#ifdef _TIPYCONV_IMPLEMENTATION
static const char FILE_HEADER[] = {0x2a, 0x2a, 0x54, 0x49, 0x38, 0x33,
                                   0x46, 0x2a, 0x1a, 0x0a, 0x00};

TiPyFile tipyfile_new(const char* src, usize src_len, const char* var_name) {
    char* a_src = calloc(1, src_len);
    check_alloc(a_src);
    strncpy(a_src, src, src_len);

    TiPyFile res = {
        .src = a_src,
        .src_len = src_len,
    };

    if (var_name) {
        strncpy(res.var_name, var_name, VAR_NAME_SZ);
    } else {
        strncpy(res.var_name, "TIPYFILE", VAR_NAME_SZ);
    }

    return res;
}

TiPyFile tipyfile_new_with_metadata(const char* src, const char* file_name,
                                    const char* file_info,
                                    const char* var_name) {
    return tipyfile_new_with_metadata_full(
        src, strlen(src), file_name, strlen(file_name), file_info, var_name);
}

TiPyFile tipyfile_new_with_metadata_full(const char* src, u16 src_len,
                                         const char* file_name,
                                         u8 file_name_len,
                                         const char* file_info,
                                         const char* var_name) {
    char* a_src = calloc(1, src_len + 1);
    char* a_fname = calloc(1, file_name_len + 1);
    check_alloc(a_src);
    check_alloc(a_fname);
    strncpy(a_src, src, src_len);
    strncpy(a_fname, file_name, file_name_len);

    TiPyFile res = {
        .src = a_src,
        .src_len = src_len,
        .file_name = a_fname,
        .file_name_len = file_name_len,
    };

    if (var_name) {
        strncpy(res.var_name, var_name, VAR_NAME_SZ);
    } else {
        strncpy(res.var_name, "TIPYFILE", VAR_NAME_SZ);
    }
    strncpy(res.file_info, file_info, FILE_INFO_SZ);

    return res;
}

TiPyFile tipyfile_new_invalid(void) { return (TiPyFile){0}; }

usize tipyfile_dump(TiPyFile* f, char* dest) {
    // we at least need that much
    a_string res = a_string_with_capacity(81);
    if (!a_string_valid(&res))
        return -1;

    // header
    a_string_append(&res, FILE_HEADER);
    // file info
    a_string_append(&res, f->file_info);

    // 19 bytes (metadata) + sizeof("PYCD\0")
    u16 dsize = 24 + f->src_len;
    dsize = (dsize << 8) | (dsize >> 8);
}

bool tipyfile_valid(const TiPyFile* f) {
    return !memcmp(&f, &(TiPyFile){0}, sizeof(TiPyFile));
}

void tipyfile_free(TiPyFile* f) {
    if (f->file_name)
        free((void*)f->file_name);
    if (f->src)
        free((void*)f->src);
}

#endif // _TIPYCONV_IMPLEMENTATION

#endif // _TIPYCONV_H
