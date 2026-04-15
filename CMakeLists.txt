cmake_minimum_required(VERSION 3.21)

project(DosochkaServerBusinessLogic LANGUAGES CXX)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

include(FetchContent)

find_package(PostgreSQL QUIET)
find_package(Boost QUIET COMPONENTS asio json system)
find_package(Qt6 QUIET COMPONENTS Core Widgets Network)
if(Qt6_FOUND)
    find_package(Qt6 QUIET COMPONENTS Test)
endif()

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    find_package(GTest QUIET)
endif()

if(NOT GTest_FOUND)
    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

add_subdirectory(libs)
add_subdirectory(apps)
add_subdirectory(tests)
