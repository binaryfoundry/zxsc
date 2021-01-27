#include "System.hpp"

#include <iostream>
#include <vector>

namespace SDLSystem
{
    System* system;
    std::function<void()> render_func;

    void render_update_func()
    {
        render_func();
    }

    void System::Init(std::function<void()> update)
    {
        render_update = update;

#if !defined(EMSCRIPTEN)
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
        {
            std::cout <<
                "Could not initialize SDL" <<
                std::endl;
            std::cout << SDL_GetError() << std::endl;
            SDL_Quit();
            throw false;
        }
#endif

        if (SDL_Init(SDL_INIT_VIDEO) < 0)
        {
            std::cout <<
                "Error while initializing the SDL : " <<
                SDL_GetError() <<
                std::endl;
            SDL_Quit();
            throw false;
        }

        SDL_DisplayMode DM;
        SDL_GetCurrentDisplayMode(0, &DM);
        display_width = DM.w;
        display_height = DM.h;

        window_width = 320 * 4;
        window_height = 256 * 4;

        InitWindow();
    }

    void System::SetMouseActive(bool status)
    {
        mouse_delta_x = 0;
        mouse_delta_y = 0;
        mouse_active = status;
        SDL_SetRelativeMouseMode(static_cast<SDL_bool>(status));
    }

    bool System::IsKeyDown(uint16_t key)
    {
        if (key_state.find(key) != key_state.end())
        {
            return key_state[key];
        }
        return false;
    }

    void System::InitWindow()
    {
        render_func = render_update;
        system = this;

        window = SDL_CreateWindow(
            "ZXS",
            SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED,
            window_width,
            window_height,
            SDL_WINDOW_SHOWN);

#if  defined (SYSTEM_PLATFORM_WIN)


#elif defined(EMSCRIPTEN)

        emscripten_set_element_css_size(
            NULL,
            window_width,
            window_height);
        emscripten_set_canvas_element_size(
            "#canvas",
            int(window_width),
            int(window_height));

#endif

        renderer = SDL_CreateRenderer(
            window, -1, 0);

#if !defined(EMSCRIPTEN)
        const char *error = SDL_GetError();
        if (*error != '\0')
        {
            std::stringstream err;
            std::cerr <<
                "SDL Error: " <<
                error <<
                std::endl;
            SDL_ClearError();
            //throw err.str();
        }
#endif
    }

    void System::Destroy()
    {
        SDL_DestroyWindow(window);
    }

    void System::FrameUpdate()
    {
        system->mouse_delta_x = 0;
        system->mouse_delta_y = 0;

        SDL_RenderPresent(renderer);
    }

#ifdef EMSCRIPTEN

    uint16_t element_width;
    uint16_t element_height;
    bool is_full_screen = false;

    EM_BOOL em_fullscreen_callback(
        int eventType,
        const EmscriptenFullscreenChangeEvent *fullscreenChangeEvent,
        void *userData)
    {
        is_full_screen = fullscreenChangeEvent->isFullscreen;
        if (is_full_screen)
        {
            int fullscreenWidth = (int)(emscripten_get_device_pixel_ratio() * fullscreenChangeEvent->screenWidth + 0.5);
            int fullscreenHeight = (int)(emscripten_get_device_pixel_ratio() * fullscreenChangeEvent->screenHeight + 0.5);
            system->window_width = (uint16_t)fullscreenWidth;
            system->window_height = (uint16_t)fullscreenHeight;
        }
        return false;
    }

    EM_BOOL em_pointerlock_callback(
        int eventType,
        const EmscriptenPointerlockChangeEvent *pointerEvent,
        void *userData)
    {
        if (eventType == EMSCRIPTEN_EVENT_POINTERLOCKCHANGE)
        {
            if (pointerEvent->isActive)
            {
                system->SetMouseActive(true);
            }
            else
            {
                system->SetMouseActive(false);
            }
        }
        return false;
    }

    EM_BOOL em_mouse_click_callback(
        int eventType,
        const EmscriptenMouseEvent *mouseEvent,
        void *userData)
    {
        bool is_inside =
            mouseEvent->canvasX > 0 &&
            mouseEvent->canvasY > 0 &&
            mouseEvent->canvasX < system->window_width &&
            mouseEvent->canvasY < system->window_height;

        if (mouseEvent->button == 0 && is_inside)
        {
            if (!system->mouse_active)
            {
                system->SetMouseActive(true);
            }
        }

        return false;
    }

    EM_BOOL em_mouse_move_callback(
        int eventType,
        const EmscriptenMouseEvent *mouseEvent,
        void *userData)
    {
        system->mouse_x = mouseEvent->canvasX;
        system->mouse_y = mouseEvent->canvasY;
        system->mouse_delta_x = mouseEvent->movementX;
        system->mouse_delta_y = mouseEvent->movementY;

        return false;
    }

    EM_BOOL on_canvassize_changed(int eventType, const void *reserved, void *userData)
    {
        int w, h, fs;
        double cssW, cssH;
        emscripten_get_element_css_size(0, &cssW, &cssH);
        system->window_width = (uint16_t)cssW;
        system->window_height = (uint16_t)cssH;
        printf("Canvas resized: WebGL RTT size: %dx%d, canvas CSS size: %02gx%02g\n", w, h, cssW, cssH);
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
        uint16_t key = static_cast<uint16_t>(keyEvent->which);
        if (!system->key_state[70] && key == 70 && !is_full_screen)
        {
            enter_full_screen();
        }

        system->key_state[key] = true;
        return false;
    }

    EM_BOOL em_key_up_callback(
        int eventType,
        const EmscriptenKeyboardEvent *keyEvent,
        void *userData)
    {
        uint16_t key = static_cast<uint16_t>(keyEvent->which);
        system->key_state[key] = false;
        return false;
    }

#else

    bool poll_events()
    {
        system->mouse_down_prev = system->mouse_down;

        SDL_Event event;
        uint16_t key;

        SDL_PumpEvents();

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
            case SDL_QUIT:
                return true;
                break;

            case SDL_APP_DIDENTERFOREGROUND:
                SDL_Log("SDL_APP_DIDENTERFOREGROUND");
                break;

            case SDL_APP_DIDENTERBACKGROUND:
                SDL_Log("SDL_APP_DIDENTERBACKGROUND");
                break;

            case SDL_APP_LOWMEMORY:
                SDL_Log("SDL_APP_LOWMEMORY");
                break;

            case SDL_APP_TERMINATING:
                SDL_Log("SDL_APP_TERMINATING");
                break;

            case SDL_APP_WILLENTERBACKGROUND:
                SDL_Log("SDL_APP_WILLENTERBACKGROUND");
                break;

            case SDL_APP_WILLENTERFOREGROUND:
                SDL_Log("SDL_APP_WILLENTERFOREGROUND");
                break;

            case SDL_MOUSEMOTION:
                system->mouse_x = event.motion.x;
                system->mouse_y = event.motion.y;
                system->mouse_delta_x = event.motion.xrel;
                system->mouse_delta_y = event.motion.yrel;
                break;

            case SDL_WINDOWEVENT:
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                    {
                        system->window_width = event.window.data1;
                        system->window_height = event.window.data2;

                        SDL_Log("Window %d resized to %dx%d",
                            event.window.windowID,
                            event.window.data1,
                            event.window.data2);
                        break;
                    }
                }

            case SDL_MOUSEBUTTONUP:
                //if (event.button.button == SDL_BUTTON_LEFT)
                //{
                //    if (!system->mouse_active && event.button.clicks == 2)
                //    {
                //        system->SetMouseActive(true);
                //    }
                //}
                system->mouse_down = false;
                break;

            case SDL_MOUSEBUTTONDOWN:
                system->mouse_down = true;
                break;

            case SDL_KEYDOWN:
                key = static_cast<uint16_t>(event.key.keysym.sym);
                system->key_state[key] = true;
                break;

            case SDL_KEYUP:
                key = static_cast<uint16_t>(event.key.keysym.sym);
                system->key_state[key] = false;
                if (key == 27)
                {
                    system->SetMouseActive(false);
                }
                break;
            }
        }

        system->mouse_click_up = !system->mouse_down && system->mouse_down_prev;
        system->mouse_click_down = system->mouse_down && !system->mouse_down_prev;

        return false;
    }
#endif

    void System::Run()
    {

#if defined(EMSCRIPTEN)

        emscripten_set_pointerlockchange_callback(NULL, NULL, true, em_pointerlock_callback);
        emscripten_set_fullscreenchange_callback(NULL, NULL, true, em_fullscreen_callback);
        //emscripten_set_resize_callback(NULL, NULL, true, em_resize_callback);
        emscripten_set_click_callback(NULL, NULL, true, em_mouse_click_callback);
        emscripten_set_mousemove_callback(NULL, NULL, true, em_mouse_move_callback);
        emscripten_set_keydown_callback(NULL, NULL, true, em_key_down_callback);
        emscripten_set_keyup_callback(NULL, NULL, true, em_key_up_callback);
        emscripten_set_main_loop(render_update_func, 0, true);
        return;

#else

        //SetMouseActive(true);

        bool done = false;

        while (!done)
        {
            done = poll_events();
            render_update_func();
        }

#endif

        Destroy();

        SDL_Quit();
    }

#if defined (EMSCRIPTEN)

    File::File(std::string path, std::string mode)
    {
        handle = fopen(path.c_str(), mode.c_str());

        if (handle == nullptr)
        {
            std::cout << "Failed to open file: " << path << std::endl;
            throw std::runtime_error("Failed to open file.");
        }

        fseek(handle, 0, SEEK_END);
        length = static_cast<size_t>(ftell(handle));
        rewind(handle);
    }

    File::~File()
    {
        fclose(handle);
    }

    size_t File::Read(void* buffer, size_t size, size_t count)
    {
        return fread(buffer, size, count, handle);
    }

    size_t File::Length()
    {
        return length;
    }

#else

    File::File(std::string path, std::string mode)
    {
        handle = SDL_RWFromFile(
            path.c_str(),
            mode.c_str());

        if (handle == nullptr)
        {
            std::cout << "Failed to open file: " << path << std::endl;
            throw std::runtime_error("Failed to open file.");
        }

        length = static_cast<size_t>(SDL_RWseek(
            handle,
            0,
            SEEK_END));

        SDL_RWseek(
            handle,
            0,
            SEEK_SET);
    }

    File::~File()
    {
        SDL_RWclose(handle);
    }

    size_t File::Read(void* buffer, size_t size, size_t count)
    {
        return SDL_RWread(handle, buffer, size, count);
    }

    size_t File::Length()
    {
        return length;
    }

#endif

    std::string File::ReadString(uint16_t count)
    {
        char* buffer = new char[count + 1];

        Read(buffer, sizeof(char), count);
        buffer[count] = 0x00;

        std::string str = std::string(buffer);

        delete[] buffer;

        return str;
    }

    // string prefixed with length (uint16_t)
    std::string File::ReadString()
    {
        uint16_t count;
        Read(&count, sizeof(uint16_t), 1);

        char* buffer = new char[count + 1];

        Read(buffer, sizeof(char), count);
        buffer[count] = 0x00;

        std::string str = std::string(buffer);

        delete[] buffer;

        return str;
    }
}
