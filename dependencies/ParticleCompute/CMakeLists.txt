add_library(ParticleCompute STATIC ParticleCompute.cpp)
target_sources(ParticleCompute PRIVATE ParticleSim.cpp)

target_include_directories(ParticleCompute PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Linking to OpenGL/GLEW is provided by the main executable; keep this lib header-only at link time.
