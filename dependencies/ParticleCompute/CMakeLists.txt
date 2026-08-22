add_library(ParticleCompute STATIC ParticleCompute.cpp)
target_sources(ParticleCompute PRIVATE ParticleSim.cpp)

target_include_directories(ParticleCompute PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

target_link_libraries(ParticleCompute PUBLIC OpenGL::GL GLEW::GLEW)
