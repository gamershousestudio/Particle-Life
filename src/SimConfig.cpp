#include "SimConfig.h"
#include <random>
#include <filesystem>

namespace {
std::filesystem::path SafeCurrentPath()
{
    try
    {
        return std::filesystem::current_path();
    }
    catch (const std::filesystem::filesystem_error &)
    {
        return std::filesystem::path("/");
    }
}

std::string ResolveFontPath()
{
    const std::filesystem::path relativePath = "res/fonts/Uroob-Regular.ttf";
    const std::filesystem::path fallbackPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";

    const std::filesystem::path currentRoot = SafeCurrentPath();
    const std::array<std::filesystem::path, 6> roots = {
        currentRoot,
        currentRoot.parent_path(),
        currentRoot.parent_path().parent_path(),
        std::filesystem::path("/home/arobertson/Documents/GitHub/Particle-Life"),
        std::filesystem::path("/home/arobertson/Documents/GitHub/Particle-Life/build"),
        std::filesystem::path("/")
    };

    for (const auto &root : roots)
    {
        const std::filesystem::path candidate = root / relativePath;
        if (std::filesystem::exists(candidate) && std::filesystem::is_regular_file(candidate))
            return candidate.string();
    }

    if (std::filesystem::exists(fallbackPath) && std::filesystem::is_regular_file(fallbackPath))
        return fallbackPath.string();

    return relativePath.string();
}
}

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

const bool punishClusters = true;

const bool side = 0; // Left = 0; right = 1
const float length = .6f;

const std::string fontPath = ResolveFontPath();

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
