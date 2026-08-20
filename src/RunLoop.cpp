#include "RunLoop.h"
#include "SimConfig.h"
#include "Particle.h"
#include "ParticleCompute/ParticleSim.h"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <chrono>

// Convert SoA particle arrays into render circles.
//
// WHY: The renderer accepts `gfx::Circle` instances for instanced rendering.
// The Simulation, however, operates on SoA buffers (`posX`/`posY`) which are
// optimized for parallel updates and fewer cache misses. Converting only the
// visible attributes into `gfx::Circle` avoids a full AoS/SoA synchronization
// and keeps rendering fast.
static void FillCirclesFromSoA(const std::vector<float> &posX,
                               const std::vector<float> &posY,
                               const std::vector<int> &colorIDs,
                               const std::vector<std::array<float,4>> &cachedColors,
                               float radius,
                               std::vector<gfx::Circle> &circles)
{
    const size_t n = posX.size();
    // The caller is expected to have pre-sized `circles` to n to avoid
    // allocations in the hot path. We write directly into the existing array.
    for (size_t i = 0; i < n; ++i)
    {
        circles[i].x = posX[i];
        circles[i].y = posY[i];
        circles[i].radius = radius;
        int cid = (i < colorIDs.size()) ? colorIDs[i] : 0;
        if (cid < 0 || cid >= static_cast<int>(cachedColors.size())) cid = 0;
        circles[i].color = cachedColors[cid];
    }
}

// Run the main GUI loop. Implementation mirrors the previous layout from
// `main.cpp` but is moved here to keep `main.cpp` short and focused.
void RunMainLoop(GLFWwindow* window,
                 UI::Panel &panel,
                 UI::SubPanel &subPanel,
                 UI::element grid,
                 UI::element timeText,
                 UI::element timeSlider,
                 UI::element varietyText,
                 UI::element varietySlider,
                 UI::element rerandomizeButton,
                 UI::element clearAllButton,
                 UI::element interactForceSlider,
                 UI::element repelForceSlider,
                 UI::element interactRangeSlider,
                 UI::element repelRangeSlider,
                 UI::element interactForceText,
                 UI::element repelForceText,
                 UI::element interactRangeText,
                 UI::element repelRangeText,
                 UI::element amountText,
                 UI::element amountSlider,
                 UI::element spawnButton,
                 UI::element deleteButton,
                 UI::element colorDropdown,
                 gfx::CircleRenderer &renderer,
                 GLuint worldShader,
                 GLuint uiShader,
                 GLuint textShader,
                 std::vector<Particle> &particles,
                 std::vector<float> &posX,
                 std::vector<float> &posY,
                 std::vector<float> &velX,
                 std::vector<float> &velY,
                 std::vector<int> &colorIDs,
                 std::vector<gfx::Circle> &circles,
                 float aspect,
                 ParticleCompute *compute,
                 bool useGPU)
{
    // Compute derived grid constants from global parameters.
    float maxQueryRadius = static_cast<float>(std::max(interactRange, repelRange));
    const float spatialCellSize = std::max((float)radius * 2, maxQueryRadius);
    const int gridCols = static_cast<int>(std::ceil(2.0f / spatialCellSize));
    const int gridRows = gridCols;
    const int gridCellCount = gridCols * gridRows;
    const int maxCellOffset = static_cast<int>(std::ceil(maxQueryRadius / spatialCellSize));

    // Local scratch buffers reused each frame to avoid allocations in the hot
    // loop. Their sizes are determined by `gridCellCount` and particle counts.
    std::vector<int> cellCounts(gridCellCount);
    std::vector<int> cellOffsets(gridCellCount + 1);
    std::vector<int> flatIndices;
    flatIndices.resize(posX.size() > 0 ? posX.size() : 1024);
    std::vector<int> writePos(gridCellCount);
    std::vector<int> particleIndices;
    particleIndices.reserve(particles.capacity() > 0 ? particles.capacity() : 1024);

    std::vector<std::array<float,4>> cachedColors;
    cachedColors.reserve(variety);
    for (unsigned int c = 0; c < variety; ++c) cachedColors.push_back(Particle::GetColor(static_cast<int>(c), 1.0f, variety));

    auto &world = panel.GetWorld();

    int lastDropdownVariety = variety;
    std::chrono::time_point lastTime = std::chrono::steady_clock::now();

    static int frameIdx = 0;
    static std::vector<std::vector<int>> neighborList;
    static int lastNeighborBuildCount = -1;

    while (!glfwWindowShouldClose(window))
    {
        // --- UI Updates ---
        timeSpeed = panel.GetSliderValue(timeSlider);
        const int timePercent = std::llround(timeSpeed * 100.0f);
        panel.SetText(timeText, std::string("Time Speed: ") + std::to_string(timePercent) + "%");

        const float interactForce = panel.GetSliderValue(interactForceSlider);
        const int interactForcePercent = std::llround(interactForce * 100.0f);
        panel.SetText(interactForceText, std::string("Interact Force: " + std::to_string(interactForcePercent) + "%"));

        const float repelForce = panel.GetSliderValue(repelForceSlider);
        const int repelForcePercent = std::llround(repelForce * 100.0f);
        panel.SetText(repelForceText, std::string("Repel Force: " + std::to_string(repelForcePercent) + "%"));

        const float interactRange = panel.GetSliderValue(interactRangeSlider);
        const int interactRangePercent = std::llround(interactRange * 100.0f);
        panel.SetText(interactRangeText, std::string("Interact Range: " + std::to_string(interactRangePercent) + "%"));

        const float repelRange = panel.GetSliderValue(repelRangeSlider);
        const int repelRangePercent = std::llround(repelRange * 100.0f);
        panel.SetText(repelRangeText, std::string("Repel Range: " + std::to_string(repelRangePercent) + "%"));

        const int varietyCount = 1 + static_cast<int>(std::lround(panel.GetSliderValue(varietySlider) * static_cast<float>(colorsCount - 1)));
        variety = static_cast<unsigned int>(std::clamp(varietyCount, 1, colorsCount));
        panel.SetText(varietyText, std::string("Color Variety: ") + std::to_string(variety));
        panel.SetGridBoxCount(grid, variety);

        if (variety != lastDropdownVariety)
        {
            const int newVariety = static_cast<int>(variety);
            std::vector<std::string> colorNames;
            std::vector<std::array<float, 4>> colorOptions;
            const auto stablePalette = Particle::BuildPaletteForVariety(newVariety, newVariety);
            colorNames.reserve(static_cast<size_t>(newVariety));
            colorOptions.reserve(static_cast<size_t>(newVariety));
            for (int i = 0; i < newVariety; ++i)
            {
                colorNames.push_back(Particle::GetColorName(i, newVariety));
                colorOptions.push_back(stablePalette[static_cast<size_t>(i)]);
            }
            subPanel.SetDropdownOptions(colorDropdown, colorNames, colorOptions);

            cachedColors = stablePalette;
            for (auto &entry : cachedColors) entry[3] = 1.0f;
            lastDropdownVariety = newVariety;
        }

        auto gridValues = panel.GetGridValues(grid);

        // --- Timing ---
        auto currentTime = std::chrono::steady_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        float simDelta = deltaTime * timeMultiplier * timeSpeed;
        lastTime = currentTime;

        // --- Build Grid and Update ---
        const int particleCount = static_cast<int>(posX.size());
        pcompute::BuildSpatialGridSoA(posX, posY, particleCount, gridCols, gridRows, cellCounts, cellOffsets, flatIndices, writePos);
        const auto &interactionMatrix = gridValues ? *gridValues : interactions;

        const bool useNeighborCache = particleCount <= 2000;
        const int rebuildInterval = 8;
        const bool rebuildNeighbors = useNeighborCache && ((particleCount != lastNeighborBuildCount) || (frameIdx % rebuildInterval == 0));
        if (rebuildNeighbors && particleCount > 1)
        {
            const float skin = static_cast<float>(interactRange) * 0.5f;
            const float maxNeighborDist = static_cast<float>(interactRange) + skin;
            const float maxNeighborDistSq = maxNeighborDist * maxNeighborDist;
            pcompute::BuildNeighborList(posX, posY, particleCount, gridCols, gridRows, maxCellOffset, cellOffsets, flatIndices, maxNeighborDistSq, neighborList);
            lastNeighborBuildCount = particleCount;
        }
        else if (!useNeighborCache || particleCount <= 1)
        {
            neighborList.assign(particleCount, std::vector<int>());
            lastNeighborBuildCount = particleCount;
        }

        if (useGPU && compute)
        {
            compute->UploadParticles(posX, posY, velX, velY, colorIDs);
            compute->UploadGrid(cellOffsets, flatIndices, gridCols, gridRows, maxCellOffset);
            compute->UploadInteractions(interactionMatrix, colorsCount);
            compute->Dispatch(particleCount, gridCols, gridRows, maxCellOffset, static_cast<float>(interactRange), static_cast<float>(repelRange), static_cast<float>(interactForce), static_cast<float>(repelForce), simDelta);
            compute->ReadBackParticles(posX, posY, velX, velY);
        }
        else
            pcompute::UpdateParticlesSoA(
                posX, posY, velX, velY, colorIDs, particleCount, gridCols, gridRows, 
                maxCellOffset, cellOffsets, flatIndices, interactionMatrix, static_cast<float>(interactRange), 
                static_cast<float>(repelRange), static_cast<float>(interactForce), static_cast<float>(repelForce), 
                simDelta, neighborList, particleIndices
            );
        ++frameIdx;

        // --- Rendering Preparation ---
        if (circles.size() != posX.size()) circles.resize(posX.size());

        if (cachedColors.size() != variety)
        {
            const int currentVariety = static_cast<int>(variety);
            const int previousVariety = static_cast<int>((cachedColors.size() > 0) ? std::min<size_t>(cachedColors.size(), static_cast<size_t>(currentVariety)) : currentVariety);
            cachedColors = Particle::BuildPaletteForVariety(currentVariety, previousVariety);
            for (auto &entry : cachedColors) entry[3] = 1.0f;
        }

        FillCirclesFromSoA(posX, posY, colorIDs, cachedColors, radius, circles);

        // --- Selection Pruning ---
        std::vector<int> prunedSelection; prunedSelection.reserve(world.selected.size());
        std::vector<float> prunedMarkerSizes; prunedMarkerSizes.reserve(world.selected.size());
        const size_t liveParticleCount = posX.size();
        for (size_t i = 0; i < world.selected.size(); ++i)
        {
            int idx = world.selected[i];
            if (idx < 0 || static_cast<size_t>(idx) >= liveParticleCount) continue;
            prunedSelection.push_back(idx);
            prunedMarkerSizes.push_back(world.selectedMarkerSizes[i]);
        }
        world.selected = std::move(prunedSelection);
        world.selectedMarkerSizes = std::move(prunedMarkerSizes);

        // --- Render ---
        glClear(GL_COLOR_BUFFER_BIT);
        renderer.DrawBatch(circles, worldShader, aspect);
        world.DisplaySelectionSoA(uiShader, posX, posY, posX.size());
        world.DisplayAreaSelection(uiShader);

        glfwPollEvents();
        panel.Update(uiShader, textShader, window);

        // --- UI Controls: Spawn/Delete ---
        const bool worldspaceSelected = UI::Events::rightSelectionActive;
        const bool particlesSelected = !world.selected.empty();
        const float amountSliderValue = std::max(0.0f, subPanel.GetSliderValue(amountSlider));
        const int particlesToSpawn = 1 + static_cast<int>(std::round(amountSliderValue * 9999.0f));
        subPanel.SetText(amountText, "Number of Particles: " + std::to_string(particlesToSpawn));
        subPanel.SetActive(particlesSelected || worldspaceSelected);
        subPanel.SetElementActive(amountText, worldspaceSelected);
        subPanel.SetElementActive(amountSlider, worldspaceSelected);
        subPanel.SetElementActive(spawnButton, worldspaceSelected);
        subPanel.SetElementActive(colorDropdown, worldspaceSelected);
        subPanel.SetElementActive(deleteButton, particlesSelected);
        subPanel.Update(uiShader, textShader, window);

        if (UI::Events::selectionRequested)
        {
            UI::Events::selectionRequested = false;
            world.selected.clear(); world.selectedMarkerSizes.clear();
            if (!UI::Events::leftMouseDownStartedOnPanel)
            {
                double xCurrent, yCurrent; glfwGetCursorPos(window, &xCurrent, &yCurrent);
                int w, h; glfwGetWindowSize(window, &w, &h);
                const double currentX = (xCurrent / w) * 2.0 - 1.0;
                const double currentY = -((yCurrent / h) * 2.0 - 1.0);
                const double startX = UI::Events::selectionStartPos[0]; const double startY = UI::Events::selectionStartPos[1];
                const bool dragSelection = std::hypot(currentX - startX, currentY - startY) > 0.001;
                const double minX = std::min(startX, currentX); const double maxX = std::max(startX, currentX);
                const double minY = std::min(startY, currentY); const double maxY = std::max(startY, currentY);
                for (size_t i = 0; i < posX.size(); ++i)
                {
                    const float posXValue = posX[i];
                    const float posYValue = posY[i];
                    const bool inside = dragSelection
                        ? (minX <= posXValue && posXValue <= maxX && minY <= posYValue && posYValue <= maxY)
                        : (std::fabs(posXValue - currentX) <= radius * 2.0 && std::fabs(posYValue - currentY) <= radius * 2.0);
                    if (inside)
                    {
                        world.selected.push_back(static_cast<int>(i));
                        world.selectedMarkerSizes.push_back(static_cast<float>(radius * 1.15));
                    }
                }
            }
        }

        if (particlesSelected && subPanel.IsButtonDown(deleteButton))
        {
            std::vector<size_t> indicesToRemove; indicesToRemove.reserve(world.selected.size());
            for (int idx : world.selected) if (idx >= 0 && static_cast<size_t>(idx) < particles.size()) indicesToRemove.push_back(static_cast<size_t>(idx));
            RemoveParticlesByIndices(particles, posX, posY, velX, velY, colorIDs, indicesToRemove);
            world.selected.clear(); world.selectedMarkerSizes.clear(); subPanel.ResetButton(deleteButton);
        }

        if (panel.IsButtonDown(clearAllButton))
        {
            particles.clear();
            posX.clear();
            posY.clear();
            velX.clear();
            velY.clear();
            colorIDs.clear();
            circles.clear();
            world.selected.clear();
            world.selectedMarkerSizes.clear();
            neighborList.clear();
            particleIndices.clear();
            panel.ResetButton(clearAllButton);
        }

        // Spawn particles: APPEND to SoA buffers to preserve current simulated positions.
        if (subPanel.IsButtonDown(spawnButton))
        {
            if (UI::Events::rightSelectionActive)
            {
                const double minX = std::min(UI::Events::rightSelectionStartPos[0], UI::Events::rightSelectionCurrentPos[0]);
                const double maxX = std::max(UI::Events::rightSelectionStartPos[0], UI::Events::rightSelectionCurrentPos[0]);
                const double minY = std::min(UI::Events::rightSelectionStartPos[1], UI::Events::rightSelectionCurrentPos[1]);
                const double maxY = std::max(UI::Events::rightSelectionStartPos[1], UI::Events::rightSelectionCurrentPos[1]);
                int selectedColorIndex = subPanel.GetDropdownSelectedIndex(colorDropdown); if (selectedColorIndex < 0) selectedColorIndex = 0;
                for (int i = 0; i < particlesToSpawn; i++)
                {
                    float sx = static_cast<float>(minX + (rand() / (float)RAND_MAX) * (maxX - minX));
                    float sy = static_cast<float>(minY + (rand() / (float)RAND_MAX) * (maxY - minY));
                    particles.emplace_back(gfx::Circle{sx, sy, radius, Particle::GetColor(selectedColorIndex, 1.0f, variety)}, selectedColorIndex);
                    posX.push_back(sx);
                    posY.push_back(sy);
                    velX.push_back(0.0f);
                    velY.push_back(0.0f);
                    colorIDs.push_back(selectedColorIndex);
                }
            }
            subPanel.ResetButton(spawnButton);
        }

        if (panel.IsButtonDown(rerandomizeButton)) { RandomizeInteractions(interactions); panel.SetGridBoxCount(grid, variety); panel.ResetButton(rerandomizeButton); }

        // Swap buffers and loop
        glfwSwapBuffers(window);
    }
}
