#ifndef ERROR_H
#define ERROR_H

#include <string.h>
#include <stdint.h>

enum chip8_result {
    chip8_ok,
    chip8_invalid_file,
    chip8_invalid_opcode,

    chip8_last_own_result,
    chip8_errno = 256
};

enum chip8_result chip8_errno_result(int err);
int chip8_get_errno(enum chip8_result result);
const char *chip8_result_to_string(enum chip8_result result);

struct chip8_decode_result {
    enum chip8_result result;
    uint16_t opcode;
};

#endif