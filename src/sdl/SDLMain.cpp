#include "SDLMain.hpp"

#if !defined(EMSCRIPTEN)

#include "../Main.hpp"

#include "../gl/GL.hpp"

#include "SDL.hpp"
#include "SDLFile.hpp"

#include <iostream>
#include <functional>
#include <stdint.h>

#include <EGL/egl.h>
#include <EGL/eglext.h>

static SDL_GLContext gl;
static EGLDisplay egl_display;
static EGLContext egl_context;
static EGLSurface egl_surface;

static int init_graphics();
static bool poll_events();

static int window_width = 320 * 4;
static int window_height = 256 * 4;

Main m;

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

    init_graphics();

    m.Init();

    bool done = false;

    while (!done)
    {
        done = poll_events();

        m.Update();
        eglSwapBuffers(egl_display, egl_surface);
    }

    m.Deinit();

    SDL_GL_DeleteContext(gl);
    SDL_DestroyWindow(sdl_window);
    SDL_Quit();

    return 0;
}

static int init_graphics()
{
    sdl_window = SDL_CreateWindow(
        "ZXS",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_width,
        window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

    SDL_SysWMinfo info;
    SDL_VERSION(
        &info.version);

    SDL_bool get_win_info = SDL_GetWindowWMInfo(
        sdl_window,
        &info);
    SDL_assert_release(
        get_win_info);

    EGLNativeWindowType hWnd = reinterpret_cast<EGLNativeWindowType>(
        info.info.win.window);

    EGLint err;
    EGLint numConfigs;
    EGLint major_version;
    EGLint minor_version;
    EGLDisplay display;
    EGLContext context;
    EGLSurface surface;
    EGLConfig config;

    EGLint config_attribs[] =
    {
        EGL_RED_SIZE,       8,
        EGL_GREEN_SIZE,     8,
        EGL_BLUE_SIZE,      8,
        EGL_ALPHA_SIZE,     8,
        EGL_DEPTH_SIZE,     24,
        EGL_SAMPLE_BUFFERS, 1,
        EGL_SAMPLES,        2,
        EGL_SURFACE_TYPE,   EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT_KHR,
        EGL_BUFFER_SIZE,    16,
        EGL_NONE
    };

    EGLint context_attribs[] =
    {
        EGL_CONTEXT_MAJOR_VERSION_KHR, 2,
        EGL_CONTEXT_MINOR_VERSION_KHR, 0,
        EGL_NONE
    };

    EGLint surface_attribs[] =
    {
        //EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        //EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,
        //EGL_COLORSPACE, EGL_COLORSPACE_sRGB,
        //EGL_GL_COLORSPACE_KHR, EGL_GL_COLORSPACE_SRGB_KHR,
        EGL_NONE
    };

    display = eglGetDisplay(GetDC(hWnd));

    if (display == EGL_NO_DISPLAY)
        goto failed;
    if (!eglInitialize(
        display,
        &major_version,
        &minor_version))
        goto failed;
    if (!eglBindAPI(EGL_OPENGL_ES_API))
        goto failed;
    if (!eglGetConfigs(
        display,
        NULL,
        0,
        &numConfigs))
        goto failed;
    if (!eglChooseConfig(
        display,
        config_attribs,
        &config,
        1,
        &numConfigs))
        goto failed;
    if (numConfigs != 1) {
        goto failed;
    }

    surface = eglCreateWindowSurface(
        display,
        config,
        hWnd,
        surface_attribs);
    if (surface == EGL_NO_SURFACE)
        goto failed;
    context = eglCreateContext(
        display,
        config,
        EGL_NO_CONTEXT,
        context_attribs);
    if (context == EGL_NO_CONTEXT)
        goto failed;
    if (!eglMakeCurrent(
        display,
        surface,
        surface,
        context))
        goto failed;

failed:
    if ((err = eglGetError()) != EGL_SUCCESS)
    {
        std::cout << err << std::endl;
        return 1;
    }

    egl_display = display;
    egl_surface = surface;
    egl_context = context;

    auto r = eglSwapInterval(egl_display, 1);

    //SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);

    const char *error = SDL_GetError();
    if (*error != '\0')
    {
        std::cout << error << std::endl;
        SDL_ClearError();
        return 1;
    }

    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "OpenGL Version: " << version << std::endl;

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