#include "Main.hpp"

#include "System.hpp"

#include <array>

extern "C" {
#include "speccy/Speccy.h"
}

SDLSystem::System sdl;

SDL_Texture* display_texture;
std::array<uint32_t, DISPLAY_BYTES> display_pixels;

zx_t zx_sys;
zx_desc_t zx_desc;

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
