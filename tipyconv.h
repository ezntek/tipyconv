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
#include <stdlib.h>
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
} Ti_PyFile;

typedef enum {
    TI_PARSE_OK = 0,
    TI_PARSE_ERROR = 1,
    TI_INVALID_FORMAT = 2,
    TI_CHECKSUM_INCORRECT = 3,
} Ti_ParseResult;

/**
 * Creates a new TiPyFile with minimal metadata.
 *
 * var_name will be truncated to 8 bytes if necessary. Leaving it null will give
 * it a default name.
 *
 * @param src source code
 * @param src_len source code length
 * @param var_name variable name in calculator
 * @return valid `TiPyFile` on success, invalid `TiPyFile` on error
 */
Ti_PyFile ti_pyfile_new(const char* src, usize src_len, const char* var_name);

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
 * @return valid `TiPyFile` on success, invalid `TiPyFile` on error
 */
Ti_PyFile ti_pyfile_new_with_metadata(const char* src, const char* file_name,
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
 * @return valid `TiPyFile` on success, invalid `TiPyFile` on error
 */
Ti_PyFile ti_pyfile_new_with_metadata_full(const char* src, u16 src_len,
                                           const char* file_name,
                                           u8 file_name_len,
                                           const char* file_info,
                                           const char* var_name);

/**
 * Parses a binary file and returns a TiPyFile.
 *
 * Asserts that data != NULL.
 * Assumes the binary stream is at least >11 to check for the file header.
 * May segfault.
 *
 * @param data binary data of TI AppVar
 * @param len length of data
 * @param pres result of the parser, if any. Can be left null
 * @return valid `TiPyFile` on success, invalid `TiPyFile` on error
 */
Ti_PyFile ti_pyfile_parse(char* data, usize len, Ti_ParseResult* pres);

/**
 * Creates an invalid TiPyFile. Used to report errors.
 */
Ti_PyFile ti_pyfile_new_invalid(void);

/**
 * Dumps the `TiPyFile` into a malloc'ed buffer.
 *
 * @param dest destination buffer
 * @param buf_len buffer length
 * @return length of buffer, -1 on error
 */
usize ti_pyfile_dump(Ti_PyFile* f, char** dest);

/**
 * Writes a `Ti_PyFile` to a file
 *
 * @param f the file
 * @param path path to write the file to. Will be inferred based on the
 * `Ti_PyFile` if left NULL.
 * @return true on success, faulse on failure (will set errno accordingly)
 */
bool ti_pyfile_write_file(const Ti_PyFile* f, const char* path);

/**
 * Checks if a Ti_PyFile is valid.
 *
 * @param f the file
 * @return true if the file is valid
 */
bool ti_pyfile_valid(const Ti_PyFile* f);

/**
 * Frees a file. If the file is invalid, this is a no-op.
 *
 * @param f the file
 */
void ti_pyfile_free(Ti_PyFile* f);

/**
 * Checks if a buffer is a TI AppVar.
 *
 * Assumes that data is longer than 11 bytes.
 *
 * @param data buffer
 * @return true if the buffer's header is that of an AppVar.
 */
bool ti_is_appvar(const char* data);

#ifdef _TIPYCONV_IMPLEMENTATION

#define BSWORD(w) ((u8[]){(u8)w, (u8)(w >> 8)})

// NOTE: since this aims to be a single-header library with no dependencies, but
// a dynamic array of sorts is needed, a part of a_vector is bundled.
typedef struct {
    u8* data;
    size_t len;
    size_t cap;
} __ti_vector_u8;
static __ti_vector_u8 __ti_vector_u8_with_capacity(size_t cap) {
    __ti_vector_u8 res = {.len = 0, .cap = cap};
    res.data = calloc(res.cap, sizeof(u8));
    check_alloc(res.data);
    return res;
}
void __ti_vector_u8_free(__ti_vector_u8* v) {
    free(v->data);
    v->len = (size_t)-1;
    v->cap = (size_t)-1;
}
static _Bool __ti_vector_u8_valid(__ti_vector_u8* v) {
    return !(v->len == (size_t)-1 || v->cap == (size_t)-1 ||
             v->data == ((void*)0));
}
static void __ti_vector_u8_reserve(__ti_vector_u8* v, size_t cap) {
    if (!__ti_vector_u8_valid(v)) {
        panic("the vector is invalid");
    }
    v->data = realloc(v->data, sizeof(u8) * cap);
    check_alloc(v->data);
    v->cap = cap;
}
static void __ti_vector_u8_append(__ti_vector_u8* v, u8 new_elem) {
    if (!__ti_vector_u8_valid(v)) {
        panic("the vector is invalid");
    }
    if (v->len + 1 > v->cap) {
        __ti_vector_u8_reserve(v, v->cap * 3);
    }
    v->data[v->len++] = new_elem;
}
static void __ti_vector_u8_append_vector(__ti_vector_u8* v,
                                         const __ti_vector_u8* other) {
    if (!__ti_vector_u8_valid(v)) {
        panic("the vector is invalid");
    }
    size_t len = v->len + other->len;
    if (len > v->cap) {
        size_t sz = v->cap;
        while (sz < len)
            sz *= 3;
        __ti_vector_u8_reserve(v, sz);
    }
    memcpy(&v->data[v->len], other->data, sizeof(u8) * other->len);
    v->len += other->len;
}
static void __ti_vector_u8_append_slice(__ti_vector_u8* v, const u8* data,
                                        size_t nitems) {
    if (!__ti_vector_u8_valid(v)) {
        panic("the vector is invalid");
    }
    size_t len = v->len + nitems;
    if (len > v->cap) {
        size_t sz = v->cap;
        while (sz < len)
            sz *= 3;
        __ti_vector_u8_reserve(v, sz);
    }
    memcpy(&v->data[v->len], data, sizeof(u8) * nitems);
    v->len += nitems;
}

static const char FILE_HEADER[] = {0x2a, 0x2a, 0x54, 0x49, 0x38, 0x33,
                                   0x46, 0x2a, 0x1a, 0x0a, 0x00};

Ti_PyFile ti_pyfile_new(const char* src, usize src_len, const char* var_name) {
    return ti_pyfile_new_with_metadata_full(src, src_len, NULL, 0, NULL,
                                            var_name);
}

Ti_PyFile ti_pyfile_new_with_metadata(const char* src, const char* file_name,
                                      const char* file_info,
                                      const char* var_name) {
    u16 src_len = 0;
    u8 file_name_len = 0;

    if (!src)
        return ti_pyfile_new_invalid();
    src_len = strlen(src);

    if (file_name)
        file_name_len = strlen(file_name);

    return ti_pyfile_new_with_metadata_full(src, src_len, file_name,
                                            file_name_len, file_info, var_name);
}

Ti_PyFile ti_pyfile_new_with_metadata_full(const char* src, u16 src_len,
                                           const char* file_name,
                                           u8 file_name_len,
                                           const char* file_info,
                                           const char* var_name) {
    if (!src)
        return ti_pyfile_new_invalid();

    char* a_src = calloc(src_len + 1, 1);
    check_alloc(a_src);
    strncpy(a_src, src, src_len);

    char* a_fname = NULL;
    if (file_name) {
        a_fname = calloc(file_name_len + 1, 1);
        check_alloc(a_fname);
        strncpy(a_fname, file_name, file_name_len);
    }

    Ti_PyFile res = {
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

Ti_PyFile ti_pyfile_new_invalid(void) { return (Ti_PyFile){0}; }

static __ti_vector_u8 _ti_pyfile_dump_payload(Ti_PyFile* f) {
    __ti_vector_u8 res =
        __ti_vector_u8_with_capacity(f->src_len + f->file_name_len + 8);

    __ti_vector_u8_append_slice(&res, (u8*)"PYCD", 4);
    if (f->file_name) {
        __ti_vector_u8_append_slice(&res, (u8[]){f->file_name_len, 0x01}, 2);
        __ti_vector_u8_append_slice(&res, (u8*)f->file_name, f->file_name_len);
    }
    __ti_vector_u8_append(&res, 0x0);
    __ti_vector_u8_append_slice(&res, (u8*)f->src, f->src_len);

    return res;
}

static u16 _ti_pyfile_get_checksum(const char* data, usize len) {
    u32 sum = 0;
    for (usize i = 0x37; i < len; i++) {
        sum += data[i];
    }
    return (u16)(sum & 0xffff);
}

usize ti_pyfile_dump(Ti_PyFile* f, char** dest) {
    // we at least need that much
    __ti_vector_u8 res = __ti_vector_u8_with_capacity(81);
    if (!__ti_vector_u8_valid(&res))
        panic("failed to allocate buffer");

    // header
    __ti_vector_u8_append_slice(&res, (u8*)FILE_HEADER, LENGTH(FILE_HEADER));
    // file info
    __ti_vector_u8_append_slice(&res, (u8*)f->file_info, FILE_INFO_SZ);

    // 19 bytes (metadata) + sizeof("PYCD\0")
    u16 dsize = 24 + f->src_len;
    if (f->file_name) {
        // length (bytes) + SOH
        dsize += f->file_name_len + 2;
    }

    // hacky but whatever!
    __ti_vector_u8_append_slice(&res, BSWORD(dsize), 2);
    __ti_vector_u8_append_slice(&res, (u8[]){0x0d, 0x00}, 2);

    __ti_vector_u8 payload = _ti_pyfile_dump_payload(f);
    u16 psize = (u16)payload.len + 2;

    // payload size
    __ti_vector_u8_append_slice(&res, BSWORD(psize), 2);

    // var id
    __ti_vector_u8_append(&res, 0x15);

    // var name
    __ti_vector_u8_append_slice(&res, (u8*)f->var_name, 8);

    // padding
    __ti_vector_u8_append_slice(&res, (u8*)&(u16){0}, 2);

    // payload size
    __ti_vector_u8_append_slice(&res, BSWORD(psize), 2);
    psize -= 2;
    __ti_vector_u8_append_slice(&res, BSWORD(psize), 2);
    __ti_vector_u8_append_vector(&res, &payload);

    // checksum
    u16 checksum = _ti_pyfile_get_checksum((char*)res.data, res.len);
    __ti_vector_u8_append_slice(&res, BSWORD(checksum), 2);

    __ti_vector_u8_free(&payload);

    *dest = (char*)res.data;
    return res.len;
}

bool ti_pyfile_write_file(const Ti_PyFile* f, const char* path) {
    char* actual_path = NULL;
    if (path != NULL) {
        actual_path = strdup(path);
    } else {
        //  ./ + .py + \0
        if (f->file_name && f->file_name_len > 0) {
            actual_path = calloc(6 + f->file_name_len, 1);
            check_alloc(actual_path);
            strcat(actual_path, "./");
            strcat(actual_path, f->file_name);
        } else {
            // 8 + 14
            actual_path = calloc(14, 1);
            check_alloc(actual_path);
            strcat(actual_path, "./");
            strcat(actual_path, f->var_name);
        }
        strcat(actual_path, ".py");
    }

    FILE* out_fp = fopen(actual_path, "w");
    if (!out_fp) {
        free(actual_path);
        return false;
    }

    // psst! this doesnt handle a short write
    usize bytes_written = fwrite(f->src, 1, f->src_len, out_fp);
    if (bytes_written != f->src_len) {
        free(actual_path);
        return false;
    }

    fclose(out_fp);
    free(actual_path);
    return true;
}

static u16 _ti_pyfile_get_word(char data[2]) {
    return (u16)((u8)(data[0]) | (u8)data[1] << 8);
}

Ti_PyFile ti_pyfile_parse(char* data, usize len, Ti_ParseResult* pres) {
    if (!data) {
        if (pres)
            *pres = TI_PARSE_ERROR;
        return ti_pyfile_new_invalid();
    }

    // check header
    if (memcmp(&data[0], FILE_HEADER, 1) != 0) {
        if (pres)
            *pres = TI_INVALID_FORMAT;
        return ti_pyfile_new_invalid();
    }

    Ti_PyFile res = {0};

    strncpy(res.file_info, &data[0xB], 42);
    strncpy(res.var_name, &data[0x3C], 8);

    u16 src_len = _ti_pyfile_get_word(&data[0x48]);
    src_len -= 5; // skip past PYCD and \0 because it will always be present

    char* file_name = NULL;
    usize src_start = 0x4F;
    // check if there's a long form filename
    if (data[0x4E] != '\0') {
        u8 file_name_len = data[0x4E];
        // 0x4F is SOH, can ignore
        file_name = calloc(file_name_len + 1, 1);
        check_alloc(file_name);
        // should stop at nullterm in stream anyway
        strncpy(file_name, &data[0x50], file_name_len);
        src_start = 0x50 + file_name_len; // payload should start later
        src_len -= 1 + file_name_len;

        res.file_name = file_name;
        res.file_name_len = file_name_len;
    }

    char* src = calloc(src_len + 1, 1);
    check_alloc(src);
    strncpy(src, &data[src_start], src_len);

    // we do not want to include the checksum in the stream already
    u16 checksum = _ti_pyfile_get_checksum(data, len - 2);
    u16 file_checksum = _ti_pyfile_get_word(&data[src_start + src_len]);

    if (checksum != file_checksum) {
        printf("wanted: %d, got: %d\n", file_checksum, checksum);
        if (pres)
            *pres = TI_CHECKSUM_INCORRECT;
        free(src);
        if (file_name)
            free(file_name);
        return ti_pyfile_new_invalid();
    }

    res.src = src;
    res.src_len = src_len;

    if (pres)
        *pres = TI_PARSE_OK;

    return res;
}

bool ti_pyfile_valid(const Ti_PyFile* f) {
    return !memcmp(f, &(Ti_PyFile){0}, sizeof(Ti_PyFile));
}

void ti_pyfile_free(Ti_PyFile* f) {
    if (f->file_name)
        free((void*)f->file_name);
    if (f->src)
        free((void*)f->src);
}

bool ti_is_appvar(const char* data) {
    return (memcmp(data, FILE_HEADER, 1) != 0);
}

#endif // _TIPYCONV_IMPLEMENTATION

#endif // _TIPYCONV_H
