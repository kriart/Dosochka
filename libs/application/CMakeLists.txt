add_library(online_board_application
    src/auth_service.cpp
    src/board_service.cpp
    src/board_state_detail.cpp
    src/board_state_shape_ops.cpp
    src/board_state_text_ops.cpp
    src/board_state_stroke_ops.cpp
    src/board_state_object_ops.cpp
    src/board_state_service.cpp
)
target_include_directories(online_board_application PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(online_board_application PUBLIC
    online_board_domain
)
