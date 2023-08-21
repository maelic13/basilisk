cmake_minimum_required(VERSION 3.24)
project(Koloss VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)

add_executable(koloss src/main.cpp src/UciProtocol.cpp src/UciProtocol.h src/SearchOptions.cpp src/SearchOptions.h
        src/Engine.cpp src/Engine.h src/Engine.h src/Constants.h src/EngineOptions.cpp src/EngineOptions.h)
