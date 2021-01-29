#include "Main.hpp"

#include "sdl/SDL.hpp"
#include "sdl/SDLFile.hpp"

#include "imgui/imgui.h"

extern "C" {
#include "speccy/Speccy.h"
}

zx_t zx_sys;
zx_desc_t zx_desc;

uint16_t remap_stuntcar_keys(uint16_t key);

void Main::Init()
{
    display_pixels.resize(
        DISPLAY_BYTES);

    speccy_render.Init(
        display_pixels);
    gui.Init();

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

void Main::Deinit()
{
    speccy_render.Deinit();
    gui.Deinit();
}

void Main::Update()
{
    ImGui::NewFrame();
    ImGui::ShowDemoWindow();

    if (update_count == 180)
    {
        File file("files/scr.z80", "rb");
        std::vector<uint8_t> data(file.Length());
        file.Read(&data[0], sizeof(uint8_t), file.Length());
        zx_quickload(&zx_sys, &data[0], static_cast<int>(file.Length()));
        printf("file loaded\n");
    }

    zx_exec(
        &zx_sys,
        16667);

    speccy_render.Draw();
    gui.Draw();

    update_count++;
}

uint16_t remap_stuntcar_keys(uint16_t key)
{
    switch (key)
    {
#if !defined(EMSCRIPTEN)
    case 80: return 111; break;
    case 81: return 120; break;
    case 79: return 112; break;
    case 82: return 115; break;
    case 224: return 109; break;
#else
    case 37: return 111; break;
    case 40: return 120; break;
    case 39: return 112; break;
    case 38: return 115; break;
    case 17: return 109; break;
#endif
    }
    return key;
}
