cmake_minimum_required(VERSION 3.10)
project(UI)

add_library(UI UI.cpp)

# Use PUBLIC so the compiler can safely see across module directories
target_include_directories(UI BEFORE PUBLIC 
    "${CMAKE_SOURCE_DIR}/dependencies"
    "${CMAKE_SOURCE_DIR}/dependencies/Body"
)

if(WIN32)
    target_include_directories(UI BEFORE PRIVATE 
        "${CMAKE_SOURCE_DIR}/thirdparty"
        "${CMAKE_SOURCE_DIR}/thirdparty/freetype2"
    )
    target_compile_definitions(UI PRIVATE GLFW_DLL)
    target_link_libraries(UI PRIVATE
        "${CMAKE_SOURCE_DIR}/thirdparty/glfw3.dll"
        "${CMAKE_SOURCE_DIR}/thirdparty/glew32.dll"
        "${CMAKE_SOURCE_DIR}/thirdparty/freetype.dll"
        opengl32
        gdi32
    )
else()
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
    find_package(glfw3 REQUIRED)
    find_package(Freetype REQUIRED)
    
    # CHANGED TO PUBLIC: This forces any file that includes UI.h (like main.cpp) 
    # to automatically know where ft2build.h lives on Linux.
    target_link_libraries(UI PUBLIC OpenGL::GL GLEW::GLEW glfw Freetype::Freetype)
endif()

