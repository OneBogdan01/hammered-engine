# Asset function

function(configure_assets_for target)
    set(ASSET_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
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

# Create backends in a loop
foreach(backend ${GAME_BACKENDS})
    set(target_name "game_${backend}")
    set(engine_target "hammered_engine_${backend}")
    add_executable(${target_name} src/main.cpp)
    target_link_libraries(${target_name} PRIVATE  ${engine_target})
    set_property(TARGET ${target_name} PROPERTY CXX_STANDARD 20)
    configure_assets_for(${target_name})
endforeach()
