# Project information
cmake_minimum_required(VERSION 3.10)
project(ParticleLife)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

# C++ version
set(CMAKE_CXX_STANDARD 23)

# Option to Enable AddressSanitizer builds for debugging memory issues.
#
# Usage: Configure the project with `-DENABLE_ASAN=ON` to add compile and
# link flags that enable AddressSanitizer (ASan) and UndefinedBehaviorSanitizer
# (UBSan) for the `ParticleLife` target. This is intended for debugging and
# automated test runs — leave it OFF for production builds to avoid the
# performance and binary-size impact of sanitizers.
option(ENABLE_ASAN "Build with AddressSanitizer enabled" OFF)


# Define Body library here
add_library(Body dependencies/Body/body.cpp)
target_include_directories(Body PUBLIC dependencies/Body)

# Include the sub-projects
add_subdirectory(dependencies/ParticleRenderer)
add_subdirectory(dependencies/UI)
add_subdirectory(dependencies/ParticleCompute)

# What files are compiled during build
add_executable(ParticleLife src/main.cpp
    src/RunLoop.cpp src/RunLoop.cpp)

# Small static library containing shared simulation config globals
add_library(SimConfigLib STATIC src/SimConfig.cpp)
target_include_directories(SimConfigLib PUBLIC src)

# Copy shaders to build
add_custom_command(TARGET ParticleLife POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/res"
    "${CMAKE_BINARY_DIR}/res"
)

# Make Linux bundles load dependencies from a sibling lib directory instead of
# depending on the target machine having the same libraries installed. The
# packaging script in tools/release_bundle.py copies those runtime files into a
# portable local bundle and then zips it for GitHub upload.
set_target_properties(ParticleLife PROPERTIES
    BUILD_RPATH "$ORIGIN/lib"
    INSTALL_RPATH "$ORIGIN/lib"
    SKIP_BUILD_RPATH FALSE
)

install(TARGETS ParticleLife RUNTIME DESTINATION bin)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/res" DESTINATION "." FILES_MATCHING PATTERN "*")

add_custom_target(release_bundle
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_SOURCE_DIR}/tools/release_bundle.py"
            --platform "${CMAKE_SYSTEM_NAME}"
            --binary "$<TARGET_FILE:ParticleLife>"
            --root "${CMAKE_SOURCE_DIR}"
            --output "${CMAKE_BINARY_DIR}/release"
    DEPENDS ParticleLife
    COMMENT "Packaging a portable release bundle for ${CMAKE_SYSTEM_NAME}"
    VERBATIM
)

# SINGLE CONSOLIDATED INCLUDE AND LINK BLOCK
if(WIN32)
    # Keep the Windows build portable and compatible with normal MinGW installs.
    # The app is expected to be distributed with DLLs alongside the EXE, not
    # with repo-local placeholders.
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

    target_include_directories(ParticleLife PRIVATE 
        src
        dependencies
        dependencies/Body
        dependencies/ParticleCompute
        dependencies/ParticleRenderer
        dependencies/UI
        thirdparty
    )

    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(Freetype REQUIRED)

    target_compile_definitions(ParticleLife PRIVATE GLFW_DLL)
    target_compile_definitions(ParticleRenderer PRIVATE GLFW_DLL)
    target_compile_definitions(UI PRIVATE GLFW_DLL)

    target_link_libraries(ParticleLife PRIVATE
        ParticleRenderer
        Body
        UI
        ParticleCompute
        SimConfigLib
        glfw
        OpenGL::GL
        GLEW::GLEW
        Freetype::Freetype
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
        dependencies/ParticleCompute
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
        ParticleCompute
        glfw
        OpenGL::GL
        GLEW::GLEW
        Freetype::Freetype
    )
    target_link_libraries(ParticleLife PRIVATE SimConfigLib)
endif()

if(ENABLE_ASAN)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "Enabling AddressSanitizer for ParticleLife")
        target_compile_options(ParticleLife PRIVATE -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(ParticleLife PRIVATE -fsanitize=address,undefined)
    else()
        message(WARNING "ENABLE_ASAN is set but compiler does not support GNU/Clang sanitizer options")
    endif()
endif()

# Add a small test binary to reproduce spawn behavior and run it with sanitizers
add_executable(spawn_test tests/spawn_test.cpp)
target_include_directories(spawn_test PRIVATE
    src
    dependencies
    dependencies/Body
    dependencies/ParticleRenderer
)
target_link_libraries(spawn_test PRIVATE Body ParticleRenderer SimConfigLib)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(spawn_test PRIVATE -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer)
    target_link_options(spawn_test PRIVATE -fsanitize=address,undefined)
endif()
