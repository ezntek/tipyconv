/*
 * Common utility definitions for all C projects.
 *
 * Copyright (c) Eason Qin <eason@ezntek.com>, 2024-2025.
 *
 * This source code form is licensed under the BSD 0-Clause license.
 *
 * INFO: common declarations
 */

#ifndef _COMMON_H
#define _COMMON_H

#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>

typedef size_t usize;
typedef ssize_t isize;
typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short int u16;
typedef signed short int i16;
typedef unsigned int u32;
typedef signed int i32;
typedef unsigned long long int u64;
typedef signed long long int i64;
typedef float f32;
typedef double f64;

#define LENGTH(lst) (i32)(sizeof(lst) / sizeof(lst[0]))

#define S_BOLD "\033[1m"
#define S_DIM  "\033[2m"
#define S_END  "\033[0m"

#define S_BLACK   "\033[30m"
#define S_RED     "\033[31m"
#define S_GREEN   "\033[32m"
#define S_YELLOW  "\033[33m"
#define S_BLUE    "\033[34m"
#define S_MAGENTA "\033[35m"
#define S_CYAN    "\033[36m"
#define S_WHITE   "\033[37m"

#define S_BGBLACK   "\033[40m"
#define S_BGRED     "\033[41m"
#define S_BGGREEN   "\033[42m"
#define S_BGYELLOW  "\033[43m"
#define S_BGBLUE    "\033[44m"
#define S_BGMAGENTA "\033[45m"
#define S_BGCYAN    "\033[46m"
#define S_BGWHITE   "\033[47m"

#define S_CLEAR_SCREEN "\033[2J\033[H"
#define S_CLEAR_LINE   "\r\033[K"

#define S_ENTER_ALT "\033[?1049h"
#define S_LEAVE_ALT "\033[?1049l"

#define S_SHOWCURSOR "\033[?25h"
#define S_HIDECURSOR "\033[?25l"

#define check_alloc(ptr)                                                       \
                                                                               \
    {                                                                          \
        if (ptr == NULL) {                                                     \
            panic("allocation of `%s` failed", #ptr);                          \
            perror("perror");                                                  \
            exit(1);                                                           \
        }                                                                      \
    }

#define eprintf(...) fprintf(stderr, __VA_ARGS__);

#define panic(...)                                                             \
    {                                                                          \
        eprintf("\033[31;1mpanic:\033[0m line %d, func \"%s\" in file "        \
                "\"%s\": ",                                                    \
                __LINE__, __func__, __FILE__);                                 \
        eprintf(__VA_ARGS__);                                                  \
        eprintf(S_DIM                                                          \
                "\n       please report this to the developer.\n" S_END);      \
        exit(1);                                                               \
    }

#define log_error(...)                                                         \
    {                                                                          \
        eprintf(S_RED S_BOLD "[error] " S_END);                                \
        eprintf(S_DIM);                                                        \
        eprintf(__VA_ARGS__);                                                  \
        eprintf(S_END "\n");                                                   \
    }

#define log_fatal(...)                                                         \
    {                                                                          \
        log_error(__VA_ARGS__);                                                \
        exit(1);                                                               \
    }

#define log_warn(...)                                                          \
    {                                                                          \
        eprintf(S_MAGENTA S_BOLD "[warn] " S_END);                             \
        eprintf(S_DIM);                                                        \
        eprintf(__VA_ARGS__);                                                  \
        eprintf(S_END "\n");                                                   \
    }

#define log_info(...)                                                          \
    {                                                                          \
        eprintf(S_CYAN S_BOLD "[info] " S_END);                                \
        eprintf(S_DIM);                                                        \
        eprintf(__VA_ARGS__);                                                  \
        eprintf(S_END "\n");                                                   \
    }

#define not_implemented log_fatal("feature not implemented")
#define unreacheable    panic("reached unreacheable code")

#define VERSION "0.1.0"

#define HELP                                                                   \
    "usage: tipyconv [OPTIONS] <filename>\n"                                   \
    "Options:\n"                                                               \
    "  -o, --outfile:       Output path of conversion\n"                       \
    "  -f, --format:        Format of input file\n"                            \
    "  -t, --target-format: Format of output file\n   "                        \
    "  -N, --varname:       Name of file in calculator (only used for text "   \
    "-> 8xv)\n"                                                                \
    "  -F, --filename:      Long file name in calculator (only used for text " \
    "-> 8xv)\n"                                                                \
    "  -V, --version:       Show the version\n"                                \
    "  -v, --verbose:       Show verbose output\n"                             \
    "  -h, --help:          Show this help screen\n"                           \
    "  -l, --license:       Show the license\n"                                \
    "The source file format will be determined by the argument passed to "     \
    "--format,\n"                                                              \
    "the file extension of <filename> in that order.\n\n"                      \
    "version " VERSION "\n"

#define LICENSE                                                                \
    "BSD 3-Clause License\n"                                                   \
    "\n"                                                                       \
    "Copyright (c) 2025, Eason Qin <eason@ezntek.com>\n"                       \
    "All rights reserved.\n"                                                   \
    "\n"                                                                       \
    "Redistribution and use in source and binary forms, with or without\n"     \
    "modification, are permitted provided that the following conditions are "  \
    "met:\n"                                                                   \
    "\n"                                                                       \
    "1. Redistributions of source code must retain the above copyright "       \
    "notice,\n"                                                                \
    "   this list of conditions and the following disclaimer.\n"               \
    "\n"                                                                       \
    "2. Redistributions in binary form must reproduce the above copyright\n"   \
    "   notice, this list of conditions and the following disclaimer in the\n" \
    "   documentation and/or other materials provided with the "               \
    "distribution.\n"                                                          \
    "\n"                                                                       \
    "3. Neither the name of the copyright holder nor the names of its\n"       \
    "   contributors may be used to endorse or promote products derived "      \
    "from\n"                                                                   \
    "   this software without specific prior written permission.\n"            \
    "\n"                                                                       \
    "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "     \
    "\"AS IS\"\n"                                                              \
    "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, "   \
    "THE\n"                                                                    \
    "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR "      \
    "PURPOSE\n"                                                                \
    "ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS "  \
    "BE\n"                                                                     \
    "LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR\n"    \
    "CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF\n"   \
    "SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR "         \
    "BUSINESS\n"                                                               \
    "INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER "    \
    "IN\n"                                                                     \
    "CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR "            \
    "OTHERWISE)\n"                                                             \
    "ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF "  \
    "THE\n"                                                                    \
    "POSSIBILITY OF SUCH DAMAGE."

#endif
