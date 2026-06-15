# Project information
cmake_minimum_required(VERSION 3.10)
project(ParticleRenderer)

# C++ version
set(CMAKE_CXX_STANDARD 20)

# Gets packages to be used for library
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)

# Libraries required for project
target_link_libraries(ParticleRenderer
    OpenGL::GL
    GLEW::GLEW
    glfw
)