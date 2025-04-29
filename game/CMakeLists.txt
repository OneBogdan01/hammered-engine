add_executable(game
    src/main.cpp
)
set_property(TARGET game PROPERTY CXX_STANDARD 20)
target_link_libraries(game PRIVATE tiny_ape_light_engine)
