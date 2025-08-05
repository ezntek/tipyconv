#include <stdio.h>

#include "3rdparty/asv/a_string.h"
#include "3rdparty/include/a_string.h"
#include "common.h"

static const char HEADER_DATA[] = {0x2a, 0x2a, 0x54, 0x49, 0x38,
                                   0x33, 0x46, 0x2a, 0x1a, 0x0a};

a_string convert(const a_string* file_contents) {
    // push header
    a_string res = astr(HEADER_DATA);

    // push padding bytes
    // 21 words = 42 bytes
    char empty[42] = {0};
    a_string_append_cstr(&res, empty);

    //

    a_string_append_astr(&res, file_contents);

    return res;
}

void disasm_bin(const char* title, const char* data, usize len) {
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
    u16 res = (u16)(data[0]) | (u16)(data[1] << 8);
    printf("\033[1m%s: \033[0m%d\n", title, res);
    return res;
}

void disasm(const a_string* file_contents) {
    // header
    const char* data = file_contents->data;
    disasm_bin("hdr", &data[0], 11);
    disasm_string("finfo", &data[0xB], 42);
    disasm_num("dsize", &data[0x35]);
    disasm_bin("fill", &data[0x37], 2);
    disasm_num("psize", &data[0x39]);
    disasm_bin("vid", &data[0x3B], 1);
    disasm_string("vname", &data[0x3C], 8);
    disasm_num("psize", &data[0x46]);
    u16 plen = disasm_num("plen", &data[0x48]);
    disasm_string("pyfmt", &data[0x4A], 5);
    plen -= 5;
    disasm_string("payload", &data[0x4F], plen);
    const size_t after_payload = 0x4F + plen;
    disasm_bin("checksum", &data[after_payload],
               file_contents->len - after_payload);
}

int main(int argc, char** argv) {
    argc--;
    argv++;

    a_string filename = {0};
    if (argc == 0) {
        filename = a_string_input("enter file path: ");
        if (!a_string_valid(&filename)) {
            panic("failed to read user input");
        }
        a_string_inplace_trim(&filename);
    } else {
        filename = astr(argv[0]);
    }

    a_string file_contents = a_string_read_file(filename.data);
    if (!a_string_valid(&file_contents)) {
        panic("failed to read file contents");
    }

    disasm(&file_contents);

    // a_string_free(&res);
    a_string_free(&filename);
    a_string_free(&file_contents);

    return 0;
}
