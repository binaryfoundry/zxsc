#include "Main.hpp"

#include "sdl/SDL.hpp"
#include "sdl/SDLFile.hpp"

#include <array>
#include <vector>

extern "C" {
#include "speccy/Speccy.h"
}

SDL_Texture* display_texture;
std::array<uint32_t, DISPLAY_BYTES> display_pixels;

zx_t zx_sys;
zx_desc_t zx_desc;

uint32_t update_count = 0;

uint16_t remap_stuntcar_keys(uint16_t key);

void init()
{
    display_texture = SDL_CreateTexture(
        sdl_renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT);

    display_pixels.fill(
        255 << 16 | 255 << 8);

    sdl_key_up_callback = [=](uint16_t key)
    {
        zx_key_up(&zx_sys, remap_stuntcar_keys(key));
    };

    sdl_key_down_callback = [=](uint16_t key)
    {
        zx_key_down(&zx_sys, remap_stuntcar_keys(key));
    };

    zx_desc.pixel_buffer = &display_pixels[0];
    zx_desc.pixel_buffer_size = DISPLAY_PIXEL_BYTES;

    zx_init(
        &zx_sys,
        &zx_desc);
}

void deinit()
{
    SDL_DestroyTexture(
        display_texture);
}

void update()
{
    if (update_count == 180)
    {
        File file("files/scr.z80", "rb");
        std::vector<uint8_t> data(file.Length());
        file.Read(&data[0], sizeof(uint8_t), file.Length());
        zx_quickload(&zx_sys, &data[0], file.Length());
        printf("file loaded\n");
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
        sdl_renderer,
        0,
        0,
        0,
        255);

    SDL_RenderClear(
        sdl_renderer);

    SDL_RenderCopy(
        sdl_renderer,
        display_texture,
        NULL,
        NULL);

    update_count++;
}

uint16_t remap_stuntcar_keys(uint16_t key)
{
    switch (key)
    {
    case 80: return 111; break;
    case 81: return 120; break;
    case 79: return 112; break;
    case 82: return 115; break;
    case 224: return 109; break;
    }
    return key;
}
