
# Add dependencies
add_subdirectory(external)
add_subdirectory(code)

foreach(backend ${GAME_BACKENDS})
    set(engine_target "hammered_engine_${backend}")

    # Common libraries for all backends
    target_link_libraries(${engine_target}
        PUBLIC
            glm
            stb_image
            fmt::fmt
            SDL3::SDL3
            imgui_core
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
                vkbootstrap
                vma
                imgui_vulkan
        )
    endif()
    # Set Visual Studio folder
    set_target_properties(${engine_target} PROPERTIES FOLDER "engine")
endforeach()
