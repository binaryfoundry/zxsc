#pragma once

#include <functional>

#if defined (IS_PLATFORM_DARWIN)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#include <SDL_syswm.h>
#endif

extern SDL_Window* sdl_window;
extern SDL_Renderer* sdl_renderer;
extern std::function<void(uint16_t key)> sdl_key_up_callback;
extern std::function<void(uint16_t key)> sdl_key_down_callback;
