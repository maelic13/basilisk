cmake_minimum_required(VERSION 3.24)

project(basilisk VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(
    basilisk
    src/Basilisk.cpp
    src/UciProtocol.cpp
    src/Parameters.cpp
    src/Engine.cpp
    src/Board.cpp
)

target_include_directories(basilisk PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
