add_library(online_board_persistence INTERFACE)
target_include_directories(online_board_persistence INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(online_board_persistence INTERFACE
    online_board_application
)

if(PostgreSQL_FOUND AND TARGET Boost::json)
    add_library(online_board_persistence_postgres
        src/postgres/postgres_board_member_repository.cpp
        src/postgres/postgres_board_object_repository.cpp
        src/postgres/postgres_board_operation_repository.cpp
        src/postgres/postgres_board_repository.cpp
        src/postgres/postgres_board_runtime_persistence.cpp
        src/postgres/postgres_connection.cpp
        src/postgres/postgres_json.cpp
        src/postgres/postgres_session_repository.cpp
        src/postgres/postgres_user_repository.cpp
    )
    target_include_directories(online_board_persistence_postgres PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_include_directories(online_board_persistence_postgres PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/postgres
    )
    target_link_libraries(online_board_persistence_postgres PUBLIC
        online_board_persistence
        Boost::json
        PostgreSQL::PostgreSQL
    )
else()
    message(STATUS "PostgreSQL or Boost::json not found. Postgres persistence target 'online_board_persistence_postgres' will be skipped.")
endif()
