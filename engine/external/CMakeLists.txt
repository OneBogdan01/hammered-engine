cmake_minimum_required(VERSION 3.10)

project(external LANGUAGES C CXX)



# System packages
find_package(Vulkan)
find_package(OpenGL)

# vulkan
add_library(vulkan INTERFACE)
set(VULKAN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers/include)
target_sources(vulkan INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan/vulkan.h)
target_include_directories(vulkan SYSTEM INTERFACE ${VULKAN_INCLUDE_DIR})
target_include_directories(vulkan SYSTEM INTERFACE ${VULKAN_INCLUDE_DIR}/vulkan)
target_compile_definitions(vulkan INTERFACE VK_NO_PROTOTYPES)

if(WIN32)
    target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_WIN32_KHR)
elseif(UNIX)
    # Choose WSI based on VKB_WSI_SELECTION
    if (VKB_WSI_SELECTION STREQUAL XCB OR VKB_WSI_SELECTION STREQUAL XLIB OR VKB_WSI_SELECTION STREQUAL WAYLAND)
        find_package(PkgConfig REQUIRED)
    endif()
    if (VKB_WSI_SELECTION STREQUAL XCB)
        pkg_check_modules(XCB xcb REQUIRED)
        if (XCB_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_XCB_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL XLIB)
        pkg_check_modules(X11 x11 REQUIRED)
        if (X11_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_XLIB_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL WAYLAND)
        pkg_check_modules(WAYLAND wayland-client REQUIRED)
        if (WAYLAND_FOUND)
            target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_WAYLAND_KHR)
        endif()
    elseif (VKB_WSI_SELECTION STREQUAL D2D)
        set(DIRECT_TO_DISPLAY TRUE)
        set(DIRECT_TO_DISPLAY TRUE PARENT_SCOPE)
        target_compile_definitions(vulkan INTERFACE VK_USE_PLATFORM_DISPLAY_KHR)
    else()
        message(FATAL_ERROR "Unknown WSI")
    endif()
endif()


# volk
set(VOLK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/volk")
set(VOLK_FILES
    "${VOLK_DIR}/volk.c"
    "${VOLK_DIR}/volk.h")

add_library(volk STATIC ${VOLK_FILES})
set_target_properties(volk PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_link_libraries(volk PUBLIC vulkan)

target_include_directories(volk SYSTEM PUBLIC ${VOLK_DIR})

if (VKB_WSI_SELECTION STREQUAL XCB)
    target_include_directories(volk SYSTEM PUBLIC ${XCB_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL XLIB)
    target_include_directories(volk SYSTEM PUBLIC ${X11_INCLUDE_DIRS})
elseif (VKB_WSI_SELECTION STREQUAL WAYLAND)
    target_include_directories(volk SYSTEM PUBLIC ${WAYLAND_INCLUDE_DIRS})
endif()
set_property(TARGET volk PROPERTY FOLDER "engine/external")

# Header-only libraries
add_library(vma INTERFACE)
target_include_directories(vma INTERFACE vma)

add_library(glm INTERFACE)
target_include_directories(glm INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/glm)

add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/stb_image)

# vkbootstrap
add_library(vkbootstrap STATIC)

target_sources(vkbootstrap PRIVATE 
  vkbootstrap/VkBootstrap.h
  vkbootstrap/VkBootstrap.cpp
  )

target_include_directories(vkbootstrap PUBLIC vkbootstrap)
target_link_libraries(vkbootstrap PUBLIC volk $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
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

# tinygltf
add_library(tinygltf INTERFACE)
set(TINYGLTF_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)
target_sources(tinygltf INTERFACE ${TINYGLTF_DIR}/tiny_gltf.h ${TINYGLTF_DIR}/json.hpp)
target_include_directories(tinygltf SYSTEM INTERFACE ${TINYGLTF_DIR})
set_target_properties(tinygltf PROPERTIES FOLDER "engine/external")

# libktx
set(KTX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ktx)

set(KTX_SOURCES
    ${KTX_DIR}/lib/checkheader.c
    ${KTX_DIR}/external/dfdutils/createdfd.c
    ${KTX_DIR}/external/dfdutils/colourspaces.c
    ${KTX_DIR}/external/dfdutils/dfd.h
    ${KTX_DIR}/external/dfdutils/dfd2vk.inl
    ${KTX_DIR}/external/dfdutils/interpretdfd.c
    ${KTX_DIR}/external/dfdutils/printdfd.c
    ${KTX_DIR}/external/dfdutils/queries.c
    ${KTX_DIR}/external/dfdutils/vk2dfd.c
    ${KTX_DIR}/external/etcdec/etcdec.cxx
    ${KTX_DIR}/lib/etcunpack.cxx
    ${KTX_DIR}/lib/filestream.c
    ${KTX_DIR}/lib/filestream.h
    ${KTX_DIR}/lib/formatsize.h
    ${KTX_DIR}/lib/hashlist.c
    ${KTX_DIR}/lib/info.c
    ${KTX_DIR}/lib/ktxint.h
    ${KTX_DIR}/lib/memstream.c
    ${KTX_DIR}/lib/memstream.h
    ${KTX_DIR}/lib/strings.c
    ${KTX_DIR}/lib/swap.c
    ${KTX_DIR}/lib/uthash.h
    ${KTX_DIR}/lib/texture.c
    ${KTX_DIR}/lib/texture.h
    ${KTX_DIR}/utils/unused.h
    
    # Basis Universal
    ${KTX_DIR}/lib/basis_sgd.h
    ${KTX_DIR}/lib/basis_transcode.cpp
    ${KTX_DIR}/lib/miniz_wrapper.cpp
    ${KTX_DIR}/external/basisu/transcoder/basisu_containers.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_containers_impl.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_file_headers.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_transcoder_internal.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_transcoder_uastc.h
    ${KTX_DIR}/external/basisu/transcoder/basisu_transcoder.cpp
    ${KTX_DIR}/external/basisu/transcoder/basisu_transcoder.h
    ${KTX_DIR}/external/basisu/transcoder/basisu.h
    ${KTX_DIR}/external/basisu/zstd/zstd.c

    # KT1
    ${KTX_DIR}/lib/texture1.c
    ${KTX_DIR}/lib/texture1.h

    # KTX2
    ${KTX_DIR}/lib/texture2.c
    ${KTX_DIR}/lib/texture2.h

    # Vulkan support
    ${KTX_DIR}/lib/vk_format.h
    ${KTX_DIR}/lib/vkformat_check.c
    ${KTX_DIR}/lib/vkformat_enum.h
    ${KTX_DIR}/lib/vkformat_str.c
    ${KTX_DIR}/lib/vkformat_typesize.c
    ${KTX_DIR}/lib/vkformat_check_variant.c
    ${KTX_DIR}/lib/vk_funcs.c
    ${KTX_DIR}/lib/vk_funcs.h
    ${KTX_DIR}/lib/vkloader.c

    # OpenGL support
    ${KTX_DIR}/lib/gl_format.h
    ${KTX_DIR}/lib/gl_funcs.c
    ${KTX_DIR}/lib/gl_funcs.h
    ${KTX_DIR}/lib/glloader.c
)

set(KTX_INCLUDE_DIRS
    ${KTX_DIR}/include
    ${KTX_DIR}/lib
    ${KTX_DIR}/utils
    ${KTX_DIR}/external    
    ${KTX_DIR}/external/basisu/zstd
    ${KTX_DIR}/external/basisu/transcoder
    ${KTX_DIR}/other_include
)

add_library(ktx STATIC ${KTX_SOURCES})

target_compile_definitions(ktx PUBLIC LIBKTX)
if (WIN32)
target_compile_definitions(ktx PUBLIC "KTX_API=__declspec(dllexport)")
endif()
target_compile_definitions(ktx PUBLIC KTX_FEATURE_WRITE=0)
target_compile_definitions(ktx PUBLIC BASISD_SUPPORT_KTX2_ZSTD=0)
target_compile_definitions(ktx PUBLIC BASISU_NO_ITERATOR_DEBUG_LEVEL)

target_include_directories(ktx SYSTEM PUBLIC ${KTX_INCLUDE_DIRS})

target_link_libraries(ktx PUBLIC vulkan)

set_target_properties(ktx PROPERTIES FOLDER "engine/external" POSITION_INDEPENDENT_CODE ON)

# fmt
add_subdirectory(fmt)
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
if(ENABLE_VK_BACKEND) 
    add_library(imgui_vulkan STATIC
        imgui/backends/imgui_impl_vulkan.cpp
    )
    target_link_libraries(imgui_vulkan PUBLIC imgui_core volk)
    set_target_properties(imgui_vulkan PROPERTIES FOLDER "engine/external")
endif()

# OpenGL variant
if(ENABLE_GL_BACKEND) 
    add_library(imgui_opengl STATIC
        imgui/backends/imgui_impl_opengl3.cpp
    )
    target_link_libraries(imgui_opengl PUBLIC imgui_core OpenGL::GL)
    set_target_properties(imgui_opengl PROPERTIES FOLDER "engine/external")
endif()
if(MSVC)
    target_compile_options(glm INTERFACE /W0)
    target_compile_options(stb_image INTERFACE /W0)
    target_compile_options(imgui_core PRIVATE /W0)
if (ENABLE_VK_BACKEND)
    target_compile_options(vkbootstrap PRIVATE /W0)
    target_compile_options(imgui_vulkan PRIVATE /W0)
endif()
if (ENABLE_GL_BACKEND)
    target_compile_options(imgui_opengl PRIVATE /W0)
    target_compile_options(glad PRIVATE /W0)
endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|Intel")
    target_compile_options(glm INTERFACE -w)
    target_compile_options(stb_image INTERFACE -w)
    target_compile_options(imgui_core PRIVATE -w)

if (ENABLE_GL_BACKEND)
    target_compile_options(glad PRIVATE -w)
    target_compile_options(imgui_opengl PRIVATE -w)
endif()
if (ENABLE_VK_BACKEND)
    target_compile_options(imgui_vulkan PRIVATE -w)
    target_compile_options(vkbootstrap PRIVATE -w)
endif()

endif()

