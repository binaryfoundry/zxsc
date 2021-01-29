#pragma once

#include <vector>

#include "GUI.hpp"

#include "speccy/Render.hpp"

class Main
{
private:

    std::vector<uint32_t> display_pixels;

    uint32_t update_count = 0;

    GUI gui;
    Speccy::Render speccy_render;

public:
    void Init();
    void Deinit();
    void Update();
};
