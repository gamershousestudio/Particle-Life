# Project information
cmake_minimum_required(VERSION 3.10)
project(UI)

# C++ version
set(CMAKE_CXX_STANDARD 20)

# Gets packages to be used for library
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Freetype REQUIRED)

add_library(UI
    UI.cpp
)

target_link_libraries(UI PRIVATE
    OpenGL::GL
    GLEW::GLEW
    glfw
    Freetype::Freetype
)