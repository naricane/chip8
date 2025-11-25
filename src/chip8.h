#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "error.h"

#define PROGRAM_START 512
#define FONTSET_START 0

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

struct last_key {
    uint8_t key;
    bool pressed;
};

struct chip8 {
    uint8_t memory[4096];
    uint8_t v_regs[16];
    uint16_t index_reg;
    uint16_t pc;
    uint8_t sp;
    uint16_t stack[16];
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t keys[16];
    struct last_key last_key;
    uint32_t screen[SCREEN_WIDTH * SCREEN_HEIGHT];
};

enum chip8_result chip8_load(struct chip8 *chip8, const char *path);
struct chip8_decode_result chip8_exec(struct chip8 *chip8);

#endif