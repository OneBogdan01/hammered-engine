# Minimum required version of CMake
cmake_minimum_required(VERSION 3.16.0)

# Define the project name and programming language
project(conflict_simulation LANGUAGES CXX)

# Add the engine and game subdirectories, they contain their own CMakeLists.txt
add_subdirectory(engine)
add_subdirectory(game)

# Set the startup project in Visual Studio to be "game"
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT game)
