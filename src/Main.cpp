#include "Main.hpp"

#include "System.hpp"

#include <array>
#include <vector>

extern "C" {
#include "speccy/Speccy.h"
}

SDLSystem::System sdl;

SDL_Texture* display_texture;
std::array<uint32_t, DISPLAY_BYTES> display_pixels;

zx_t zx_sys;
zx_desc_t zx_desc;

uint32_t update_count = 0;

void init()
{
    display_pixels.fill(
        255 << 16 | 255 << 8);

    zx_desc.pixel_buffer = &display_pixels[0];
    zx_desc.pixel_buffer_size = DISPLAY_PIXEL_BYTES;

    zx_init(
        &zx_sys,
        &zx_desc);
}

void update()
{
    if (update_count == 180)
    {
        SDLSystem::File file("files\\scr.z80", "rb");
        std::vector<uint8_t> data(file.Length());
        file.Read(&data[0], sizeof(uint8_t), file.Length());
        zx_quickload(&zx_sys, &data[0], file.Length());
    }

    zx_exec(
        &zx_sys,
        16667);

    SDL_UpdateTexture(
        display_texture,
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
        display_texture,
        NULL,
        NULL);

    sdl.FrameUpdate();

    update_count++;
}

int main(int argc, char *argv[])
{
    sdl.Init([=]() { update(); });

    display_texture = SDL_CreateTexture(
        sdl.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT);

    init();

    sdl.Run();

    SDL_DestroyTexture(
        display_texture);

    return 0;
}
