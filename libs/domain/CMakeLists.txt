add_library(online_board_domain INTERFACE)
target_include_directories(online_board_domain INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(online_board_domain INTERFACE
    online_board_common
)
