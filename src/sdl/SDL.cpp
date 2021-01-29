#include "SDL.hpp"

SDL_Window* sdl_window = nullptr;
int sdl_window_width;
int sdl_window_height;

std::function<void(uint16_t key)> sdl_key_up_callback;
std::function<void(uint16_t key)> sdl_key_down_callback;
