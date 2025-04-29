cmake_minimum_required(VERSION 3.16)
project(conflict_simulation LANGUAGES CXX)

# Basic Visual Studio project setup
if(MSVC)
    add_compile_options(/EHsc)

    # For clang-cl (detected via simulation ID)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
        add_compile_options(/clang:-Wall /clang:-Wextra /clang:-Werror)
    endif()
endif()

# Native Clang (Linux/macOS)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
    add_compile_options(-Wall -Wextra -Werror -fcolor-diagnostics)

    # Optional sanitizers
    option(ENABLE_SANITIZERS "Enable address and undefined behavior sanitizers" OFF)
    if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

add_subdirectory(engine)
add_subdirectory(game)

# Set Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT game)
