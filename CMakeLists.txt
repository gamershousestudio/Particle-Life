# Project information
cmake_minimum_required(VERSION 3.10)
project(ParticleLife)

# C++ version
set(CMAKE_CXX_STANDARD 23)

# Define Body library here
add_library(Body dependencies/Body/body.cpp)
target_include_directories(Body PUBLIC dependencies/Body)

# Include the sub-projects
add_subdirectory(dependencies/ParticleRenderer)
add_subdirectory(dependencies/UI)

# What files are compiled during build
add_executable(ParticleLife src/main.cpp)

# Copy shaders to build
add_custom_command(TARGET ParticleLife POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/res"
    "${CMAKE_BINARY_DIR}/res"
)

# SINGLE CONSOLIDATED INCLUDE AND LINK BLOCK
if(WIN32)
    # FORCE the compiler to bake libgcc, libstdc++, and winpthreads directly inside your .exe
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

    # Your existing Windows configuration follows completely untouched
    target_include_directories(ParticleLife PRIVATE 
        src
        dependencies
        dependencies/Body
        dependencies/ParticleRenderer
        dependencies/UI
        thirdparty
    )
    
    target_compile_definitions(ParticleLife PRIVATE GLFW_DLL)
    target_compile_definitions(ParticleRenderer PRIVATE GLFW_DLL)
    target_compile_definitions(UI PRIVATE GLFW_DLL)
    
    target_link_libraries(ParticleLife PRIVATE
        ParticleRenderer
        Body
        UI
        "${CMAKE_SOURCE_DIR}/thirdparty/glfw3.dll"
        "${CMAKE_SOURCE_DIR}/thirdparty/glew32.dll"
        "${CMAKE_SOURCE_DIR}/thirdparty/freetype.dll"
        opengl32
        gdi32
    )
else()
    # Appends all paths for your native Linux/Ubuntu build
    target_include_directories(ParticleLife PRIVATE 
        src
        dependencies
        dependencies/Body
        dependencies/ParticleRenderer
        dependencies/UI
    )

    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(Freetype REQUIRED) # Ensure Freetype lookup is active

    # Link everything to the main executable (Adding Freetype::Freetype fixes your include error)
    target_link_libraries(ParticleLife PRIVATE
        ParticleRenderer
        Body
        UI
        glfw
        OpenGL::GL
        GLEW::GLEW
        Freetype::Freetype
    )
endif()
