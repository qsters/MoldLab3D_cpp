cmake_minimum_required(VERSION 3.15)
project(MoldLab3D)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add source files (make sure glad.c is explicitly included)
file(GLOB_RECURSE SRC_FILES src/*.cpp src/glad.c)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(C:/msys64/mingw64/include)

# Link directories
link_directories(C:/msys64/mingw64/lib)

# Executable target
add_executable(MoldLab3D ${SRC_FILES})

# Find and link libraries
find_package(OpenGL REQUIRED)

# Manually link GLFW (if find_package does not work in MSYS2)
# Use static library for GLFW
target_link_libraries(MoldLab3D OpenGL::GL C:/msys64/mingw64/lib/libglfw3.a)

# Optionally link dynamic library instead:
# target_link_libraries(MoldLab3D OpenGL::GL C:/msys64/mingw64/lib/libglfw3.dll.a)
