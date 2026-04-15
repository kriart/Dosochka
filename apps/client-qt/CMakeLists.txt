if(Qt6_FOUND)
    add_executable(online_board_client_qt
        src/main.cpp
        src/session/board_operation_builder.cpp
        src/session/client_session_controller.cpp
        src/session/client_session_controller_connection.cpp
        src/session/client_session_controller_messages.cpp
        src/ui/main_window.cpp
        src/ui/main_window_network.cpp
        src/ui/main_window_pages.cpp
        src/ui/main_window_state.cpp
        src/ui/main_window_styles.cpp
        src/ui/board_canvas_scene_growth.cpp
        src/ui/board_canvas_snapshot_renderer.cpp
        src/ui/board_tool_panel.cpp
        src/ui/board_top_bar.cpp
        src/ui/board_participants_view.cpp
        src/network/client_json.cpp
        src/session/board_session_state.cpp
        src/network/server_connection.cpp
        src/ui/board_canvas_widget.cpp
    )
    target_include_directories(online_board_client_qt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/network
        ${CMAKE_CURRENT_SOURCE_DIR}/src/session
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    )
    target_link_libraries(online_board_client_qt PRIVATE
        online_board_application
        online_board_protocol
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
    )
    if(MSVC)
        target_compile_options(online_board_client_qt PRIVATE /FS)
    endif()
else()
    message(STATUS "Qt6 Core/Widgets/Network not found. Client target 'online_board_client_qt' will be skipped.")
endif()
