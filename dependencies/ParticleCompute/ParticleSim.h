#pragma once

#include <vector>

namespace pcompute
{
    // Build a flat spatial grid (two-pass counting + scatter) from SoA positions.
    void BuildSpatialGridSoA(const std::vector<float> &posX,
                             const std::vector<float> &posY,
                             int particleCount,
                             int gridCols, int gridRows,
                             std::vector<int> &cellCounts,
                             std::vector<int> &cellOffsets,
                             std::vector<int> &flatIndices,
                             std::vector<int> &writePos);

    // Update particles on CPU using SoA arrays and a built flat spatial grid.
    void UpdateParticlesSoA(std::vector<float> &posX,
                             std::vector<float> &posY,
                             std::vector<float> &velX,
                             std::vector<float> &velY,
                             const std::vector<int> &colorIDs,
                             int particleCount,
                             int gridCols, int gridRows,
                             int maxCellOffset,
                             const std::vector<int> &cellOffsets,
                             const std::vector<int> &flatIndices,
                             const std::vector<std::vector<float>> &interactionMatrix,
                             float interactRange, float repelRange,
                             float interactForce, float repelForce,
                             float simDelta,
                             const std::vector<std::vector<int>> &neighborList,
                             std::vector<int> &indices);

    // Build neighbor list using flat grid (fills neighborList with adjacent particles within maxNeighborDistSq)
    void BuildNeighborList(const std::vector<float> &posX,
                           const std::vector<float> &posY,
                           int particleCount,
                           int gridCols, int gridRows,
                           int maxCellOffset,
                           const std::vector<int> &cellOffsets,
                           const std::vector<int> &flatIndices,
                           float maxNeighborDistSq,
                           std::vector<std::vector<int>> &neighborList);
}
