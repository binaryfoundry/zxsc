#include "Render.hpp"

namespace Speccy
{
    static const std::string vertex_shader_string =
        R"(#version 100
        #ifdef GL_ES
        precision mediump float;
        #endif
        uniform mat4 projection;
        uniform mat4 view;
        attribute vec3 position;
        attribute vec2 texcoord;
        varying vec2 v_texcoord;
        void main()
        {
            v_texcoord = texcoord;
            gl_Position = projection * view * vec4(position, 1.0);
        })";

    static const std::string fragment_shader_string =
        R"(#version 100
        #ifdef GL_ES
        precision mediump float;
        #endif
        varying vec2 v_texcoord;
        uniform sampler2D tex;
        uniform int flip;
        vec3 to_linear_approx(vec3 v) { return pow(v, vec3(2.2)); }
        vec3 to_gamma_approx(vec3 v) { return pow(v, vec3(1.0 / 2.2)); }
        void main()
        {
            vec2 tc = flip == 1 ? vec2(v_texcoord.x, 1.0 - v_texcoord.y) : v_texcoord;
            vec3 c = texture2D(tex, tc).xyz;
            gl_FragColor = vec4(c, 1.0);
        })";

    static const std::vector<float> quad_vertices_data
    {
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        1.0f, 1.0f
    };

    static const std::vector<uint32_t> quad_indices_data
    {
         0, 1, 2, 2, 3, 0
    };

    const float speccy_aspect = 1.33333f;
    const uint8_t super_sampling = 12;

    Render::Render()
    {
    }

    void Render::Init(
        const uint32_t width,
        const uint32_t height,
        std::vector<uint32_t>& display_pixels)
    {
        display_width = width;
        display_height = height;

        display_texture_data = reinterpret_cast<uint8_t*>(
            &display_pixels[0]);

        display_texture = OpenGL::GenTextureRGBA8(
            display_width,
            display_height,
            display_texture_data);

        OpenGL::GenFrameBufferRGBA8(
            display_width * super_sampling,
            display_height * super_sampling,
            true,
            frame_buffer);

        gl_shader_program = OpenGL::LinkShader(
            vertex_shader_string,
            fragment_shader_string);

        OpenGL::GLCheckError();

        position_attribute_location = glGetAttribLocation(
            gl_shader_program,
            "position");

        texcoord_attribute_location = glGetAttribLocation(
            gl_shader_program,
            "texcoord");

        projection_uniform_location = glGetUniformLocation(
            gl_shader_program,
            "projection");

        view_uniform_location = glGetUniformLocation(
            gl_shader_program,
            "view");

        texture_uniform_location = glGetUniformLocation(
            gl_shader_program,
            "tex");

        texture_uniform_flip = glGetUniformLocation(
            gl_shader_program,
            "flip");

        vertex_buffer = OpenGL::GenBuffer(
            quad_vertices_data);

        index_buffer = OpenGL::GenBufferIndex(
            quad_indices_data);

        OpenGL::GLCheckError();
    }

    void Render::Deinit()
    {
        frame_buffer.Delete();

        glDeleteProgram(
            gl_shader_program);

        glDeleteBuffers(
            1, &vertex_buffer);

        glDeleteBuffers(
            1, &index_buffer);
    }

    void Render::Draw(
        const uint32_t window_width,
        const uint32_t window_height,
        const uint32_t border_color,
        const bool supersampling)
    {
        glDisable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        // Update Speccy display

        glActiveTexture(
            GL_TEXTURE0);

        glBindTexture(
            GL_TEXTURE_2D,
            display_texture);

        const GLuint gl_internal_format = GL_RGBA;
        const GLuint gl_format = GL_RGBA;
        const GLuint gl_type = GL_UNSIGNED_BYTE;

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            gl_internal_format,
            display_width,
            display_height,
            0,
            gl_format,
            gl_type,
            (GLvoid*)display_texture_data);

        glGenerateMipmap(
            GL_TEXTURE_2D);

        // Render Speccy display to FBO

        if (supersampling)
        {
            glBindFramebuffer(
                GL_FRAMEBUFFER,
                frame_buffer.frame);

            glViewport(
                0, 0,
                frame_buffer.width,
                frame_buffer.height);

            glClearColor(0, 0, 0, 0);

            glClear(
                GL_COLOR_BUFFER_BIT |
                GL_DEPTH_BUFFER_BIT |
                GL_STENCIL_BUFFER_BIT);

            const glm::mat4 proj_fb = glm::ortho<float>(
                0,
                static_cast<float>(frame_buffer.width),
                static_cast<float>(frame_buffer.height),
                0,
                -1.0f,
                1.0f);

            glm::mat4 view_fb;

            view_fb = glm::scale(
                view_fb,
                glm::vec3(frame_buffer.width, frame_buffer.height, 1.0f));

            DrawDisplay(
                proj_fb,
                view_fb,
                display_texture,
                true,
                GL_NEAREST,
                GL_NEAREST);

            glBindFramebuffer(
                GL_FRAMEBUFFER,
                0);
        }

        // Render to front buffer

        glViewport(
            0,
            0,
            window_width,
            window_height);

        float border_r = static_cast<float>(
            (border_color & 0x000000ff) >> 0) / 255.0f;

        float border_g = static_cast<float>(
            (border_color & 0x0000ff00) >> 8) / 255.0f;

        float border_b = static_cast<float>(
            (border_color & 0x00ff0000) >> 16) / 255.0f;

        glClearColor(
            border_r,
            border_g,
            border_b,
            1);

        glClear(
            GL_COLOR_BUFFER_BIT |
            GL_DEPTH_BUFFER_BIT |
            GL_STENCIL_BUFFER_BIT);

        const float window_aspect =
            static_cast<float>(window_width) /
            window_height;

        const float aspect = window_aspect / speccy_aspect;
        const bool wide = window_width / speccy_aspect > window_height;

        const glm::vec3 scale = wide ?
            glm::vec3(std::floor(window_width / aspect), window_height, 1) :
            glm::vec3(window_width, std::floor(window_height * aspect), 1);

        const float hpos = wide ?
            std::round((window_width / 2) - (scale.x / 2)) : 0;

        const float vpos = wide ?
            0 : std::round((window_height / 2) - (scale.y / 2));

        const glm::mat4 proj = glm::ortho<float>(
            0,
            static_cast<float>(window_width),
            static_cast<float>(window_height),
            0,
            -1.0f,
            1.0f);

        glm::mat4 view = glm::mat4();

        view = glm::translate(
            view,
            glm::vec3(hpos, vpos, 0.0f));

        view = glm::scale(
            view,
            scale);

        if (supersampling)
        {
            DrawDisplay(
                proj,
                view,
                frame_buffer.texture,
                false,
                GL_LINEAR_MIPMAP_LINEAR,
                GL_NEAREST);
        }
        else
        {
            DrawDisplay(
                proj,
                view,
                display_texture,
                false,
                GL_NEAREST,
                GL_NEAREST);
        }
    }

    void Render::DrawDisplay(
        const glm::mat4 proj,
        const glm::mat4 view,
        const GLuint texture,
        const bool flip,
        const GLint min_filter,
        const GLint mag_filter)
    {
        glUseProgram(
            gl_shader_program);

        glUniformMatrix4fv(
            projection_uniform_location,
            1,
            false,
            &proj[0][0]);

        glUniformMatrix4fv(
            view_uniform_location,
            1,
            false,
            &view[0][0]);

        glUniform1i(
            texture_uniform_flip,
            flip ? 1 : 0);

        glActiveTexture(
            GL_TEXTURE0);

        glBindTexture(
            GL_TEXTURE_2D,
            texture);

        glGenerateMipmap(
            GL_TEXTURE_2D);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            min_filter);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            mag_filter);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_CLAMP_TO_EDGE);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_CLAMP_TO_EDGE);

        glUniform1i(
            texture_uniform_location,
            0);

        glBindBuffer(
            GL_ARRAY_BUFFER,
            vertex_buffer);

        glEnableVertexAttribArray(0);

        glVertexAttribPointer(
            position_attribute_location,
            3,
            GL_FLOAT,
            GL_FALSE,
            5 * sizeof(GLfloat),
            (GLvoid*)0);

        glEnableVertexAttribArray(1);

        glVertexAttribPointer(
            texcoord_attribute_location,
            2,
            GL_FLOAT,
            GL_FALSE,
            5 * sizeof(GLfloat),
            (GLvoid*)(3 * sizeof(GLfloat)));

        glBindBuffer(
            GL_ELEMENT_ARRAY_BUFFER,
            index_buffer);

        glDrawElements(
            GL_TRIANGLES,
            static_cast<GLsizei>(quad_indices_data.size()),
            GL_UNSIGNED_INT,
            static_cast<char const*>(0));

        glBindTexture(
            GL_TEXTURE_2D,
            NULL);

        glUseProgram(
            NULL);
    }
}
