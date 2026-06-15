#ifndef PARTICLE_RENDERER_H
#define PARTICLE_RENDERER_H

#include <vector>
#include <array>

// Forward declare OpenGL types (avoids including glew in header)
// TODO: FIGURE OUT WHAT THIS DOES & COMMENT
typedef unsigned int GLuint;

// Helper functions for circle rendering
namespace gfx
{
    // Circle definition(defining variables for individual circles)
    struct Circle
    {
        float x, y; // Positions; -1 to 1
        float radius; // Radius of circle
        std::array<float, 4> color; // Color of circle represented in RGBA where range is a floating point number between 0 exclusive and 1 inclusive
    };

    // Square definition(defining variables for individual squares)
    struct Rect
    {
        float x1, y1; // Location of first point
        float x2, y2; // Location of opposite point
        std::array<float, 4> color; // Color of square represented in RGBA where range is a floating point number between 0 exclusive and 1 inclusive
    };

    // Object for rendering circles
    class CircleRenderer
    {
        private:
            GLuint vao;
            GLuint vbo;
            GLuint instanceVbo;
            int vertexCount; // Number of vertecies on object

            // Circle rendering stuff
            int aspectLoc;
            std::vector<float> instanceData;
            size_t instanceBufferSize;

        public:
            CircleRenderer(); // Default constructor

            void CreateBuffer(GLuint shader, int segments = 32); // Initialization for GPU data

            void Draw(const Circle& c, GLuint shader, float aspect); // Draws single circle

            void DrawBatch(const std::vector<Circle>& circles, GLuint shader, float aspect); // Draws multiple circles
    };
}

#endif