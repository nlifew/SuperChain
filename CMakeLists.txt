cmake_minimum_required(VERSION 3.21)
project(SuperChain)

set(CMAKE_CXX_STANDARD 17)

add_executable(SuperChain main.cpp zip.cpp)

target_link_libraries(
        SuperChain
        z
)
