#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <SDL2/SDL.h>
#include "chip8.h"

const static uint8_t keys[16] = {
    SDL_SCANCODE_X,
    SDL_SCANCODE_1,
    SDL_SCANCODE_2,
    SDL_SCANCODE_3,
    SDL_SCANCODE_Q,
    SDL_SCANCODE_W,
    SDL_SCANCODE_E,
    SDL_SCANCODE_A,
    SDL_SCANCODE_S,
    SDL_SCANCODE_D,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_C,
    SDL_SCANCODE_4,
    SDL_SCANCODE_R,
    SDL_SCANCODE_F,
    SDL_SCANCODE_V
};

struct synthesizer_state {
    float timestep;
    float phasestep;
    float phase;
    float freq;
    bool playing;
};

static inline void init_state(struct synthesizer_state *state, float freq, int sample_rate) {
    memset(state, 0, sizeof(struct synthesizer_state));
    state->freq = freq;
    state->timestep = 1.f / sample_rate;
    state->phasestep = state->timestep * freq;
}

static inline void synthesize(struct synthesizer_state *state, float *stream, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        if (!state->playing && state->phase < 0.001f) {
            return;
        }

        state->phase += state->phasestep;
        stream[i] = sinf(state->phase * 2 * M_PI) * 0.5f;
        state->phase = fmodf(state->phase, 1.f);
    }
}

void
audio_callback(void *userdata, Uint8 *stream, int len)
{
    struct synthesizer_state *state = (struct synthesizer_state *)userdata;
    memset(stream, 0, len);
    synthesize(state, (float *)stream, len / sizeof(float));
}

int
main(int argc, char **args)
{
    srand(time(NULL));

    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        goto sdl2_err;
    }

    SDL_Window *window = SDL_CreateWindow(
        "chip8",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        SCREEN_WIDTH * 10,
        SCREEN_HEIGHT * 10,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        goto sdl2_err;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        goto sdl2_err;
    }

    SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);

    SDL_Texture *screen_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        SCREEN_WIDTH,
        SCREEN_HEIGHT);
    if (screen_texture == NULL) {
        goto sdl2_err;
    }
    
    struct synthesizer_state synth_state;
    init_state(&synth_state, 440, 48000);

    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = 48000;
    desired.format = AUDIO_F32SYS;
    desired.channels = 1;
    desired.samples = 512;
    desired.callback = audio_callback;
    desired.userdata = &synth_state;

    SDL_AudioDeviceID audio_device
        = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);

    SDL_PauseAudioDevice(audio_device, 0);

    SDL_Event event;

    if (argc < 2) {
        fprintf(stderr, "chip8 program not specified!\n");
        exit(chip8_invalid_file);
    }

    struct chip8 chip8;
    enum chip8_result result = chip8_load(&chip8, args[1]);
    if (result != chip8_ok) {
        fprintf(
            stderr,
            "error: %s\n",
            chip8_result_to_string(result));
        exit(result);
    }


    uint64_t start = SDL_GetTicks64();
    float accum = 0.f;

    while (true) {
        uint64_t end = SDL_GetTicks64();
        float dt = (end - start) / 1000.f;
        if (dt > 0.25f) {
            dt = 0.25f;
        }
        start = end;
        accum += dt;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                return 0;
            }
        }

        const uint8_t *state = SDL_GetKeyboardState(NULL);
        for (uint8_t i = 0; i < 16; i++) {
            chip8.keys[i] = state[keys[i]];
        }

        while (accum >= 1 / 60.f) {
            for (int i = 0; i < 9; i++) {
                if ((chip8.memory[chip8.pc] & 0xF0) == 0xD0) {
                    i = 9;
                }
                struct chip8_decode_result decode_result = chip8_exec(&chip8);
                if (decode_result.result != chip8_ok) {
                    fprintf(
                        stderr,
                        "error: %s, opcode: %4x\n",
                        chip8_result_to_string(decode_result.result),
                        decode_result.opcode);
                    exit(decode_result.result);
                }
            }
            if (chip8.delay_timer > 0) {
                chip8.delay_timer--;
            }
            if (chip8.sound_timer > 0) {
                synth_state.playing = true;
                chip8.sound_timer--;
            }
            else {
                synth_state.playing = false;
            }


            accum -= 1 / 60.f;
        }

        SDL_UpdateTexture(screen_texture, NULL, chip8.screen, sizeof(chip8.screen[0]) * SCREEN_WIDTH);

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, screen_texture, NULL, NULL);
        SDL_RenderPresent(renderer);

        SDL_Delay(1000 / 60);
    }

    return 0;

sdl2_err:
    fprintf(stderr, "error: %s\n", SDL_GetError());
    exit(chip8_last_own_result);
}
