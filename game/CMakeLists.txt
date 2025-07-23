find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

if(GLSL_VALIDATOR)
    message(STATUS "Found glslangValidator: ${GLSL_VALIDATOR}")
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${ASSET_SOURCE_DIR}/shaders/*.frag"
    "${ASSET_SOURCE_DIR}/shaders/*.vert"
    "${ASSET_SOURCE_DIR}/shaders/*.comp"
    "${ASSET_SOURCE_DIR}/shaders/*.tesc"
    "${ASSET_SOURCE_DIR}/shaders/*.tese"
    "${ASSET_SOURCE_DIR}/shaders/*.geom"
    )

# Generate Vulkan and OpenGL SPIR-V for each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)

if(ENABLE_VK_BACKEND)
    # Vulkan SPIR-V
    set(SPIRV_VK "${ASSET_SOURCE_DIR}/shaders/${FILE_NAME}.vk.spv")
    add_custom_command(
        OUTPUT ${SPIRV_VK}
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV_VK}
        DEPENDS ${GLSL}
        COMMENT "Compiling Vulkan SPIR-V: ${FILE_NAME} -> ${SPIRV_VK}"
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV_VK})
endif()
if(ENABLE_GL_BACKEND)
    # OpenGL SPIR-V
    set(SPIRV_GL "${ASSET_SOURCE_DIR}/shaders/${FILE_NAME}.gl.spv")
    add_custom_command(
        OUTPUT ${SPIRV_GL}
        COMMAND ${GLSL_VALIDATOR} -G ${GLSL} -o ${SPIRV_GL}
        DEPENDS ${GLSL}
        COMMENT "Compiling OpenGL SPIR-V: ${FILE_NAME} -> ${SPIRV_GL}"
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV_GL})
endif()

endforeach()

add_custom_target(
    shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)
set_target_properties(shaders PROPERTIES FOLDER "utilities")

else()
  message(STATUS "glslangValidator was not found!")
endif()
