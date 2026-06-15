# Project information
cmake_minimum_required(VERSION 3.28.3)
project(Body)

# Tells C++ this is a library
add_library(Body body.cpp)
target_include_directories(Body PUBLIC include) # Lets project be included from anywhere