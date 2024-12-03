cmake_minimum_required(VERSION 3.15)
project(MoldLab3D)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files
file(GLOB_RECURSE SRC_FILES src/*.cpp)

# Include directories
include_directories(include)

# Executable target
add_executable(MoldLab3D ${SRC_FILES})

# Find and link libraries
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

target_link_libraries(MoldLab3D OpenGL::GL glfw glm)
