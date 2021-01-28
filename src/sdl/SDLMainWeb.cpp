#include "SDLMainWeb.hpp"

#if defined(EMSCRIPTEN)

#include "../Main.hpp"

#include "../gl/GL.hpp"

#include "SDL.hpp"
#include "SDLFile.hpp"
#include "SDLImgui.hpp"

#include <iostream>
#include <functional>
#include <stdint.h>

#include <emscripten.h>
#include <emscripten/html5.h>

static uint32_t element_width;
static uint32_t element_height;
static bool is_full_screen = false;

static void sdl_run();
static void sdl_update();
static int sdl_init_graphics();

Main m;

int main(int argc, char *argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cout << "SDL error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    const char *error = SDL_GetError();
    if (*error != '\0')
    {
        std::cout << "SDL error: " << error;
        SDL_ClearError();
        return 1;
    }

    sdl_imgui_initialise();
    sdl_init_graphics();

    m.Init();

    sdl_run();

    m.Deinit();

    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}

static void sdl_update()
{
    sdl_imgui_update_input(sdl_window);
    sdl_imgui_update_cursor();
    m.Update();
}

static int sdl_init_graphics()
{
    double cssW, cssH;
    emscripten_get_element_css_size(0, &cssW, &cssH);
    element_width = 320 * 4;//static_cast<uint32_t>(cssW);
    element_height = 240 * 4;//static_cast<uint32_t>(cssH);

    sdl_window = SDL_CreateWindow(
        "ZXS",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        element_width,
        element_height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);

    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    attr.alpha = 0;
    attr.depth = 1;
    attr.stencil = 0;
    attr.antialias = 1;
    attr.preserveDrawingBuffer = 0;
    attr.preferLowPowerToHighPerformance = 0;
    attr.failIfMajorPerformanceCaveat = 0;
    attr.enableExtensionsByDefault = 1;
    attr.premultipliedAlpha = 0;
    attr.explicitSwapControl = 0;
    attr.majorVersion = 2;
    attr.minorVersion = 0;

    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(
        0, &attr);

    emscripten_webgl_make_context_current(
        ctx);

    return 0;
}

EM_BOOL em_fullscreen_callback(
    int eventType,
    const EmscriptenFullscreenChangeEvent *fullscreenChangeEvent,
    void *userData)
{
    is_full_screen = fullscreenChangeEvent->isFullscreen;
    if (is_full_screen)
    {
        int fullscreenWidth = (int)(emscripten_get_device_pixel_ratio()
            * fullscreenChangeEvent->screenWidth + 0.5);
        int fullscreenHeight = (int)(emscripten_get_device_pixel_ratio()
            * fullscreenChangeEvent->screenHeight + 0.5);

        element_width = static_cast<uint32_t>(fullscreenWidth);
        element_height = static_cast<uint32_t>(fullscreenHeight);
    }
    return false;
}

EM_BOOL em_pointerlock_callback(
    int eventType,
    const EmscriptenPointerlockChangeEvent *pointerEvent,
    void *userData)
{
    return false;
}

EM_BOOL em_mouse_click_callback(
    int eventType,
    const EmscriptenMouseEvent *mouseEvent,
    void *userData)
{
    return false;
}

EM_BOOL em_mouse_move_callback(
    int eventType,
    const EmscriptenMouseEvent *mouseEvent,
    void *userData)
{
    return false;
}

EM_BOOL on_canvassize_changed(int eventType, const void *reserved, void *userData)
{
    int w, h, fs;
    double cssW, cssH;
    emscripten_get_element_css_size(0, &cssW, &cssH);
    element_width = static_cast<uint32_t>(cssW);
    element_height = static_cast<uint32_t>(cssH);

    printf("Canvas resized: %dx%d, canvas CSS size: %02gx%02g\n", w, h, cssW, cssH);
    return 0;
}

void enter_full_screen()
{
    if (!is_full_screen)
    {
        EM_ASM(JSEvents.inEventHandler = true);
        EM_ASM(JSEvents.currentEventHandler = { allowsDeferredCalls:true });
        EmscriptenFullscreenStrategy s;
        memset(&s, 0, sizeof(s));
        s.scaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
        s.canvasResolutionScaleMode = EMSCRIPTEN_FULLSCREEN_CANVAS_SCALE_HIDEF;
        s.filteringMode = EMSCRIPTEN_FULLSCREEN_FILTERING_DEFAULT;
        s.canvasResizedCallback = on_canvassize_changed;
        EMSCRIPTEN_RESULT ret = emscripten_request_fullscreen_strategy(0, 1, &s);
        is_full_screen = true;
    }
}

extern "C"
{
    EMSCRIPTEN_KEEPALIVE
        void external_enter_full_screen()
    {
        enter_full_screen();
    }
}

EM_BOOL em_key_down_callback(
    int eventType,
    const EmscriptenKeyboardEvent *keyEvent,
    void *userData)
{
    sdl_key_down_callback((uint16_t)keyEvent->keyCode);
    return false;
}

EM_BOOL em_key_up_callback(
    int eventType,
    const EmscriptenKeyboardEvent *keyEvent,
    void *userData)
{
    sdl_key_up_callback((uint16_t)keyEvent->keyCode);
    return false;
}

EM_BOOL em_resize_callback(
    int eventType,
    const EmscriptenUiEvent *e,
    void *userData)
{
    double cssW, cssH;
    emscripten_get_element_css_size(0, &cssW, &cssH);
    element_width = static_cast<uint32_t>(cssW);
    element_height = static_cast<uint32_t>(cssH);
    return 0;
}

//static EM_BOOL em_handle_key_press(
//    int eventType,
//    const EmscriptenKeyboardEvent *keyEvent,
//    void *userData)
//{
//    char text[5];
//    if (Emscripten_ConvertUTF32toUTF8(keyEvent->charCode, text)) {
//        ImGuiIO& io = ImGui::GetIO();
//        io.AddInputCharactersUTF8(text);
//    }
//    return SDL_GetEventState(SDL_TEXTINPUT) == SDL_ENABLE;
//}
//
//static EM_BOOL em_wheel_callback(
//    int eventType,
//    const EmscriptenWheelEvent *wheelEvent,
//    void *userData)
//{
//    ImGuiIO& io = ImGui::GetIO();
//    if (wheelEvent->deltaX > 0) io.MouseWheelH -= 1;
//    if (wheelEvent->deltaX < 0) io.MouseWheelH += 1;
//    if (wheelEvent->deltaY > 0) io.MouseWheel -= 1;
//    if (wheelEvent->deltaY < 0) io.MouseWheel += 1;
//    return SDL_TRUE;
//}

static void sdl_run()
{
    emscripten_set_pointerlockchange_callback(
        NULL, NULL, true, em_pointerlock_callback);
    emscripten_set_fullscreenchange_callback(
        NULL, NULL, true, em_fullscreen_callback);
    emscripten_set_click_callback(
        NULL, NULL, true, em_mouse_click_callback);
    emscripten_set_mousemove_callback(
        NULL, NULL, true, em_mouse_move_callback);
    emscripten_set_keydown_callback(
        NULL, NULL, true, em_key_down_callback);
    emscripten_set_keyup_callback(
        NULL, NULL, true, em_key_up_callback);
    emscripten_set_resize_callback(
        NULL, NULL, true, em_resize_callback);
    //emscripten_set_keypress_callback(
    //    NULL, NULL, true, em_handle_key_press);
    //emscripten_set_wheel_callback(
    //    NULL, NULL, true, em_wheel_callback);
    emscripten_set_element_css_size(
        NULL, element_width, element_height);
    emscripten_set_main_loop(
        sdl_update, 0, true);
}

#endif
