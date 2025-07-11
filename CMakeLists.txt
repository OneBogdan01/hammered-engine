# Minimum required version of CMake
cmake_minimum_required(VERSION 3.16.0)

# Define the project name and programming language
project(hammered_template)

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

set(BUILD_SCRIPT_SRC  "${CMAKE_SOURCE_DIR}/build_backends.py")
set(BUILD_SCRIPT_DEST "${CMAKE_BINARY_DIR}/build_backends.py")

configure_file(
    "${BUILD_SCRIPT_SRC}"            # input
    "${BUILD_SCRIPT_DEST}"           # output
    COPYONLY                         # copy byte‑for‑byte, update only if different
)

# ── Utility target: run the helper script (e.g. ninja backends) ───────────
add_custom_target(backends
    COMMAND ${CMAKE_COMMAND} -E echo "Running build_backends.py all"
    COMMAND ${CMAKE_COMMAND} -E env python3 "${BUILD_SCRIPT_DEST}" all
    COMMENT "Building all graphics back‑ends"
)

set_target_properties(backends PROPERTIES FOLDER "utilities")



set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_subdirectory(engine)

# Asset function
set(ASSET_SOURCE_DIR "${CMAKE_SOURCE_DIR}/game/assets")
function(configure_assets_for target)


    set(ASSET_BINARY_DIR "$<TARGET_FILE_DIR:${target}>/assets")
    set(ASSET_SOLUTION_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")

    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSET_SOURCE_DIR}"
        "${ASSET_BINARY_DIR}"
    )
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSET_SOURCE_DIR}"
        "${ASSET_SOLUTION_DIR}"
    )
endfunction()

# figure out final executable suffix
set(EXE_SUFFIX "${CMAKE_EXECUTABLE_SUFFIX}")

set(GAME_GL_EXECUTABLE_NAME "game_gl${EXE_SUFFIX}")
set(GAME_VK_EXECUTABLE_NAME "game_vk${EXE_SUFFIX}")
function(exec_macro_for target)
    target_compile_definitions(${target}
    PUBLIC
        GAME_GL_EXECUTABLE_NAME="${GAME_GL_EXECUTABLE_NAME}"
        GAME_VK_EXECUTABLE_NAME="${GAME_VK_EXECUTABLE_NAME}"
)
endfunction()
# compiles shaders
add_subdirectory(game)
function(add_backend_helper backend exe_target)
    set(stamp "${CMAKE_CURRENT_BINARY_DIR}/.${backend}_resources.stamp")

    # Custom command that (1) runs the helper, (2) touches a stamp file.
    add_custom_command(
        OUTPUT  "${stamp}"
        COMMAND ${CMAKE_COMMAND} -E env
                python3 "${CMAKE_BINARY_DIR}/build_backends.py" "${backend}"
        COMMAND ${CMAKE_COMMAND} -E touch "${stamp}"
        COMMENT "Generating resources for ${backend}"
        VERBATIM)

    # Make the executable depend on the stamp, not the other way around.
    add_custom_target(resources_${backend} DEPENDS "${stamp}")
    add_dependencies(${exe_target} resources_${backend})
endfunction()
# Create backends in a loop
foreach(backend ${GAME_BACKENDS})
    set(target_name "game_${backend}")
    set(engine_target "hammered_engine_${backend}")
    # TODO Needs to be extended for scripting capabilities
    add_executable(${target_name} game/src/main.cpp)

    target_link_libraries(${target_name} PRIVATE  ${engine_target})

    # each engine backend compiles shaders
    add_dependencies(${engine_target} shaders)

    add_backend_helper(${backend} ${target_name})   
    exec_macro_for(${target_name})
    exec_macro_for(${engine_target})
      

    set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 20)
    configure_assets_for(${target_name})

endforeach()


# Set the Visual Studio startup project to the first enabled backend executable
list(GET GAME_BACKENDS 0 FIRST_BACKEND)

set(STARTUP_PROJECT "game_${FIRST_BACKEND}")

message(STATUS "Setting VS startup project to ${STARTUP_PROJECT}")
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${STARTUP_PROJECT})

