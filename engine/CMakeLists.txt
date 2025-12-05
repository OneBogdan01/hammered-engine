add_library(hammered_engine_common INTERFACE)
# Add dependencies
add_subdirectory(external)
add_subdirectory(code)

foreach(backend ${GAME_BACKENDS})
    set(engine_target "hammered_engine_${backend}")

    # Common libraries for all backends
    target_link_libraries(${engine_target}
        PUBLIC
            hammered_engine_common
            glm
            stb_image
            SDL3::SDL3
            imgui_core
            tinygltf
            EnTT
            glslang::glslang
            glslang::SPIRV
            glslang::glslang-default-resource-limits
            Tracy::TracyClient
    )

    # Backend-specific libraries
    if(backend STREQUAL "gl")
        target_link_libraries(${engine_target}
            PUBLIC
                glad
                imgui_opengl
        )
    elseif(backend STREQUAL "vk")
        target_link_libraries(${engine_target}
            PUBLIC
                volk
                vkbootstrap
                vma
                imgui_vulkan
                
        )
    endif()

    target_precompile_headers(${engine_target} PUBLIC
        "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/code/include/pch.hpp>"
    )
    set_target_properties(${engine_target} PROPERTIES FOLDER "engine")
endforeach()
