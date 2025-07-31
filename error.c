#include "error.h"

static const char *errors[] = {
    [chip8_invalid_file] = "invalid file",
    [chip8_invalid_opcode] = "invalid opcode"
};

enum chip8_result
chip8_errno_result(int err)
{
    return chip8_errno | err;
}

int
chip8_get_errno(enum chip8_result result)
{
    return result & ~chip8_errno;
}

const char *
chip8_result_to_string(enum chip8_result result)
{
    if (result < chip8_errno) {
        return errors[result];
    }
    else if (result >= chip8_errno) {
        return strerror(chip8_get_errno(result));
    }
}