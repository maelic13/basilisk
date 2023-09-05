cmake_minimum_required(VERSION 3.24)
project(basilisk VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)

add_executable(
        basilisk
        src/Basilisk.cpp src/UciProtocol.cpp src/UciProtocol.h src/Parameters.cpp src/Parameters.h
        src/Engine.cpp src/Engine.h src/Engine.h src/Constants.h src/Board.cpp src/Board.h
        src/Piece.h
)
