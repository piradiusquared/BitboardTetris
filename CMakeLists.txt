cmake_minimum_required(VERSION 3.16)

project(TetrisBot)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(raylib REQUIRED)

add_executable(tetris
    src/tetris.cpp
)

target_link_libraries(tetris PRIVATE raylib)