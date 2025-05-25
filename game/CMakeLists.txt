add_executable(game
    src/main.cpp
)
set_property(TARGET game PROPERTY CXX_STANDARD 20)
target_link_libraries(game PRIVATE tiny_ape_light_engine)

# Define asset source and target paths
set(ASSET_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(ASSET_BINARY_DIR "$<TARGET_FILE_DIR:game>/assets")
set(ASSET_SOLUTION_DIR "${CMAKE_CURRENT_BINARY_DIR}/assets")  

# Copy assets next to the binary
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${ASSET_SOURCE_DIR}"
    "${ASSET_BINARY_DIR}"
)

# Copy assets to the solution directory
add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${ASSET_SOURCE_DIR}"
    "${ASSET_SOLUTION_DIR}"
)