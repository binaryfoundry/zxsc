#pragma once

#if defined (_WIN32) || defined (__WIN32__)
#define SYSTEM_PLATFORM_WIN
#elif defined (linux) || defined (__linux)
#define SYSTEM_PLATFORM_LINUX
#define SYSTEM_PLATFORM_POSIX
#elif defined (__APPLE__) || defined (MACOSX) || defined (macintosh) || defined (Macintosh)
#define SYSTEM_PLATFORM_DARWIN
#define SYSTEM_PLATFORM_POSIX
#endif

#if defined (__i386__) || defined (_M_IX86)
#define SYSTEM_PLATFORM_ARCH_X86
#elif defined (__x86_64__) || defined (_M_AMD64)
#define SYSTEM_PLATFORM_ARCH_X86_64
#endif

#if defined (_MSC_VER)
#define SYSTEM_COMPILER_MSVCPP
#define SYSTEM_COMPILER_FEATURE_NULLPTR
#else
#define SYSTEM_COMPILER_CLANG
#define SYSTEM_COMPILER_FEATURE_NULLPTR
#endif

#if defined (SYSTEM_COMPILER_MSVCPP)
#define __func__ __FUNCTION__
#else
#define __func__ __PRETTY_FUNCTION__
#endif

#ifdef PATH_MAX
#undef PATH_MAX
#endif

#define PATH_MAX 256

#if defined (SYSTEM_PLATFORM_IOS)
#include <CoreFoundation/CoreFoundation.h>
#elif defined (SYSTEM_PLATFORM_OSX)
#include <CoreServices/CoreServices.h>
#include <CoreFoundation/CoreFoundation.h>
#elif defined (SYSTEM_PLATFORM_WIN)
#elif defined (EMSCRIPTEN)
#include <emscripten.h>
#include <emscripten/html5.h>
#endif

#include <memory>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <math.h>
#include <map>
#include <memory>
#include <string>

#if defined (SYSTEM_PLATFORM_DARWIN)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#include <SDL_syswm.h>
#endif

namespace SDLSystem
{
    class System
    {
    private:
        void InitWindow();

    public:
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;

        void Init(std::function<void()> update);
        void Run();
        void FrameUpdate();
        void Destroy();

        void SetMouseActive(bool status);
        bool IsKeyDown(uint16_t key);

        std::function<void()> render_update;
        std::map<uint16_t, bool> key_state;

        bool mouse_active = false;

        uint16_t display_width = 0;
        uint16_t display_height = 0;
        uint16_t window_width = 0;
        uint16_t window_height = 0;

        int16_t mouse_x = -1;
        int16_t mouse_y = -1;
        int16_t mouse_delta_x = 0;
        int16_t mouse_delta_y = 0;

        bool mouse_down_prev = false;
        bool mouse_down = false;

        bool mouse_click_up = false;
        bool mouse_click_down = false;
    };

    class File
    {
    private:
        size_t length;

#if defined (EMSCRIPTEN)
        FILE* handle;
#else
        SDL_RWops* handle;
#endif

    public:
        File(std::string path, std::string mode);
        virtual ~File();

        size_t Read(void* buffer, size_t size, size_t count);

        std::string ReadString(uint16_t count);
        std::string ReadString();

        size_t Length();
    };
}
