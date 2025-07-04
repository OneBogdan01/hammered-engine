# Minimum required version of CMake
cmake_minimum_required(VERSION 3.16.0)

# Define the project name and programming language
project(conflict_simulation)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# User-configurable options
option(ENABLE_GL_BACKEND "Build with OpenGL backend" ON)
option(ENABLE_VK_BACKEND "Build with Vulkan backend" ON)

# Backend selection
set(GAME_BACKENDS "")

if(ENABLE_GL_BACKEND)
    list(APPEND GAME_BACKENDS gl)
endif()

if(ENABLE_VK_BACKEND)
    list(APPEND GAME_BACKENDS vk)
endif()

# Error if no backends enabled
if(NOT GAME_BACKENDS)
    message(FATAL_ERROR "No game backend enabled! Enable at least one of ENABLE_GL_BACKEND or ENABLE_VK_BACKEND")
endif()


set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Add the engine and game subdirectories, they contain their own CMakeLists.txt
add_subdirectory(engine)
add_subdirectory(game)

# Set the Visual Studio startup project to the first enabled backend executable
list(GET GAME_BACKENDS 0 FIRST_BACKEND)

set(STARTUP_PROJECT "game_${FIRST_BACKEND}")
message(STATUS "Setting VS startup project to ${STARTUP_PROJECT}")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${STARTUP_PROJECT})