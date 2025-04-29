cmake_minimum_required(VERSION 3.16)
project(conflict_simulation LANGUAGES CXX)

add_subdirectory(engine)
add_subdirectory(game)

# Set the startup project in Visual Studio
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT game)

