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

        GLuint display_texture = 0;
        uint8_t* display_texture_data = nullptr;

        GLuint vertex_buffer = 0;
        GLuint index_buffer = 0;

        glm::mat4 proj;
        glm::mat4 view;

    public:
        Render();

        void Init(std::vector<uint32_t>& display_pixels);
        void Deinit();
        void Draw();
    };
}
