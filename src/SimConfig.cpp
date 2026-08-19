#include "SimConfig.h"
#include <random>

const char* programName = "Particle Life";

const std::array<int,2> aspectRatio = {(int)(1600*1), (int)(900*1)}; // Width, height

const double clusterFactor = 1; // Increases motivation to form clusters

const double interactRangeMultiplier = .03 * clusterFactor;
const double repelRangeMultiplier = .013 * clusterFactor;
double interactRange = .5;
double repelRange = .5;

const double interactForceMultiplier = 50;
const double repelForceMultiplier = 5;
double interactForce = .3;
double repelForce = .3;

const double timeMultiplier = 1;
double timeSpeed = .1;

const double radius = .01;

const int count = 0;
unsigned int variety = 4; // Total number of different particle types to use; not marked as const as it is changed if too large in main()

const bool punishClusters = false;

const bool side = 0; // Left = 0; right = 1
const float length = .6f;

const std::string fontPath = "res/fonts/Uroob-Regular.ttf";

std::vector<std::vector<float>> interactions(colorsCount, std::vector<float>(colorsCount));

void RandomizeInteractions(std::vector<std::vector<float>> &matrix)
{
    std::random_device rd; // Seed for the generator
    std::mt19937 gen(rd()); // Standard Mersenne Twister engine
    std::uniform_real_distribution<float> distr(-1, 1); // Distribution between -1 and 1

    for(int x = 0; x < colorsCount; x++)
    {
        for(int y = 0; y < colorsCount; y++)
        {
            matrix[x][y] = distr(gen);
        }
    }
}
