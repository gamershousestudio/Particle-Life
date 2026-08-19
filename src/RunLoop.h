#pragma once

#include <vector>
#include <GL/glew.h>
#include "UI.h"
#include "ParticleCompute/ParticleCompute.h"

// Forward declarations
class Particle;
namespace gfx { struct Circle; class CircleRenderer; }

// Run the main GUI loop. The signature mirrors the previous definition in
// `main.cpp` to keep the separation minimal while allowing the main TU to
// remain short and focused on initialization/teardown.
void RunMainLoop(GLFWwindow* window,
                 UI::Panel &panel,
                 UI::SubPanel &subPanel,
                 UI::element grid,
                 UI::element timeText,
                 UI::element timeSlider,
                 UI::element varietyText,
                 UI::element varietySlider,
                 UI::element rerandomizeButton,
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
                 bool useGPU);
