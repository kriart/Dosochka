add_executable(online_board_compile_smoke
    runtime/compile_smoke.cpp
)
target_include_directories(online_board_compile_smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(online_board_compile_smoke PRIVATE
    online_board_runtime
    online_board_persistence
)

if((GTest_FOUND OR TARGET GTest::gtest_main) AND Boost_FOUND AND TARGET Boost::asio AND TARGET Boost::json AND TARGET Boost::system)
    add_executable(online_board_tests
        support/test_support.hpp
        auth/auth_service_tests.cpp
        board/board_access_service_tests.cpp
        board/board_service_join_tests.cpp
        board/board_service_lifecycle_tests.cpp
        board/board_service_settings_tests.cpp
        board/board_service_membership_tests.cpp
        board/board_state_shape_tests.cpp
        board/board_state_stroke_tests.cpp
        board/board_state_text_tests.cpp
        operations/operation_service_tests.cpp
        runtime/board_runtime_manager_tests.cpp
        server/server_integration_tests.cpp
        ../apps/server/src/protocol/json_common.cpp
        ../apps/server/src/protocol/json_parsing.cpp
        ../apps/server/src/protocol/json_serialization.cpp
        ../apps/server/src/session/client_session.cpp
        ../apps/server/src/session/client_session_auth.cpp
        ../apps/server/src/session/client_session_board.cpp
        ../apps/server/src/session/client_session_dispatch.cpp
        ../apps/server/src/session/client_session_io.cpp
        ../apps/server/src/core/board_access_coordinator.cpp
        ../apps/server/src/core/server_state.cpp
        ../apps/server/src/transport/json_tcp_server.cpp
    )
    target_include_directories(online_board_tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/apps/server/src
        ${CMAKE_SOURCE_DIR}/apps/server/src/core
        ${CMAKE_SOURCE_DIR}/apps/server/src/protocol
        ${CMAKE_SOURCE_DIR}/apps/server/src/session
        ${CMAKE_SOURCE_DIR}/apps/server/src/transport
        ${Boost_INCLUDE_DIR}
    )
    target_link_libraries(online_board_tests PRIVATE
        online_board_runtime
        online_board_persistence
        online_board_protocol
        Boost::asio
        Boost::json
        Boost::system
        GTest::gtest_main
    )
    if(TARGET online_board_persistence_postgres)
        target_link_libraries(online_board_tests PRIVATE online_board_persistence_postgres)
        target_compile_definitions(online_board_tests PRIVATE ONLINE_BOARD_HAS_POSTGRES)
    endif()
    if(MSVC)
        target_compile_options(online_board_tests PRIVATE /FS /Z7 /MP1)
    endif()
    if(TARGET Boost::headers)
        target_link_libraries(online_board_tests PRIVATE Boost::headers)
    elseif(TARGET Boost::boost)
        target_link_libraries(online_board_tests PRIVATE Boost::boost)
    endif()

    include(GoogleTest)
    gtest_discover_tests(online_board_tests)
else()
    message(WARNING "GTest or Boost server dependencies could not be configured. Test target 'online_board_tests' will not be generated.")
endif()

if(Qt6_FOUND AND TARGET Qt6::Test)
    add_executable(online_board_client_qt_tests
        client/board_session_state_qt_tests.cpp
        ../apps/client-qt/src/session/board_session_state.cpp
        ../apps/client-qt/src/ui/board_participants_view.cpp
        ../apps/client-qt/src/ui/board_canvas_scene_growth.cpp
        ../apps/client-qt/src/ui/board_canvas_snapshot_renderer.cpp
        ../apps/client-qt/src/ui/board_canvas_widget.cpp
        ../apps/client-qt/src/ui/board_tool_panel.cpp
    )
    set_target_properties(online_board_client_qt_tests PROPERTIES AUTOMOC ON)
    target_include_directories(online_board_client_qt_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/client-qt/src
        ${CMAKE_SOURCE_DIR}/apps/client-qt/src/network
        ${CMAKE_SOURCE_DIR}/apps/client-qt/src/session
        ${CMAKE_SOURCE_DIR}/apps/client-qt/src/ui
    )
    target_link_libraries(online_board_client_qt_tests PRIVATE
        online_board_application
        Qt6::Core
        Qt6::Test
        Qt6::Widgets
    )
    if(MSVC)
        target_compile_options(online_board_client_qt_tests PRIVATE /FS)
    endif()
    add_test(NAME online_board_client_qt_tests COMMAND $<TARGET_FILE:online_board_client_qt_tests>)
    set_tests_properties(online_board_client_qt_tests PROPERTIES
        ENVIRONMENT "PATH=$<TARGET_FILE_DIR:Qt6::Core>;$ENV{PATH}"
    )
endif()
