if(Boost_FOUND AND TARGET Boost::asio AND TARGET Boost::json AND TARGET Boost::system)
    add_executable(online_board_server
        src/protocol/json_common.cpp
        src/protocol/json_parsing.cpp
        src/protocol/json_serialization.cpp
        src/session/client_session.cpp
        src/session/client_session_auth.cpp
        src/session/client_session_board.cpp
        src/session/client_session_dispatch.cpp
        src/session/client_session_io.cpp
        src/main.cpp
        src/core/server_state.cpp
        src/transport/json_tcp_server.cpp
    )
    target_include_directories(online_board_server PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol
        ${CMAKE_CURRENT_SOURCE_DIR}/src/session
        ${CMAKE_CURRENT_SOURCE_DIR}/src/transport
        ${Boost_INCLUDE_DIR}
    )
    target_compile_definitions(online_board_server PRIVATE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0A00
    )
    target_link_libraries(online_board_server PRIVATE
        online_board_runtime
        online_board_persistence
        online_board_protocol
        Boost::asio
        Boost::json
        Boost::system
    )
    if(TARGET online_board_persistence_postgres)
        target_link_libraries(online_board_server PRIVATE
            online_board_persistence_postgres
        )
        target_compile_definitions(online_board_server PRIVATE
            ONLINE_BOARD_HAS_POSTGRES
        )
    endif()
    if(TARGET Boost::headers)
        target_link_libraries(online_board_server PRIVATE Boost::headers)
    elseif(TARGET Boost::boost)
        target_link_libraries(online_board_server PRIVATE Boost::boost)
    endif()
else()
    message(STATUS "Boost asio/json/system not found. Server target 'online_board_server' will be skipped.")
endif()
