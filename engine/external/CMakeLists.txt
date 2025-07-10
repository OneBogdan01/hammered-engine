# Required system packages
find_package(Vulkan REQUIRED)
find_package(OpenGL REQUIRED)

# Header-only libraries
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vma)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)
set_target_properties(glm PROPERTIES FOLDER "engine/external")

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb_image)
set_target_properties(stb_image PROPERTIES FOLDER "engine/external")

# vkbootstrap
add_library(vkbootstrap STATIC)

target_sources(vkbootstrap PRIVATE 
  vkbootstrap/VkBootstrap.h
  vkbootstrap/VkBootstrap.cpp
  )

target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
set_property(TARGET vkbootstrap PROPERTY CXX_STANDARD 20)
set_target_properties(vkbootstrap PROPERTIES FOLDER "engine/external")

# SDL3
add_subdirectory(SDL)  
# Organize all SDL-related targets under "engine/external"
get_property(SDL_TARGETS DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/SDL PROPERTY BUILDSYSTEM_TARGETS)
foreach(tgt IN LISTS SDL_TARGETS)
    if (TARGET ${tgt})
        set_target_properties(${tgt} PROPERTIES FOLDER "engine/external")
    endif()
endforeach()


# fmt
add_subdirectory(fmt EXCLUDE_FROM_ALL)
set_target_properties(fmt PROPERTIES FOLDER "engine/external")

# glad
add_library(glad STATIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c)
target_include_directories(glad PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/glad/include)
set_target_properties(glad PROPERTIES FOLDER "engine/external")


# imgui
set(IMGUI_CORE
    imgui/imgui.cpp
    imgui/imgui_demo.cpp
    imgui/imgui_draw.cpp
    imgui/imgui_widgets.cpp
    imgui/imgui_tables.cpp
    imgui/backends/imgui_impl_sdl3.cpp
)

add_library(imgui_core STATIC ${IMGUI_CORE})
target_include_directories(imgui_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
target_link_libraries(imgui_core PUBLIC SDL3::SDL3)
set_target_properties(imgui_core PROPERTIES FOLDER "engine/external")

# Vulkan variant
add_library(imgui_vulkan STATIC
    imgui/backends/imgui_impl_vulkan.cpp
)
target_link_libraries(imgui_vulkan PUBLIC imgui_core Vulkan::Vulkan)
set_target_properties(imgui_vulkan PROPERTIES FOLDER "engine/external")

# OpenGL variant
add_library(imgui_opengl STATIC
    imgui/backends/imgui_impl_opengl3.cpp
)
target_link_libraries(imgui_opengl PUBLIC imgui_core OpenGL::GL)
set_target_properties(imgui_opengl PROPERTIES FOLDER "engine/external")

if(MSVC)
    target_compile_options(glm INTERFACE /W0)
    target_compile_options(stb_image INTERFACE /W0)
    target_compile_options(vkbootstrap PRIVATE /W0)
    target_compile_options(glad PRIVATE /W0)
    target_compile_options(imgui_core PRIVATE /W0)
    target_compile_options(imgui_vulkan PRIVATE /W0)
    target_compile_options(imgui_opengl PRIVATE /W0)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|Intel")
    target_compile_options(glm INTERFACE -w)
    target_compile_options(stb_image INTERFACE -w)
    target_compile_options(vkbootstrap PRIVATE -w)
    target_compile_options(glad PRIVATE -w)
    target_compile_options(imgui_core PRIVATE -w)
    target_compile_options(imgui_vulkan PRIVATE -w)
    target_compile_options(imgui_opengl PRIVATE -w)
endif()

