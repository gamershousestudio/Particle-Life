#pragma once

#include <array>
#include <vector>
#include <string>

extern const char* programName;
extern const std::array<int,2> aspectRatio;

extern const double clusterFactor;

extern const double interactRange;
extern const double repelRange;

extern const double interactForce;
extern const double repelForce;

extern const double timeMultiplier;
extern double timeSpeed;

extern const double radius;

extern const int count;
extern unsigned int variety;

extern const bool punishClusters;

extern const bool side;
extern const float length;

extern const std::string fontPath;

constexpr int colorsCount = 50;

extern std::vector<std::vector<float>> interactions;

void RandomizeInteractions(std::vector<std::vector<float>> &matrix);
