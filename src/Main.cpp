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

#define DISPLAY_BYTES (DISPLAY_WIDTH * DISPLAY_HEIGHT * DISPLAY_PIXEL_BYTES)

SDLSystem::System sys;
SDL_Texture* display;
std::array<uint32_t, DISPLAY_BYTES> display_pixels;

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
        sys.renderer,
        0,
        0,
        0,
        255);

    SDL_RenderClear(
        sys.renderer);

    SDL_RenderCopy(
        sys.renderer,
        display,
        NULL,
        NULL);

    sys.FrameUpdate();
}

int main(int argc, char *argv[])
{
    sys.Init([=]() { update(); });

    display = SDL_CreateTexture(
        sys.renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STATIC,
        DISPLAY_WIDTH,
        DISPLAY_HEIGHT);

    sys.Run();

    SDL_DestroyTexture(display);

    return 0;
}
