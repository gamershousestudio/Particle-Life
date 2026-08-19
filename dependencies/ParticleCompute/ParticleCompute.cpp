#include "ParticleCompute.h"
#include <fstream>
#include <sstream>
#include <iostream>

static std::string ReadFile(const std::string &p)
{
    std::ifstream ifs(p);
    std::stringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

ParticleCompute::ParticleCompute() {}
ParticleCompute::~ParticleCompute() { Shutdown(); }

GLuint ParticleCompute::CompileShader(const std::string &src, GLenum type)
{
    GLuint s = glCreateShader(type);
    const char *cstr = src.c_str();
    glShaderSource(s, 1, &cstr, nullptr);
    glCompileShader(s);
    GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0; glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string msg(len, '\0'); glGetShaderInfoLog(s, len, nullptr, msg.data());
        std::cerr << "Shader compile error: " << msg << std::endl;
        glDeleteShader(s);
        return 0;
    }
    return s;
}

bool ParticleCompute::Init(const std::string &shaderPath)
{
    auto src = ReadFile(shaderPath);
    if (src.empty()) { std::cerr << "Failed to read shader: " << shaderPath << std::endl; return false; }
    GLuint cs = CompileShader(src, GL_COMPUTE_SHADER);
    if (!cs) return false;
    program = glCreateProgram();
    glAttachShader(program, cs);
    glLinkProgram(program);
    GLint ok = 0; glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) { GLint len = 0; glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len); std::string msg(len, '\0'); glGetProgramInfoLog(program, len, nullptr, msg.data()); std::cerr << "Program link error: " << msg << std::endl; glDeleteShader(cs); return false; }
    glDeleteShader(cs);

    glGenBuffers(1, &ssboPos);
    glGenBuffers(1, &ssboVel);
    glGenBuffers(1, &ssboColor);
    glGenBuffers(1, &ssboOffsets);
    glGenBuffers(1, &ssboFlat);
    glGenBuffers(1, &ssboInteractions);
    return true;
}

void ParticleCompute::Shutdown()
{
    if (ssboPos) glDeleteBuffers(1, &ssboPos); ssboPos = 0;
    if (ssboVel) glDeleteBuffers(1, &ssboVel); ssboVel = 0;
    if (ssboColor) glDeleteBuffers(1, &ssboColor); ssboColor = 0;
    if (ssboOffsets) glDeleteBuffers(1, &ssboOffsets); ssboOffsets = 0;
    if (ssboFlat) glDeleteBuffers(1, &ssboFlat); ssboFlat = 0;
    if (ssboInteractions) glDeleteBuffers(1, &ssboInteractions); ssboInteractions = 0;
    if (program) { glDeleteProgram(program); program = 0; }
}

void ParticleCompute::UploadParticles(const std::vector<float> &posX, const std::vector<float> &posY, const std::vector<float> &velX, const std::vector<float> &velY, const std::vector<int> &colorIDs)
{
    int n = static_cast<int>(posX.size());
    if (n > currentParticleCapacity)
    {
        currentParticleCapacity = n;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboPos);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*2*currentParticleCapacity, nullptr, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVel);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*2*currentParticleCapacity, nullptr, GL_DYNAMIC_COPY);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboColor);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int)*currentParticleCapacity, nullptr, GL_DYNAMIC_COPY);
    }
    // Interleave pos and vel into vec2 buffers
    std::vector<float> posInterleaved(n*2), velInterleaved(n*2);
    for (int i = 0; i < n; ++i) { posInterleaved[2*i] = posX[i]; posInterleaved[2*i+1] = posY[i]; velInterleaved[2*i] = velX[i]; velInterleaved[2*i+1] = velY[i]; }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboPos);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float)*2*n, posInterleaved.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssboPos);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVel);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float)*2*n, velInterleaved.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssboVel);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboColor);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int)*n, colorIDs.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssboColor);
}

void ParticleCompute::UploadGrid(const std::vector<int> &cellOffsets, const std::vector<int> &flatIndices, int gridCols, int gridRows, int maxCellOffset)
{
    int offN = static_cast<int>(cellOffsets.size());
    if (offN > currentOffsetsCapacity)
    {
        currentOffsetsCapacity = offN;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboOffsets);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int)*currentOffsetsCapacity, nullptr, GL_DYNAMIC_COPY);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboOffsets);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int)*offN, cellOffsets.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssboOffsets);

    int flatN = static_cast<int>(flatIndices.size());
    if (flatN > currentFlatCapacity)
    {
        currentFlatCapacity = flatN;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboFlat);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(int)*currentFlatCapacity, nullptr, GL_DYNAMIC_COPY);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboFlat);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(int)*flatN, flatIndices.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssboFlat);
}

void ParticleCompute::UploadInteractions(const std::vector<std::vector<float>> &interactions, int colorsCount)
{
    int n = colorsCount * colorsCount;
    if (n > currentInteractionsCapacity)
    {
        currentInteractionsCapacity = n;
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboInteractions);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(float)*currentInteractionsCapacity, nullptr, GL_DYNAMIC_COPY);
    }
    std::vector<float> flat(n);
    for (int i = 0; i < colorsCount; ++i) for (int j = 0; j < colorsCount; ++j) flat[i*colorsCount + j] = interactions[i][j];
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboInteractions);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float)*n, flat.data());
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssboInteractions);
    currentColorsCount = colorsCount;
}

void ParticleCompute::Dispatch(int particleCount, int gridCols, int gridRows, int maxCellOffset, float interactRange, float repelRange, float interactForce, float repelForce, float simDelta)
{
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "u_particleCount"), particleCount);
    glUniform1i(glGetUniformLocation(program, "u_gridCols"), gridCols);
    glUniform1i(glGetUniformLocation(program, "u_gridRows"), gridRows);
    glUniform1i(glGetUniformLocation(program, "u_maxCellOffset"), maxCellOffset);
    // the main code sets other uniforms before dispatch in main.cpp; keep minimal here

    glUniform1f(glGetUniformLocation(program, "u_interactRange"), interactRange);
    glUniform1f(glGetUniformLocation(program, "u_repelRange"), repelRange);
    glUniform1f(glGetUniformLocation(program, "u_interactForce"), interactForce);
    glUniform1f(glGetUniformLocation(program, "u_repelForce"), repelForce);
    glUniform1f(glGetUniformLocation(program, "u_simDelta"), simDelta);
    glUniform1i(glGetUniformLocation(program, "u_colorsCount"), currentColorsCount);

    const int local = 256;
    int groups = (particleCount + local - 1) / local;
    glDispatchCompute(groups, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void ParticleCompute::ReadBackParticles(std::vector<float> &posX, std::vector<float> &posY, std::vector<float> &velX, std::vector<float> &velY)
{
    int n = static_cast<int>(posX.size());
    std::vector<float> posInterleaved(n*2), velInterleaved(n*2);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboPos);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float)*2*n, posInterleaved.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssboVel);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(float)*2*n, velInterleaved.data());
    for (int i = 0; i < n; ++i) { posX[i] = posInterleaved[2*i]; posY[i] = posInterleaved[2*i+1]; velX[i] = velInterleaved[2*i]; velY[i] = velInterleaved[2*i+1]; }
}
