
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(VKB_WSI_SELECTION "XCB" CACHE STRING "Select WSI target (XCB, XLIB, WAYLAND, D2D)")

if(MSVC)
    add_compile_options(/MP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /EHsc /DNOMINMAX")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /MDd /Od /Ob0 /DDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Zi /DDEVELOP /MD")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /DNDEBUG /MD")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Configuring some global settings
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
    # Copy it to the source directory for VS Code
    if(CMAKE_EXPORT_COMPILE_COMMANDS)
        add_custom_target(copy_compile_commands ALL
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
            DEPENDS ${CMAKE_BINARY_DIR}/compile_commands.json
        )
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -DNOMINMAX")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DDEVELOP")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")
endif()



## Setting the dlls and .exe in the same folder
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>)

# Asset and Shader Management Functions
set(ASSET_SOURCE_DIR "${CMAKE_SOURCE_DIR}/game/assets")

# Shader compilation function - creates compiled shaders target
function(setup_shader_compilation)
    find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

    if(NOT GLSL_VALIDATOR)
        message(WARNING "glslangValidator was not found! Shader compilation will be skipped.")
        return()
    endif()

    message(STATUS "Found glslangValidator: ${GLSL_VALIDATOR}")

    # Find all shader files
    file(GLOB_RECURSE GLSL_SOURCE_FILES
        "${ASSET_SOURCE_DIR}/shaders/*.frag"
        "${ASSET_SOURCE_DIR}/shaders/*.vert"
        "${ASSET_SOURCE_DIR}/shaders/*.comp"
        "${ASSET_SOURCE_DIR}/shaders/*.tesc"
        "${ASSET_SOURCE_DIR}/shaders/*.tese"
        "${ASSET_SOURCE_DIR}/shaders/*.geom"
    )

    set(SPIRV_BINARY_FILES)
    set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/compiled_shaders")

    # Create output directory
    file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

    # Compile shaders for each enabled backend
    foreach(GLSL ${GLSL_SOURCE_FILES})
        get_filename_component(FILE_NAME ${GLSL} NAME)

        if(ENABLE_VK_BACKEND)
            # Vulkan SPIR-V
            set(SPIRV_VK "${SHADER_OUTPUT_DIR}/${FILE_NAME}.vk.spv")
            add_custom_command(
                OUTPUT ${SPIRV_VK}
                COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV_VK}
                DEPENDS ${GLSL}
                COMMENT "Compiling Vulkan SPIR-V: ${FILE_NAME}"
            )
            list(APPEND SPIRV_BINARY_FILES ${SPIRV_VK})
        endif()

        if(ENABLE_GL_BACKEND)
            # OpenGL SPIR-V
            set(SPIRV_GL "${SHADER_OUTPUT_DIR}/${FILE_NAME}.gl.spv")
            add_custom_command(
                OUTPUT ${SPIRV_GL}
                COMMAND ${GLSL_VALIDATOR} -G ${GLSL} -o ${SPIRV_GL}
                DEPENDS ${GLSL}
                COMMENT "Compiling OpenGL SPIR-V: ${FILE_NAME}"
            )
            list(APPEND SPIRV_BINARY_FILES ${SPIRV_GL})
        endif()
    endforeach()

    # Create shader compilation target
    add_custom_target(
        compile_shaders
        DEPENDS ${SPIRV_BINARY_FILES}
        COMMENT "Compiling all shaders"
    )
    set_target_properties(compile_shaders PROPERTIES FOLDER "utilities")
endfunction()

# Asset copying function - copies assets and compiled shaders to target directory
function(configure_assets_for target)
    set(ASSET_BINARY_DIR "$<TARGET_FILE_DIR:${target}>/assets")
    set(ASSET_SOLUTION_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")

    # Copy source assets (textures, models, etc.)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSET_SOURCE_DIR}"
        "${ASSET_BINARY_DIR}"
        COMMENT "Copying assets to ${target} runtime directory"
    )
    
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSET_SOURCE_DIR}"
        "${ASSET_SOLUTION_DIR}"
        COMMENT "Copying assets to ${target} build directory"
    )

    # Copy compiled shaders if they exist
    set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/compiled_shaders")
    if(EXISTS "${SHADER_OUTPUT_DIR}")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SHADER_OUTPUT_DIR}"
            "${ASSET_BINARY_DIR}/shaders"
            COMMENT "Copying compiled shaders to ${target} runtime directory"
        )
        
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SHADER_OUTPUT_DIR}"
            "${ASSET_SOLUTION_DIR}/shaders"
            COMMENT "Copying compiled shaders to ${target} build directory"
        )
    endif()
endfunction()
# Generic addition of macro per backend
function(exec_macro_for target backend)
    foreach(backend IN LISTS backends)
        string(TOUPPER "${backend}" B_UP)
        target_compile_definitions(${target} PUBLIC
        "GAME_${B_UP}_EXECUTABLE_NAME=\"$<TARGET_FILE_NAME:game_${backend}>\""
            )
    endforeach()

endfunction()

function(add_game_backends backends)
 setup_shader_compilation()

    set(game_exes)

    foreach(backend IN LISTS backends)
        message(STATUS "  Generating targets for backend = ${backend}")
        set(target "game_${backend}")
        set(engine "hammered_engine_${backend}")

        #TODO expand this for scripting, or structuring the game application
        add_executable(${target} game/src/main.cpp)
        target_link_libraries(${target} PRIVATE ${engine})
        # Ensure shaders are compiled before building the target
        if(TARGET compile_shaders)
            add_dependencies(${target} compile_shaders)
        endif()

        exec_macro_for(${target} "${backends}")
        exec_macro_for(${engine} "${backend}")


        set_property(TARGET ${target} PROPERTY CXX_STANDARD 20)
        configure_assets_for(${target})

        list(APPEND game_exes ${target})

    endforeach()

    add_custom_target(build_all_backends ALL
        DEPENDS ${game_exes}
    )
    set_target_properties(build_all_backends PROPERTIES FOLDER "utilities")
endfunction()

