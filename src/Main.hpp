#pragma once

#include <vector>

#include "speccy/Render.hpp"

class Main
{
private:

    //SDL_Texture* display_texture;
    std::vector<uint32_t> display_pixels;

    uint32_t update_count = 0;

    Speccy::Render speccy_render;

public:
    void Init();
    void Deinit();
    void Update();
};
