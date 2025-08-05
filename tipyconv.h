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

#include "3rdparty/asv/a_vector.h"
#include "common.h"

#include <stdbool.h>
#include <stdio.h>
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
 * Assumes string lengths are equal to strlen(the string) given that it were
 * null-terminated.
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
usize tipyfile_dump(TiPyFile* f, char** dest);

bool tipyfile_valid(const TiPyFile* f);
void tipyfile_free(TiPyFile* f);

#ifdef _TIPYCONV_IMPLEMENTATION

#define BSWORD(w) ((u8[]){(u8)w, (u8)(w >> 8)})

A_VECTOR_DECL(u8)

A_VECTOR_IMPL(u8)

static const char FILE_HEADER[] = {0x2a, 0x2a, 0x54, 0x49, 0x38, 0x33,
                                   0x46, 0x2a, 0x1a, 0x0a, 0x00};

TiPyFile tipyfile_new(const char* src, usize src_len, const char* var_name) {
    return tipyfile_new_with_metadata_full(src, src_len, NULL, 0, NULL,
                                           var_name);
}

TiPyFile tipyfile_new_with_metadata(const char* src, const char* file_name,
                                    const char* file_info,
                                    const char* var_name) {
    u16 src_len = 0;
    u8 file_name_len = 0;

    if (!src)
        return tipyfile_new_invalid();
    src_len = strlen(src);

    if (file_name)
        file_name_len = strlen(file_name);

    return tipyfile_new_with_metadata_full(src, src_len, file_name,
                                           file_name_len, file_info, var_name);
}

TiPyFile tipyfile_new_with_metadata_full(const char* src, u16 src_len,
                                         const char* file_name,
                                         u8 file_name_len,
                                         const char* file_info,
                                         const char* var_name) {
    if (!src)
        return tipyfile_new_invalid();

    char* a_src = calloc(1, src_len + 1);
    check_alloc(a_src);
    strncpy(a_src, src, src_len);

    char* a_fname = NULL;
    if (file_name) {
        a_fname = calloc(1, file_name_len + 1);
        check_alloc(a_fname);
        strncpy(a_fname, file_name, file_name_len);
    }

    TiPyFile res = {
        .src = a_src,
        .src_len = src_len,
        .file_name = a_fname,
        .file_name_len = file_name_len,
    };

    if (var_name)
        strncpy(res.var_name, var_name, VAR_NAME_SZ);
    else
        strncpy(res.var_name, "PYFILE", VAR_NAME_SZ);

    if (file_info)
        strncpy(res.file_info, file_info, FILE_INFO_SZ);

    return res;
}

TiPyFile tipyfile_new_invalid(void) { return (TiPyFile){0}; }

static a_vector_u8 _tipyfile_dump_payload(TiPyFile* f) {
    a_vector_u8 res =
        a_vector_u8_with_capacity(f->src_len + f->file_name_len + 8);

    a_vector_u8_append_slice(&res, (u8*)"PYCD", 4);
    if (f->file_name) {
        a_vector_u8_append_slice(&res, (u8[]){f->file_name_len, 0x01}, 2);
        a_vector_u8_append_slice(&res, (u8*)f->file_name, f->file_name_len);
    }
    a_vector_u8_append(&res, 0x0);
    a_vector_u8_append_slice(&res, (u8*)f->src, f->src_len);

    return res;
}

// only call this when the whole file thus far has been populated!
static u16 _tipyfile_get_checksum(a_vector_u8* res) {
    u32 sum = 0;
    for (usize i = 0x37; i < res->len; i++) {
        sum += res->data[i];
    }
    if (sum > 1 << 16) {
        sum = sum % 1 << 16;
    }
    return (u16)sum;
}

usize tipyfile_dump(TiPyFile* f, char** dest) {
    // we at least need that much
    a_vector_u8 res = a_vector_u8_with_capacity(81);
    if (!a_vector_u8_valid(&res))
        return -1;

    // header
    a_vector_u8_append_slice(&res, (u8*)FILE_HEADER, LENGTH(FILE_HEADER));
    // file info
    a_vector_u8_append_slice(&res, (u8*)f->file_info, FILE_INFO_SZ);

    // 19 bytes (metadata) + sizeof("PYCD\0")
    u16 dsize = 24 + f->src_len;
    if (f->file_name) {
        // length (bytes) + SOH
        dsize += f->file_name_len + 2;
    }

    // hacky but whatever!
    a_vector_u8_append_slice(&res, BSWORD(dsize), 2);
    a_vector_u8_append_slice(&res, (u8[]){0x0d, 0x00}, 2);

    a_vector_u8 payload = _tipyfile_dump_payload(f);
    u16 psize = (u16)payload.len + 2;

    // payload size
    a_vector_u8_append_slice(&res, BSWORD(psize), 2);

    // var id
    a_vector_u8_append(&res, 0x15);

    // var name
    a_vector_u8_append_slice(&res, (u8*)f->var_name, 8);

    // padding
    a_vector_u8_append_slice(&res, (u8*)&(u16){0}, 2);

    // payload size
    a_vector_u8_append_slice(&res, BSWORD(psize), 2);
    psize -= 2;
    a_vector_u8_append_slice(&res, BSWORD(psize), 2);
    a_vector_u8_append_vector(&res, &payload);

    // checksum
    u16 checksum = _tipyfile_get_checksum(&res);
    a_vector_u8_append_slice(&res, BSWORD(checksum), 2);

    a_vector_u8_free(&payload);

    *dest = (char*)res.data;
    return res.len;
}

bool tipyfile_valid(const TiPyFile* f) {
    return !memcmp(f, &(TiPyFile){0}, sizeof(TiPyFile));
}

void tipyfile_free(TiPyFile* f) {
    if (f->file_name)
        free((void*)f->file_name);
    if (f->src)
        free((void*)f->src);
}

#endif // _TIPYCONV_IMPLEMENTATION

#endif // _TIPYCONV_H
