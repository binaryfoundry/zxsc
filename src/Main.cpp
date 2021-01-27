#include "Main.hpp"

#include "System.hpp"

extern "C" {
#include "Z80.h"
}

SDLSystem::System sys;

void update()
{
    int window_width, window_height;
    SDL_GetWindowSize(sys.window, &window_width, &window_height);
    SDL_SetRenderDrawColor(sys.renderer, 0, 0, 0, 255);
    SDL_RenderClear(sys.renderer);

    sys.FrameUpdate();
}

int main(int argc, char *argv[])
{
    sys.Init([=]() { update(); });
    sys.Run();
    return 0;
}
