#pragma once

#include "SimConfig.h"
#include <array>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include "body.h"
#include "particleRenderer.h" // for gfx::Circle

class Particle : public body::Body
{
    float radius;
    int colorIndex;

public:
    static std::array<float, 4> GetColor(int colorIndex, float a, int varietyCount = variety)
    {
        const int effectiveVariety = std::max(1, varietyCount);
        const int clampedIndex = std::clamp(colorIndex, 0, effectiveVariety - 1);

        const auto hsvToRgb = [](float h, float s, float v) {
            const float hue = std::fmod(h, 360.0f);
            const float c = v * s;
            const float x = c * (1.0f - std::fabs(std::fmod(hue / 60.0f, 2.0f) - 1.0f));
            const float m = v - c;

            float r = 0.0f, g = 0.0f, b = 0.0f;
            if (hue < 60.0f) { r = c; g = x; b = 0.0f; }
            else if (hue < 120.0f) { r = x; g = c; b = 0.0f; }
            else if (hue < 180.0f) { r = 0.0f; g = c; b = x; }
            else if (hue < 240.0f) { r = 0.0f; g = x; b = c; }
            else if (hue < 300.0f) { r = x; g = 0.0f; b = c; }
            else { r = c; g = 0.0f; b = x; }

            return std::array<float, 3>{r + m, g + m, b + m};
        };

        float hue = 0.0f;
        if (effectiveVariety == 1) hue = 0.0f;
        else if (effectiveVariety == 2) hue = clampedIndex == 0 ? 0.0f : 180.0f;
        else hue = 360.0f * static_cast<float>(clampedIndex) / static_cast<float>(effectiveVariety);

        const auto rgb = hsvToRgb(hue, 0.85f, 0.95f);
        return {rgb[0], rgb[1], rgb[2], a};
    }

    static std::string GetColorName(int colorIndex, int varietyCount = variety)
    {
        const int effectiveVariety = std::max(1, varietyCount);
        const int clampedIndex = std::clamp(colorIndex, 0, effectiveVariety - 1);

        static const std::array<std::string, 50> hueNames = {
            "Red","Scarlet","Vermilion","Orange-Red","Red-Orange",
            "Orange","Tangerine","Amber","Golden","Yellow-Orange",
            "Yellow","Lemon","Chartreuse","Yellow-Green","Lime",
            "Lime Green","Spring Green","Green","Emerald","Sea Green",
            "Turquoise","Teal","Robin Egg Blue","Cyan","Aqua",
            "Sky Blue","Azure","Cerulean","Dodger Blue","Blue",
            "Cobalt","Sapphire","Indigo","Blue-Violet","Violet",
            "Purple","Amethyst","Orchid","Medium Orchid","Magenta",
            "Fuchsia","Deep Pink","Hot Pink","Rose","Cerise",
            "Crimson","Ruby","Raspberry","Cherry","Red"
        };
        float hue = 0.0f;
        if (effectiveVariety == 1) hue = 0.0f;
        else if (effectiveVariety == 2) hue = (clampedIndex == 0) ? 0.0f : 180.0f;
        else hue = 360.0f * static_cast<float>(clampedIndex) / static_cast<float>(effectiveVariety);
        const int nameIndex = static_cast<int>(std::lround((hue / 360.0f) * static_cast<float>(hueNames.size() - 1)));
        const int clampedNameIndex = std::clamp(nameIndex, 0, static_cast<int>(hueNames.size() - 1));
        return hueNames[clampedNameIndex];
    }

    Particle(const gfx::Circle pos, int colorIndex)
    {
        SetPosition(pos.x, pos.y);
        radius = pos.radius;
        this->colorIndex = colorIndex;
        stable = !punishClusters;
    }

    const gfx::Circle GetProperties()
    {
        return (gfx::Circle){(float)position[0], (float)position[1], radius, GetColor(colorIndex, 1)};
    }

    const int getColorID() const { return colorIndex; }
};

// Sync SoA positions back into AoS Particle objects for UI selection and pointer-based APIs
static inline void SyncSoAToParticles(std::vector<Particle> &particles,
                                     const std::vector<float> &posX,
                                     const std::vector<float> &posY)
{
    size_t n = std::min(particles.size(), posX.size());
    for (size_t i = 0; i < n; ++i)
        particles[i].SetPosition(posX[i], posY[i]);
}

// Sync particle list into SoA buffers (positions, velocities, color ids)
static inline void SyncParticlesToSoA(const std::vector<Particle> &particles,
                                     std::vector<float> &posX,
                                     std::vector<float> &posY,
                                     std::vector<float> &velX,
                                     std::vector<float> &velY,
                                     std::vector<int> &colorIDs)
{
    size_t n = particles.size();
    posX.resize(n);
    posY.resize(n);
    colorIDs.resize(n);
    size_t oldV = velX.size();
    velX.resize(n);
    velY.resize(n);

    for (size_t i = 0; i < n; ++i)
    {
        const auto &p = particles[i].GetPosition();
        posX[i] = p[0];
        posY[i] = p[1];
        colorIDs[i] = particles[i].getColorID();
        if (i >= oldV)
        {
            velX[i] = 0.0f;
            velY[i] = 0.0f;
        }
    }
}

// Remove particles and their matching SoA entries while preserving the
// current live ordering. This keeps the AoS and SoA representations aligned,
// especially when deleting a particle from the middle of the list.
static inline void RemoveParticlesByIndices(std::vector<Particle> &particles,
                                           std::vector<float> &posX,
                                           std::vector<float> &posY,
                                           std::vector<float> &velX,
                                           std::vector<float> &velY,
                                           std::vector<int> &colorIDs,
                                           const std::vector<size_t> &indices)
{
    if (indices.empty()) return;

    std::vector<size_t> sorted = indices;
    std::sort(sorted.begin(), sorted.end(), std::greater<size_t>());

    for (size_t idx : sorted)
    {
        if (idx >= particles.size()) continue;
        particles.erase(particles.begin() + static_cast<std::ptrdiff_t>(idx));
        if (idx < posX.size()) posX.erase(posX.begin() + static_cast<std::ptrdiff_t>(idx));
        if (idx < posY.size()) posY.erase(posY.begin() + static_cast<std::ptrdiff_t>(idx));
        if (idx < velX.size()) velX.erase(velX.begin() + static_cast<std::ptrdiff_t>(idx));
        if (idx < velY.size()) velY.erase(velY.begin() + static_cast<std::ptrdiff_t>(idx));
        if (idx < colorIDs.size()) colorIDs.erase(colorIDs.begin() + static_cast<std::ptrdiff_t>(idx));
    }
}

// Spawns particles randomly around the map
static inline std::vector<Particle> InitializeParticles(int count, int variety)
{
    std::vector<Particle> particles;
    for (int i = 0; i < variety; i++)
    {
        for (int j = 0; j < count/variety; j++)
        {
            float x = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            float y = (rand() / (float)RAND_MAX) * 2.0f - 1.0f;
            particles.emplace_back(gfx::Circle{x, y, (float)radius, Particle::GetColor(i, 1.0f, variety)}, i);
        }
    }
    return particles;
}
