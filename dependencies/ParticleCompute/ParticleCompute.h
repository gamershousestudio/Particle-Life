#pragma once

#include <GL/glew.h>
#include <vector>
#include <string>

class ParticleCompute
{
public:
    ParticleCompute();
    ~ParticleCompute();

    bool Init(const std::string &shaderPath);
    void Shutdown();

    // Upload buffers (pos: x,y interleaved, vel: x,y interleaved)
    void UploadParticles(const std::vector<float> &posX, const std::vector<float> &posY, const std::vector<float> &velX, const std::vector<float> &velY, const std::vector<int> &colorIDs);
    void UploadGrid(const std::vector<int> &cellOffsets, const std::vector<int> &flatIndices, int gridCols, int gridRows, int maxCellOffset);
    void UploadInteractions(const std::vector<std::vector<float>> &interactions, int colorsCount);

    // Dispatch compute shader to update particles in-place
    void Dispatch(int particleCount, int gridCols, int gridRows, int maxCellOffset, float interactRange, float repelRange, float interactForce, float repelForce, float simDelta);

    // Read back updated positions/velocities
    void ReadBackParticles(std::vector<float> &posX, std::vector<float> &posY, std::vector<float> &velX, std::vector<float> &velY);

private:
    GLuint CompileShader(const std::string &src, GLenum type);
    GLuint program = 0;

    GLuint ssboPos = 0;
    GLuint ssboVel = 0;
    GLuint ssboColor = 0;
    GLuint ssboOffsets = 0;
    GLuint ssboFlat = 0;
    GLuint ssboInteractions = 0;

    int currentParticleCapacity = 0;
    int currentFlatCapacity = 0;
    int currentOffsetsCapacity = 0;
    int currentInteractionsCapacity = 0;
    int currentColorsCount = 0;
};
