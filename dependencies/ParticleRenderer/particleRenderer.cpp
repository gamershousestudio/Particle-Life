#include "particleRenderer.h"

#include <GL/glew.h>
#include <cmath>
#include <iostream>
#include <numbers>

namespace gfx
{
    #pragma region CircleRenderer
    // Initializes empty circle buffer
    CircleRenderer::CircleRenderer(): vao(0), vbo(0), instanceVbo(0), vertexCount(0), aspectLoc(-1), instanceBufferSize(0) {}

    /* Buffer Initialization */
    // Initializes vertex array for circle
    void CircleRenderer::CreateBuffer(GLuint shader, int segments)
    {
        // Allows for transparency
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Initialization of new vertecies
        std::vector<float> vertices;

        // Center of circle (for triangle fan)
        vertices.push_back(0.0f);
        vertices.push_back(0.0f);

        // Generate unit circle
        for (int i = 0; i <= segments; i++)
        {
            // Generates angle for each segment (distributes evenly throughout circle)
            float angle = 2.0f * std::numbers::pi * i / segments;

            // Puts points at x and y positions based on given angle
            vertices.push_back(cos(angle));
            vertices.push_back(sin(angle));
        }

        // Number of vertices in the triangle fan: center + one vertex per segment + repeat first vertex
        vertexCount = segments + 2;

        // Creates vertex array & binds to OpenGL
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        // Creates vertex buffer & binds to OpenGL
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);

        // Passes necessary data into vertex buffer object(vbo)
        glBufferData(GL_ARRAY_BUFFER,
                     vertices.size() * sizeof(float),
                     vertices.data(),
                     GL_STATIC_DRAW);

        // The circle shape is stored as a vertex list; attribute 0 is the per-vertex position
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Create instance buffer for per-particle attributes
        glGenBuffers(1, &instanceVbo);
        glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
        glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW); // allocate an empty dynamic buffer first

        // instancePosition (vec2) for each particle
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(0 * sizeof(float)));
        glVertexAttribDivisor(1, 1); // advance once per instance instead of once per vertex

        // instanceRadius (float) for each particle
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(2 * sizeof(float)));
        glVertexAttribDivisor(2, 1);

        // instanceColor (vec4) for each particle
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
        glVertexAttribDivisor(3, 1);

        // Unbinds intermediates
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        // Find uniforms
        aspectLoc = glGetUniformLocation(shader, "u_Aspect");
    }

    /* Single Circle Drawing */
    // Draws circle based on already provided information and given circle information
    void CircleRenderer::Draw(const Circle &c, GLuint shader, float aspect)
    {
        // What shader and vertex array OpenGL should use to render
        glUseProgram(shader);
        glBindVertexArray(vao);

        // Where each variable is located in the shader so they can be set
        GLint posLoc = glGetUniformLocation(shader, "u_Position");
        GLint scaleLoc = glGetUniformLocation(shader, "u_Scale");
        GLint aspLoc = glGetUniformLocation(shader, "u_Aspect");
        GLint colorLoc = glGetUniformLocation(shader, "desiredColor");


        // Makes sure shader works correctly
        if (posLoc == -1 || scaleLoc == -1 || aspLoc == -1 || colorLoc == -1)
        {
            std::cout << "Warning: shader uniform not found." << std::endl;
        }

        // Binds shader variables
        glUniform2f(posLoc, c.x, c.y);
        glUniform1f(scaleLoc, c.radius);
        glUniform1f(aspLoc, aspect);
        glUniform4f(colorLoc, c.color[0], c.color[1], c.color[2], c.color[3]);

        // Draws circle(aka triangle fan)
        glDrawArrays(GL_TRIANGLE_FAN, 0, vertexCount);

        // Unbinds intermediates
        glBindVertexArray(0);
    }

    /* Multiple Circles Drawing */
    // Draws multiple circles based on already provided information and given circles information
    void CircleRenderer::DrawBatch(const std::vector<Circle>& circles, GLuint shader, float aspect)
    {
        if (circles.empty())
            return;

        // prepare instance attribute data for all particles in a single contiguous buffer
        instanceData.resize(circles.size() * 7);
        for (size_t i = 0; i < circles.size(); ++i)
        {
            const Circle& c = circles[i];
            float *dst = instanceData.data() + i * 7;
            dst[0] = c.x;
            dst[1] = c.y;
            dst[2] = c.radius;
            dst[3] = c.color[0];
            dst[4] = c.color[1];
            dst[5] = c.color[2];
            dst[6] = c.color[3];
        }

        glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
        size_t byteSize = instanceData.size() * sizeof(float);
        if (byteSize > instanceBufferSize)
        {
            // If the stored buffer is too small, allocate a larger one.
            // This happens only when the number of particles grows beyond the previous maximum.
            glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(byteSize), instanceData.data(), GL_DYNAMIC_DRAW);
            instanceBufferSize = byteSize;
        }
        else
        {
            // If the buffer already has enough space, update only the used portion.
            // This avoids reallocating the GPU buffer every frame.
            glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(byteSize), instanceData.data());
        }

        // What shader and vertex array OpenGL should use to render
        glUseProgram(shader);
        glBindVertexArray(vao);

        if (aspectLoc == -1)
        {
            std::cout << "Warning: shader uniform not found!" << std::endl;
        }

        glUniform1f(aspectLoc, aspect);
        // Draw the same circle mesh once per particle instance using the instance buffer
        glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, vertexCount, static_cast<GLsizei>(circles.size()));

        // Unbinds intermediates
        glBindVertexArray(0);
    }

    #pragma endregion
}