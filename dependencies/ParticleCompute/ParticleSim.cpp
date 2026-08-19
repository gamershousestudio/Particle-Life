#include "ParticleSim.h"
#include <algorithm>
#include <numeric>
#include <execution>
#include <cmath>

namespace pcompute
{

void BuildSpatialGridSoA(const std::vector<float> &posX,
                         const std::vector<float> &posY,
                         int particleCount,
                         int gridCols, int gridRows,
                         std::vector<int> &cellCounts,
                         std::vector<int> &cellOffsets,
                         std::vector<int> &flatIndices,
                         std::vector<int> &writePos)
{
    const int gridCellCount = gridCols * gridRows;

    // First pass: counts
    std::fill(cellCounts.begin(), cellCounts.end(), 0);
    for (int i = 0; i < particleCount; ++i)
    {
        int cx = static_cast<int>(((posX[i] + 1.0f) * 0.5f) * gridCols);
        int cy = static_cast<int>(((posY[i] + 1.0f) * 0.5f) * gridRows);
        cx = std::clamp(cx, 0, gridCols - 1);
        cy = std::clamp(cy, 0, gridRows - 1);
        ++cellCounts[cy * gridCols + cx];
    }

    // Prefix sum offsets
    cellOffsets[0] = 0;
    for (int c = 0; c < gridCellCount; ++c)
        cellOffsets[c + 1] = cellOffsets[c] + cellCounts[c];

    // Ensure flatIndices fits
    flatIndices.resize(particleCount);

    // Prepare write cursors (reuse given vector)
    writePos.assign(cellOffsets.begin(), cellOffsets.begin() + gridCellCount);

    // Scatter indices into flat buffer
    for (int i = 0; i < particleCount; ++i)
    {
        int cx = static_cast<int>(((posX[i] + 1.0f) * 0.5f) * gridCols);
        int cy = static_cast<int>(((posY[i] + 1.0f) * 0.5f) * gridRows);
        cx = std::clamp(cx, 0, gridCols - 1);
        cy = std::clamp(cy, 0, gridRows - 1);
        int cellIndex = cy * gridCols + cx;
        flatIndices[writePos[cellIndex]++] = i;
    }
}

void BuildNeighborList(const std::vector<float> &posX,
                       const std::vector<float> &posY,
                       int particleCount,
                       int gridCols, int gridRows,
                       int maxCellOffset,
                       const std::vector<int> &cellOffsets,
                       const std::vector<int> &flatIndices,
                       float maxNeighborDistSq,
                       std::vector<std::vector<int>> &neighborList)
{
    const int gridCellCount = gridCols * gridRows;
    if (static_cast<int>(neighborList.size()) != particleCount)
        neighborList.assign(particleCount, std::vector<int>());

    const int gridColsLocal = gridCols;
    const auto &px = posX;
    const auto &py = posY;
    const int *flatIdx = flatIndices.data();
    const int *cellOff = cellOffsets.data();

    for (int cell = 0; cell < gridCellCount; ++cell)
    {
        int start = cellOff[cell];
        int end = cellOff[cell + 1];
        if (start == end) continue;

        int cx = cell % gridColsLocal;
        int cy = cell / gridColsLocal;

        for (int oy = -maxCellOffset; oy <= maxCellOffset; ++oy)
        {
            int ny = cy + oy;
            if (ny < 0 || ny >= gridRows) continue;
            for (int ox = -maxCellOffset; ox <= maxCellOffset; ++ox)
            {
                int nx = cx + ox;
                if (nx < 0 || nx >= gridColsLocal) continue;
                int other = ny * gridColsLocal + nx;
                if (other <= cell) continue; // avoid duplicates
                int start2 = cellOff[other];
                int end2 = cellOff[other + 1];
                for (int a = start; a < end; ++a)
                {
                    int i = flatIdx[a];
                    for (int b = start2; b < end2; ++b)
                    {
                        int j = flatIdx[b];
                        float dx = px[j] - px[i]; if (dx > 1.0f) dx -= 2.0f; else if (dx < -1.0f) dx += 2.0f;
                        float dy = py[j] - py[i]; if (dy > 1.0f) dy -= 2.0f; else if (dy < -1.0f) dy += 2.0f;
                        float d2 = dx*dx + dy*dy;
                        if (d2 <= maxNeighborDistSq)
                        {
                            neighborList[i].push_back(j);
                            neighborList[j].push_back(i);
                        }
                    }
                }
            }
        }
    }
}

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
                        std::vector<int> &indices)
{
    // Reuse the provided index buffer to avoid allocations each frame
    indices.resize(particleCount);
    std::iota(indices.begin(), indices.end(), 0);

    const int gridCellCount = gridCols * gridRows;

    // Capture frequently-used references locally for speed
    const float fInteractRange = interactRange;
    const float fRepelRange = repelRange;
    const float fInteractForce = interactForce;
    const float fRepelForce = repelForce;

    // Raw pointers to vector data to help optimizer
    float *px = posX.data();
    float *py = posY.data();
    float *vx = velX.data();
    float *vy = velY.data();
    const int *flatIdx = flatIndices.data();
    const int *cellOff = cellOffsets.data();
    const int *cids = colorIDs.data();

    const float repelRangeSq = fRepelRange * fRepelRange;
    const float interactRangeSq = fInteractRange * fInteractRange;

    std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), [&](int i)
    {
        float xi = px[i];
        float yi = py[i];

        int cellX = static_cast<int>(((xi + 1.0f) * 0.5f) * gridCols);
        int cellY = static_cast<int>(((yi + 1.0f) * 0.5f) * gridRows);
        cellX = std::clamp(cellX, 0, gridCols - 1);
        cellY = std::clamp(cellY, 0, gridRows - 1);

        const auto &row = interactionMatrix[cids[i]];
        const float *rowPtr = row.data();

        if (!neighborList.empty())
        {
            const auto &nlist = neighborList[i];
            for (int k = 0, nk = static_cast<int>(nlist.size()); k < nk; ++k)
            {
                int j = nlist[k];
                // Shortest periodic displacement
                float dx = px[j] - xi;
                if (dx > 1.0f) dx -= 2.0f; else if (dx < -1.0f) dx += 2.0f;
                float dy = py[j] - yi;
                if (dy > 1.0f) dy -= 2.0f; else if (dy < -1.0f) dy += 2.0f;

                float distSq = dx*dx + dy*dy;
                if (distSq < 1e-8f) continue;

                if (distSq < repelRangeSq)
                {
                    float dist = std::sqrt(distSq);
                    float strength = (fRepelRange - dist) / fRepelRange;
                    strength *= strength;
                    float impulse = strength * fRepelForce * simDelta;
                    float maxImpulse = 0.10f * simDelta;
                    if (impulse > maxImpulse) impulse = maxImpulse;
                    vx[i] -= dx * impulse;
                    vy[i] -= dy * impulse;
                }
                else if (distSq < interactRangeSq)
                {
                    float dist = std::sqrt(distSq);
                    float normalized = (fInteractRange - dist) / fInteractRange;
                    float strength = normalized * normalized;
                    float factor = fInteractForce * rowPtr[cids[j]];
                    float impulse = strength * factor * simDelta;
                    float maxImpulse = 0.15f * simDelta;
                    if (impulse > maxImpulse) impulse = maxImpulse;
                    vx[i] += dx * impulse;
                    vy[i] += dy * impulse;
                }
            }
        }
        else
        {
            for (int oy = -maxCellOffset; oy <= maxCellOffset; ++oy)
            {
                int ny = cellY + oy;
                if (ny < 0 || ny >= gridRows) continue;
                for (int ox = -maxCellOffset; ox <= maxCellOffset; ++ox)
                {
                    int nx = cellX + ox;
                    if (nx < 0 || nx >= gridCols) continue;
                    int neighborCell = ny * gridCols + nx;
                    int start = cellOff[neighborCell];
                    int end = cellOff[neighborCell + 1];
                    for (int idx = start; idx < end; ++idx)
                    {
                        int j = flatIdx[idx];
                        if (j == i) continue;

                        // Shortest periodic displacement
                        float dx = px[j] - xi;
                        if (dx > 1.0f) dx -= 2.0f; else if (dx < -1.0f) dx += 2.0f;
                        float dy = py[j] - yi;
                        if (dy > 1.0f) dy -= 2.0f; else if (dy < -1.0f) dy += 2.0f;

                        float distSq = dx*dx + dy*dy;
                        if (distSq < 1e-8f) continue;

                        if (distSq < repelRangeSq)
                        {
                            float dist = std::sqrt(distSq);
                            float strength = (fRepelRange - dist) / fRepelRange;
                            strength *= strength;
                            float impulse = strength * fRepelForce * simDelta;
                            float maxImpulse = 0.10f * simDelta;
                            if (impulse > maxImpulse) impulse = maxImpulse;
                            vx[i] -= dx * impulse;
                            vy[i] -= dy * impulse;
                        }
                        else if (distSq < interactRangeSq)
                        {
                            float dist = std::sqrt(distSq);
                            float normalized = (fInteractRange - dist) / fInteractRange;
                            float strength = normalized * normalized;
                            float factor = fInteractForce * rowPtr[cids[j]];
                            float impulse = strength * factor * simDelta;
                            float maxImpulse = 0.15f * simDelta;
                            if (impulse > maxImpulse) impulse = maxImpulse;
                            vx[i] += dx * impulse;
                            vy[i] += dy * impulse;
                        }
                    }
                }
            }
        }

        // Damping & velocity cap (simple inline approximation of Body::Update behaviour)
        float speedSq = velX[i]*velX[i] + velY[i]*velY[i];
        if (speedSq > 0.0f)
        {
            float speed = std::sqrt(speedSq);
            float dragFactor = 1.0f - (0.8f * speed * simDelta);
            if (dragFactor < 0.0f) dragFactor = 0.0f;
            velX[i] *= dragFactor;
            velY[i] *= dragFactor;
        }

        float maxV = 1.0f;
        float curSpeed = std::sqrt(velX[i]*velX[i] + velY[i]*velY[i]);
        if (curSpeed > maxV)
        {
            float s = maxV / curSpeed;
            velX[i] *= s; velY[i] *= s;
        }

        // Integrate & wrap
        posX[i] += velX[i] * simDelta;
        posY[i] += velY[i] * simDelta;
        if (posX[i] < -1.0f) posX[i] += 2.0f; else if (posX[i] > 1.0f) posX[i] -= 2.0f;
        if (posY[i] < -1.0f) posY[i] += 2.0f; else if (posY[i] > 1.0f) posY[i] -= 2.0f;
    });
}

} // namespace pcompute
