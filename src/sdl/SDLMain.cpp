#include "SDLMain.hpp"

#if !defined(EMSCRIPTEN)

#include "../Main.hpp"

#include "SDL.hpp"
#include "SDLFile.hpp"

#include <iostream>
#include <functional>
#include <stdint.h>

static bool poll_events();

static int window_width = 320 * 4;
static int window_height = 256 * 4;

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cout << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    sdl_window = SDL_CreateWindow(
        "ZXS",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN);

    sdl_renderer = SDL_CreateRenderer(
        sdl_window,
        -1,
        SDL_RENDERER_PRESENTVSYNC);

    init();

    bool done = false;

    while (!done)
    {
        done = poll_events();

        update();
        SDL_RenderPresent(sdl_renderer);
    }

    deinit();

    SDL_DestroyRenderer(sdl_renderer);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}

static bool poll_events()
{
    SDL_Event event;
    SDL_PumpEvents();

    uint16_t key = 0;

    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_QUIT:
            return true;
            break;

        case SDL_APP_DIDENTERFOREGROUND:
            break;

        case SDL_APP_DIDENTERBACKGROUND:
            break;

        case SDL_APP_LOWMEMORY:
            break;

        case SDL_APP_TERMINATING:
            break;

        case SDL_APP_WILLENTERBACKGROUND:
            break;

        case SDL_APP_WILLENTERFOREGROUND:
            break;

        case SDL_MOUSEMOTION:
            break;

        case SDL_WINDOWEVENT:
            switch (event.window.event)
            {
                case SDL_WINDOWEVENT_RESIZED:
                {
                    break;
                }
            }

        case SDL_MOUSEBUTTONUP:
            break;

        case SDL_MOUSEBUTTONDOWN:
            break;

        case SDL_KEYDOWN:
            key = static_cast<uint16_t>(event.key.keysym.sym);
            sdl_key_down_callback(key);
            break;

        case SDL_KEYUP:
            key = static_cast<uint16_t>(event.key.keysym.sym);
            sdl_key_up_callback(key);
            break;
        }
    }

    return false;
}

#endif
