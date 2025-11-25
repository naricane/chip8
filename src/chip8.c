#include "chip8.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

static const uint8_t fontset[] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

static inline int
rand_between(int min, int max)
{
    return (rand() % (max - min + 1)) + min;
}

static inline enum chip8_result
chip8_decode(uint16_t opcode, struct chip8 *chip8)
{
    uint16_t addr = opcode & 0x0FFF;
    uint8_t n_nibble = opcode & 0x000F;
    uint8_t low_byte = opcode & 0x00FF;
    uint8_t x_nibble = (opcode & 0x0F00) >> 8;
    uint8_t y_nibble = (opcode & 0x00F0) >> 4;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (opcode & 0x00FF) {
                case 0x00E0:
                    memset(chip8->screen, 0, sizeof(chip8->screen));
                    break;
                case 0x00EE:
                    chip8->sp--;
                    chip8->pc = chip8->stack[chip8->sp];
                    break;
                default:
                    goto err;
                    break;
            }
            break;
        case 0x1000:
            chip8->pc = addr;
            break;
        case 0x2000:
            chip8->stack[chip8->sp] = chip8->pc;
            chip8->sp++;
            chip8->pc = addr;
            break;
        case 0x3000:
            if (chip8->v_regs[x_nibble] == low_byte) {
                chip8->pc += 2;
            }
            break;
        case 0x4000:
            if (chip8->v_regs[x_nibble] != low_byte) {
                chip8->pc += 2;
            }
            break;
        case 0x5000:
            if (chip8->v_regs[x_nibble] == chip8->v_regs[y_nibble]) {
                chip8->pc += 2;
            }
            break;
        case 0x6000:
            chip8->v_regs[x_nibble] = low_byte;
            break;
        case 0x7000:
            chip8->v_regs[x_nibble] += low_byte;
            break;
        case 0x8000:
            switch (opcode & 0x000F) {
                case 0x0000:
                    chip8->v_regs[x_nibble] = chip8->v_regs[y_nibble];
                    break;
                case 0x0001:
                    chip8->v_regs[x_nibble] |= chip8->v_regs[y_nibble];
                    chip8->v_regs[0xF] = 0;
                    break;
                case 0x0002:
                    chip8->v_regs[x_nibble] &= chip8->v_regs[y_nibble];
                    chip8->v_regs[0xF] = 0;
                    break;
                case 0x0003:
                    chip8->v_regs[x_nibble] ^= chip8->v_regs[y_nibble];
                    chip8->v_regs[0xF] = 0;
                    break;
                case 0x0004: {
                    uint16_t val = chip8->v_regs[x_nibble] + chip8->v_regs[y_nibble];
                    uint8_t flag = val > 0xFF;
                    chip8->v_regs[x_nibble] = val;
                    chip8->v_regs[0xF] = flag;
                    break;
                }
                case 0x0005: {
                    uint8_t val = chip8->v_regs[x_nibble] - chip8->v_regs[y_nibble];
                    uint8_t flag = chip8->v_regs[x_nibble] >= chip8->v_regs[y_nibble];
                    chip8->v_regs[x_nibble] = val;
                    chip8->v_regs[0xF] = flag;
                    break;
                }
                case 0x0006: {
                    uint8_t val = chip8->v_regs[y_nibble] >> 1;
                    uint8_t flag = chip8->v_regs[y_nibble] & 0b1;
                    chip8->v_regs[x_nibble] = val;
                    chip8->v_regs[0xF] = flag;
                    break;
                }
                case 0x0007: {
                    uint8_t val = chip8->v_regs[y_nibble] - chip8->v_regs[x_nibble];
                    uint8_t flag = chip8->v_regs[y_nibble] >= chip8->v_regs[x_nibble];
                    chip8->v_regs[x_nibble] = val;
                    chip8->v_regs[0xF] = flag;
                    break;
                }
                case 0x000E: {
                    uint8_t val = chip8->v_regs[y_nibble] << 1;
                    uint8_t flag = chip8->v_regs[y_nibble] >> 7;
                    chip8->v_regs[x_nibble] = val;
                    chip8->v_regs[0xF] = flag;
                    break;
                }
                default:
                    goto err;
                    break;
            }
            break;
        case 0x9000:
            if (chip8->v_regs[x_nibble] != chip8->v_regs[y_nibble]) {
                chip8->pc += 2;
            }
            break;
        case 0xA000:
            chip8->index_reg = addr;
            break;
        case 0xB000:
            chip8->pc = addr + chip8->v_regs[0];
            break;
        case 0xC000:
            chip8->v_regs[x_nibble] = rand_between(0, 255) & low_byte;
            break;
        case 0xD000:
            uint8_t x_start = chip8->v_regs[x_nibble] % SCREEN_WIDTH;
            uint8_t y_start = chip8->v_regs[y_nibble] % SCREEN_HEIGHT;
            chip8->v_regs[0xF] = 0;
            for (uint8_t y = 0; y < n_nibble; y++) {
                uint8_t sprite_byte = chip8->memory[chip8->index_reg + y];
                for (uint8_t x = 0; x < 8; x++) {
                    if (x_start + x >= SCREEN_WIDTH || y_start + y >= SCREEN_HEIGHT) {
                        continue;
                    }
                    uint8_t sprite_pixel = sprite_byte & (128 >> x);
                    uint32_t *screen_pixel = &chip8->screen[(x_start + x) + (y_start + y) * SCREEN_WIDTH];
                    if (sprite_pixel != 0) {
                        if (*screen_pixel != 0) {
                            chip8->v_regs[0xF] = 1;
                        }
                        *screen_pixel ^= 0xFFFFFFFF;
                    }
                }
            }
            break;
        case 0xE000:
           switch (opcode & 0x00FF) {
                case 0x009E:
                    if (chip8->keys[chip8->v_regs[x_nibble]] != 0) {
                        chip8->pc += 2;
                    }
                    break;
                case 0x00A1:
                    if (chip8->keys[chip8->v_regs[x_nibble]] == 0) {
                        chip8->pc += 2;
                    }
                    break;
                default:
                    goto err;
                    break;
            }
            break;
        case 0xF000:
           switch (opcode & 0x00FF) {
                case 0x0007:
                    chip8->v_regs[x_nibble] = chip8->delay_timer;
                    break;
                case 0x000A:
                    bool wait = true;
                    for (uint8_t i = 0; i < 16; i++) {
                        if (chip8->keys[i] == 0) {
                            if (chip8->last_key.key == i && chip8->last_key.pressed == true) {
                                chip8->v_regs[x_nibble] = i;
                                chip8->last_key.pressed = false;
                                wait = false;
                            }
                        }
                        if (chip8->keys[i] != 0) {
                            chip8->last_key.key = i;
                            chip8->last_key.pressed = true;
                        }
                    }
                    if (wait == true) {
                        chip8->pc -= 2;
                    }
                    break;
                case 0x0015:
                    chip8->delay_timer = chip8->v_regs[x_nibble];
                    break;
                case 0x0018:
                    chip8->sound_timer = chip8->v_regs[x_nibble];
                    break;
                case 0x001E:
                    chip8->index_reg = chip8->index_reg + chip8->v_regs[x_nibble];
                    break;
                case 0x0029:
                    chip8->index_reg = FONTSET_START + (chip8->v_regs[x_nibble] & 0xF) * 5;
                    break;
                case 0x0033:
                    uint8_t val = chip8->v_regs[x_nibble];
                    for (uint8_t i = 0; i < 3; i++) {
                        chip8->memory[chip8->index_reg + 2 - i] = val % 10;
                        val /= 10;
                    }
                    break;
                case 0x0055:
                    for (uint8_t i = 0; i <= x_nibble; i++) {
                        chip8->memory[chip8->index_reg & 0xFFF] = chip8->v_regs[i];
                        chip8->index_reg++;
                    }
                    break;
                case 0x0065:
                    for (uint8_t i = 0; i <= x_nibble; i++) {
                        chip8->v_regs[i] = chip8->memory[chip8->index_reg & 0xFFF];
                        chip8->index_reg++;
                    }
                    break;
                default:
                    goto err;
                    break;
            }
            break;
        default:
            goto err;
            break;
    }

    return chip8_ok;

err:
    return chip8_invalid_opcode;
}

enum chip8_result
chip8_load(struct chip8 *chip8, const char *path)
{
    memset(chip8, 0, sizeof(struct chip8));
    chip8->pc = PROGRAM_START;

    memcpy(chip8->memory + FONTSET_START, fontset, sizeof(fontset));

    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return chip8_errno_result(errno);
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    rewind(f);

    if (fsize > 4096 - 512) {
        return chip8_invalid_file;
    }

    fread(chip8->memory + PROGRAM_START, sizeof(char), fsize, f);
    fclose(f);

    return chip8_ok;
}

uint16_t
chip8_fetch(struct chip8 *chip8)
{
    return (chip8->memory[chip8->pc] << 8) | chip8->memory[chip8->pc + 1];
}

struct chip8_decode_result
chip8_exec(struct chip8 *chip8)
{
    uint16_t opcode = chip8_fetch(chip8);

    chip8->pc += 2;

    enum chip8_result result = chip8_decode(opcode, chip8);

    return (struct chip8_decode_result) {
        .result = result,
        .opcode = opcode
    };
}