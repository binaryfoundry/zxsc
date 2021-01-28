#include "SDL.hpp"

SDL_Window* sdl_window = nullptr;

std::function<void(uint16_t key)> sdl_key_up_callback;
std::function<void(uint16_t key)> sdl_key_down_callback;
