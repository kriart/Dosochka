add_library(online_board_runtime
    src/board_registry.cpp
    src/board_runtime_manager.cpp
)
target_include_directories(online_board_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(online_board_runtime PUBLIC
    online_board_application
)
