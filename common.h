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
#include <sys/types.h>

#define VERSION "0.1.1"

#define VERSION_TXT "tipyconv version " VERSION

#define HELP                                                                   \
    "usage: tipyconv [OPTIONS] <filename>\n"                                   \
    "Options:\n"                                                               \
    "  -o, --outfile:       Output path of conversion\n"                       \
    "  -n, --varname:       Name of file in calculator\n"                      \
    "  -V, --version:       Show the version\n"                                \
    "  -v, --verbose:       Show verbose output\n"                             \
    "  -h, --help:          Show this help screen\n"                           \
    "  -l, --license:       Show the license\n"                                \
    "The format of the output file can be inferred from the input "            \
    "file, but\n"                                                              \
    "passing in an invalid input file is disallowed."

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
