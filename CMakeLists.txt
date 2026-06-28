# Project information
cmake_minimum_required(VERSION 3.10)
project(ParticleLife)

# C++ version
set(CMAKE_CXX_STANDARD 23)

# Gets packages to be used for project
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

# Adds body library to project
add_library(Body
    dependencies/Body/body.cpp
)

# Includes dependencies/Body as one of the root directories
target_include_directories(Body PUBLIC
    dependencies/Body
)

# Adds ParticleRenderer library to this project
add_library(ParticleRenderer
    dependencies/ParticleRenderer/particleRenderer.cpp
)

# Includes dependencies/ParticleRenderer as one of the root directories
target_include_directories(ParticleRenderer PUBLIC
    dependencies/ParticleRenderer
)

# Adds UI library to this project
add_library(UI
    dependencies/UI/UI.cpp
)

# So that UI can use Freetype
find_package(Freetype REQUIRED)

target_link_libraries(UI PUBLIC
    Freetype::Freetype
)

# Includes dependencies/UI as one of the root link_directories
target_include_directories(UI PUBLIC
    dependencies/UI
    dependencies/Body
)

# Links ParticleRenderer and Body
target_link_libraries(ParticleRenderer PUBLIC Body)

# What files are compiled during build
add_executable(ParticleLife
    src/main.cpp
)

# Includes src as one of the root directories
target_include_directories(ParticleLife PRIVATE
    src
)

# Copy shaders to build
add_custom_command(TARGET ParticleLife POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/res"
    "${CMAKE_BINARY_DIR}/res"
)


# Tells CMake that these external libraries will be used in project
target_link_libraries(ParticleLife PRIVATE
    ParticleRenderer
    Body
    UI
    glfw
    OpenGL::GL
    GLEW::GLEW
)