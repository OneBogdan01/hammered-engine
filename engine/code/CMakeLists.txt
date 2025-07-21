foreach(backend ${GAME_BACKENDS})
    set(target_name "hammered_engine_${backend}")

    # common sources
    file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp
    )
    list(FILTER ALL_SOURCES EXCLUDE REGEX ".*/platform/.*")

    file(GLOB_RECURSE ALL_HEADERS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp
    )
    list(FILTER ALL_HEADERS EXCLUDE REGEX ".*/platform/.*")

    set(ENGINE_SOURCES ${ALL_SOURCES})
    set(ENGINE_HEADERS ${ALL_HEADERS})

    # backend-specific sources
    if(backend STREQUAL "gl")
        file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/source/platform/opengl/*.cpp
        )
        file(GLOB_RECURSE PLATFORM_HEADERS CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/include/platform/opengl/*.hpp
        )
        set(PLATFORM_DEFINE "GFX_USE_OPENGL")
    elseif(backend STREQUAL "vk")
        file(GLOB_RECURSE PLATFORM_SOURCES CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/source/platform/vulkan/*.cpp
        )
        file(GLOB_RECURSE PLATFORM_HEADERS CONFIGURE_DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/include/platform/vulkan/*.hpp
        )
        set(PLATFORM_DEFINE "GFX_USE_VULKAN")
    else()
        message(FATAL_ERROR "Unknown backend: ${backend}")
    endif()

    list(APPEND ENGINE_SOURCES ${PLATFORM_SOURCES})
    list(APPEND ENGINE_HEADERS ${PLATFORM_HEADERS})

    add_library(${target_name} STATIC
        ${ENGINE_SOURCES}
        ${ENGINE_HEADERS}
    )

    target_include_directories(${target_name} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_compile_features(${target_name} PUBLIC cxx_std_20)
    target_compile_definitions(${target_name} PUBLIC ${PLATFORM_DEFINE})



    # Visual Studio organization
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/source PREFIX "source" FILES ${ENGINE_SOURCES})
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include PREFIX "include" FILES ${ENGINE_HEADERS})

    set_target_properties(${target_name} PROPERTIES FOLDER "engine")
endforeach()
