#include "Main.hpp"

#include "System.hpp"

#include <array>

extern "C" {
#include "Z80.h"
#include "Rom.h"
}

#define DISPLAY_WIDTH 256
#define DISPLAY_HEIGHT 192
#define DISPLAY_PIXEL_BYTES sizeof(uint32_t)
#define SYSTEM_MEMORY_BYTES 49152

#define DISPLAY_BYTES (DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_PIXEL_BYTES)

SDLSystem::System sdl;

SDL_Texture* display;
std::array<uint32_t, DISPLAY_BYTES> display_pixels;

std::array<uint8_t, SYSTEM_MEMORY_BYTES> system_memory;

void init()
{
    system_memory.fill(0);

    memcpy(
        &system_memory[0],
        &rom48k[0],
        16384);
}

void update()
{
    display_pixels.fill(
        255 << 16 | 255 << 8);

    SDL_UpdateTexture(
        display,
        NULL,
        &display_pixels[0],
        DISPLAY_WIDTH * DISPLAY_PIXEL_BYTES);

    SDL_SetRenderDrawColor(
        sdl.renderer,
        0,
        0,
        0,
        255);

    SDL_RenderClear(
        sdl.renderer);

    SDL_RenderCopy(
        sdl.renderer,
        display,
        NULL,
        NULL);

    sdl.FrameUpdate();
}

int main(int argc, char *argv[])
{
    sdl.Init([=]() { update(); });

    display = SDL_CreateTexture(
        sdl.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT);

    init();

    sdl.Run();

    SDL_DestroyTexture(display);

    return 0;
}
