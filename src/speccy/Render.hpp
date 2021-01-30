#pragma once

#include "../gl/GL.hpp"
#include "../gl/Math.hpp"

#include <vector>

namespace Speccy
{
    class Render
    {
    private:
        GLuint gl_shader_program = 0;
        GLuint gl_sampler_state = 0;

        GLuint position_attribute_location = 0;
        GLuint texcoord_attribute_location = 0;
        GLuint projection_uniform_location = 0;
        GLuint view_uniform_location = 0;
        GLuint texture_uniform_location = 0;
        GLuint texture_scale_uniform_location = 0;
        GLuint texture_uniform_flip = 0;

        GLuint display_texture = 0;
        uint8_t* display_texture_data = nullptr;

        GLuint vertex_buffer = 0;
        GLuint index_buffer = 0;

        uint32_t display_width = 0;
        uint32_t display_height = 0;

        OpenGL::FrameBuffer frame_buffer;

        void DrawDisplay(
            const glm::mat4 proj,
            const glm::mat4 view,
            const GLuint texture,
            const bool flip,
            const GLint filter);

    public:
        Render();

        void Init(
            const uint32_t display_width,
            const uint32_t display_height,
            std::vector<uint32_t>& display_pixels);
        void Deinit();
        void Draw(
            const uint32_t window_width,
            const uint32_t window_height,
            const uint32_t border_color);
    };
}
