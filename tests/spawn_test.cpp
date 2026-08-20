// Spawn Test: Verifies that Spawning Appends To SoA Buffers Without Overwriting
// Existing Simulated Positions.
//
// WHY: The GUI spawn path must not reset positions of already-moving
// particles. This test uses project APIs to create an initial particle set,
// spawn additional particles, and assert that the original particle positions
// remain unchanged after SoA synchronization.

#include <iostream>
#include <vector>
#include <cstdlib>
#include <cmath>
#include "SimConfig.h"
#include "Particle.h"

int main()
{
    // Deterministic seed for reproducibility.
    srand(12345);

    const int initial = 10000;
    const int varietyCount = std::min(colorsCount, 4);

    // Create initial particles using the project's initializer so semantics
    // match the runtime code.
    auto particles = InitializeParticles(initial, varietyCount);

    // Record initial positions from AoS (Particle objects) so we can compare
    // after we perform the spawn and convert to SoA.
    std::vector<std::pair<float,float>> before;
    before.reserve(particles.size());
    for (auto &p : particles) { auto pos = p.GetPosition(); before.emplace_back(pos[0], pos[1]); }

    // Spawn new particles in a small region (simulating a GUI spawn).
    double minX = -0.5, maxX = -0.4, minY = 0.3, maxY = 0.35;
    int selColor = 1;
    const int spawnN = 50;
    for (int i = 0; i < spawnN; ++i)
        particles.emplace_back(gfx::Circle{static_cast<float>(minX + (rand() / (float)RAND_MAX) * (maxX - minX)), static_cast<float>(minY + (rand() / (float)RAND_MAX) * (maxY - minY)), (float)radius, Particle::GetColor(selColor, 1.0f, varietyCount)}, selColor);

    // Convert the combined AoS list to SoA buffers using the project's helper.
    std::vector<float> posX, posY, velX, velY; std::vector<int> colorIDs;
    SyncParticlesToSoA(particles, posX, posY, velX, velY, colorIDs);

    // Verify that the original particle positions were preserved after the
    // spawn and AoS->SoA conversion. Fail the test on the first mismatch.
    bool ok = true;
    for (size_t i = 0; i < before.size(); ++i)
    {
        if (std::fabs(posX[i] - before[i].first) > 1e-6f || std::fabs(posY[i] - before[i].second) > 1e-6f)
        {
            ok = false;
            std::cerr << "Mismatch at " << i << " before=" << before[i].first << "," << before[i].second << " after=" << posX[i] << "," << posY[i] << "\n";
            break;
        }
    }

    // Regression check: if the SoA buffers are updated, the AoS list must also
    // reflect the live particle positions before UI selection and deletion use it.
    std::vector<float> livePosX = posX;
    std::vector<float> livePosY = posY;
    for (size_t i = 0; i < livePosX.size(); ++i)
    {
        livePosX[i] += 0.25f;
        livePosY[i] -= 0.5f;
    }
    SyncSoAToParticles(particles, livePosX, livePosY);
    for (size_t i = 0; i < before.size(); ++i)
    {
        const auto actual = particles[i].GetPosition();
        if (std::fabs(actual[0] - livePosX[i]) > 1e-6f || std::fabs(actual[1] - livePosY[i]) > 1e-6f)
        {
            ok = false;
            std::cerr << "AoS sync mismatch after live update: expected " << livePosX[i] << "," << livePosY[i] << " got " << actual[0] << "," << actual[1] << "\n";
            break;
        }
    }

    // Deletion regression: removing a middle particle must not leave the
    // remaining velocity data misaligned with particle order.
    std::vector<Particle> deleteParticles = particles;
    std::vector<float> deletePosX = livePosX;
    std::vector<float> deletePosY = livePosY;
    std::vector<float> deleteVelX(deleteParticles.size(), 0.5f);
    std::vector<float> deleteVelY(deleteParticles.size(), -0.25f);
    std::vector<int> deleteColorIDs(deleteParticles.size(), 1);
    std::vector<size_t> deleteIdx = {deleteParticles.size() / 2};
    RemoveParticlesByIndices(deleteParticles, deletePosX, deletePosY, deleteVelX, deleteVelY, deleteColorIDs, deleteIdx);
    if (deleteParticles.size() != deletePosX.size() || deleteParticles.size() != deleteVelX.size() || deleteParticles.size() != deleteColorIDs.size())
    {
        ok = false;
        std::cerr << "Delete sync mismatch after removing middle particle\n";
    }

    if (ok) { std::cout << "spawn_test: PASS\n"; return 0; }
    else { std::cout << "spawn_test: FAIL\n"; return 2; }
}
